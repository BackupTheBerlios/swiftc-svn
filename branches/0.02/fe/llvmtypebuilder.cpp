#include "fe/llvmtypebuilder.h"

#include <llvm/Module.h>
#include <llvm/Support/TypeBuilder.h>

#include "utils/cast.h"
#include "utils/list.h"

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/type.h"

using llvm::StructType;

namespace swift {

typedef Module::ClassMap::const_iterator CIter;

LLVMTypebuilder::LLVMTypebuilder(Context* ctxt)
    : ctxt_(ctxt)
{
    const Module::ClassMap& classes = ctxt_->module_->classes();
    Module* m = ctxt_->module_;
    llvm::Module* lm = ctxt_->lmodule();

    for (CIter iter = classes.begin(); iter != classes.end(); ++iter)
    {
        Class* c = iter->second;

        // skip builtin types
        if ( ScalarType::isScalar(c->id()) )
            continue;

        if ( buildTypeFromClass(c) )
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

            ctxt_->result_ = false;

            return;
        }
    }

    // refine all opaque types
    for (size_t i = 0; i < refinements.size(); ++i)
    {
        Refine& refine = refinements[i];
        refine.opaque_->refineAbstractTypeTo( refine.type_->getLLVMType(m) );
    }

    /*
     * now vectorize types
     */

    for (CIter iter = classes.begin(); iter != classes.end(); ++iter)
    {
        Class* c = iter->second;

        // skip builtin types
        if ( ScalarType::isScalar(c->id()) )
            continue;

        if ( !c->isSimd() )
            continue;

        const llvm::Type* vType = vec::vecType(
                lm, Context::SIMD_WIDTH, c->llvmType_, c->simdLength_);

        if (vType)
            c->vecType_ = cast<llvm::StructType>(vType);
        else
        {
            swiftAssert(c->simdLength_ == vec::ERROR, "must be an error");
            errorf( c->loc(), "class '%s' is not vectorizable", c->cid() );
            ctxt_->result_ = false;
        }
    }
}

bool LLVMTypebuilder::buildTypeFromClass(Class* c)
{
    Module* m = ctxt_->module_;
    bool simd = c->isSimd();

    // already processed?
    if ( c->llvmType_ )
        return true;

    typedef Class::MemberVars::const_iterator MVIter;

    if ( cycle_.contains(c) )
        return false; // pass-through error
    else
        cycle_.insert(c);

    LLVMTypes llvmTypes;
    for (size_t i = 0; i < c->memberVars().size(); ++i)
    {
        MemberVar* mv = c->memberVars()[i];
        //mv->type
        c->memberVars()[i]->index_ = i;
        const Type* type = c->memberVars()[i]->getType();

        llvm::OpaqueType* opaque;
        const UserType* missing;
        const llvm::Type* llvmType = type->defineLLVMType(opaque, missing, m);

        if (simd)
        {
            Class* inner = mv->getType()->cast<BaseType>()->lookupClass(m);

            if ( !inner->isSimd() )
            {
                errorf( c->loc(), "class '%s' is not declared as simd class", inner->cid() );
                errorf( c->loc(), "needed by class '%s'", c->cid() );

                ctxt_->result_ = false;
            }
        }

        if (opaque)
        {
            swiftAssert(missing, "must be set");
            swiftAssert(llvmType, "must be set");

            refinements.push_back( Refine(opaque, missing) );
        }
        else if (!llvmType)
        {
            swiftAssert(missing, "must be set");

            if ( !buildTypeFromClass(missing->lookupClass(m)) )
                return false;

            llvmType = type->getLLVMType(m);
        }

        llvmTypes.push_back(llvmType);
    }

    c->llvmType_ = llvm::StructType::get(*m->lctxt_, llvmTypes);

    // mark this class as done
    cycle_.erase(c);

    // add a name
    m->getLLVMModule()->addTypeName( c->cid(), c->llvmType_ );

    return true;
}

} // namespace swift
