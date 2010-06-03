#include "fe/llvmtypebuilder.h"

#include <llvm/Module.h>
#include <llvm/Support/TypeBuilder.h>

#include "utils/list.h"

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/type.h"

namespace swift {

LLVMTypebuilder::LLVMTypebuilder(Context* ctxt)
    : ctxt_(ctxt)
    , result_(true)
{
    typedef Module::ClassMap::const_iterator CIter;
    const Module::ClassMap& classes = ctxt_->module_->classes();

    for (CIter iter = classes.begin(); iter != classes.end(); ++iter)
    {
        Class* c = iter->second;

        // skip builtin types
        if ( ScalarType::isScalar(c->id()) )
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

    // refine all opaque types
    for (size_t i = 0; i < refinements.size(); ++i)
    {
        Refine& refine = refinements[i];
        refine.opaque_->refineAbstractTypeTo( refine.type_->getLLVMType(ctxt_->module_) );
    }
}

bool LLVMTypebuilder::getResult() const
{
    return result_;
}

bool LLVMTypebuilder::process(Class* c)
{
    Module* m = ctxt_->module_;

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

        llvm::OpaqueType* opaque;
        const UserType* missing;
        const llvm::Type* llvmType = type->defineLLVMType(opaque, missing, m);

        if (opaque)
        {
            swiftAssert(missing, "must be set");
            swiftAssert(llvmType, "must be set");
            refinements.push_back( Refine(opaque, missing) );
        }
        else if (!llvmType)
        {
            swiftAssert(missing, "must be set");

            if ( !process(missing->lookupClass(ctxt_->module_)) )
                return false;

            llvmType = type->getLLVMType(m);
        }

        llvmTypes.push_back(llvmType);
    }

    c->llvmType() = llvm::StructType::get(*m->llvmCtxt_, llvmTypes);

    // mark this class as done
    cycle_.erase(c);

    // add a name
    m->getLLVMModule()->addTypeName( c->cid(), c->llvmType() );

    return true;
}

} // namespace swift
