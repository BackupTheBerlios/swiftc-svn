#include "fe/llvmtypebuilder.h"

#include <llvm/Module.h>
#include <llvm/Support/TypeBuilder.h>

#include "fe/class.h"
#include "fe/error.h"
#include "fe/type.h"

namespace swift {

LLVMTypebuilder::LLVMTypebuilder(Module* module)
    : module_(module)
    , result_(true)
{
    typedef Module::ClassMap::const_iterator CIter;

    for (CIter iter = module_->classes().begin(); iter != module_->classes().end(); ++iter)
    {
        Class* c = iter->second;

        // skip builtin types
        if ( BaseType::isBuiltin(c->id()) )
            continue;

        if ( process(c) )
            continue;
        else
        {
            swiftAssert( cycle_.size() >= 2, 
                    "there must be at least two classes involved in the cycle" );

            ClassSet::const_iterator iter = cycle_.begin();
            errorf( (*iter)->loc(), "there is a cyclic dependency between class '%s'",
                    (*iter)->cid() );

            for (++iter; iter != cycle_.end(); ++iter)
                errorf( (*iter)->loc(), "and class '%s'", (*iter)->cid() );

            result_ = false;
            return;
        }
    }
}

bool LLVMTypebuilder::getResult() const
{
    return result_;
}

bool LLVMTypebuilder::process(Class* c)
{
    // already processed?
    if ( c->llvmType() )
        return true;

    typedef Class::MemberVars::const_iterator MVIter;

    if ( cycle_.contains(c) )
        return false; // pass-through error
    else
        cycle_.insert(c);

    std::vector<const llvm::Type*> llvmTypes;
    for (size_t i = 0; i < c->memberVars().size(); ++i)
    {
        c->memberVars()[i]->index_ = i;
        const Type* type = c->memberVars()[i]->getType();

        if ( !type->isSimple() )
        {
            if (const BaseType* bt = type->isInner() )
            {
                Class* c = bt->lookupClass(module_);
                swiftAssert(c, "must be found");

                if ( !process(c) )
                    return false;
            }

            swiftAssert(true, "TODO");
        }

        llvmTypes.push_back( type->getLLVMType(module_) );
    }

    c->llvmType() = llvm::StructType::get(
            module_->getLLVMModule()->getContext(), llvmTypes );

    // mark this class as done
    cycle_.erase(c);

    return true;
}

} // namespace swift
