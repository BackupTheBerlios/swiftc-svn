#include "fe/llvmtypebuilder.h"

#include <llvm/Module.h>
#include <llvm/Support/TypeBuilder.h>

#include "utils/list.h"

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/type.h"

namespace swift {

typedef Module::ClassMap::const_iterator CIter;

LLVMTypebuilder::LLVMTypebuilder(Context* ctxt)
    : ctxt_(ctxt)
    , result_(true)
{
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

    /*
     * collect structs which should be vectorized
     * and build up reverse map
     */

    for (CIter iter = classes.begin(); iter != classes.end(); ++iter)
    {
        Class* c = iter->second;

        // skip builtin types
        if ( ScalarType::isScalar(c->id()) )
            continue;

        struct2Class_[ c->llvmType_ ] = c;

        if ( c->isSimd() )
            vecStructs_[ c->llvmType_ ] = vec::VecType();
    }

    // vec types
    vec::TypeVectorizer typeVec( this, vecStructs_, ctxt_->module_->getLLVMModule() );

    /*
     * fill entries in class
     */

    for (CIter iter = classes.begin(); iter != classes.end(); ++iter)
    {
        Class* c = iter->second;

        // skip builtin types
        if ( ScalarType::isScalar(c->id()) )
            continue;

        if ( c->isSimd() )
        {
            struct2Class_[ c->llvmType_ ] = c;
            vec::VecType& vec = vecStructs_[c->llvmType_];

            c->vecType_ = vec.struct_;
            c->simdLength_ = vec.simdLength_;
        }
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

    c->llvmType_ = llvm::StructType::get(*m->llvmCtxt_, llvmTypes);

    // mark this class as done
    cycle_.erase(c);

    // add a name
    m->getLLVMModule()->addTypeName( c->cid(), c->llvmType_ );

    return true;
}

void LLVMTypebuilder::notInMap(const llvm::StructType* st, const llvm::StructType* parent) const
{
    {
        Class* c = struct2Class_.find(st)->second;
        errorf( c->loc(), "class '%s' is not declared as simd class", c->cid() );
    }

    {
        Class* c = struct2Class_.find(parent)->second;
        errorf( c->loc(), "needed by class '%s'", c->cid() );
    }

    ctxt_->result_ = false;
}

void LLVMTypebuilder::notVectorizable(const llvm::StructType* st) const
{
    Class* c = struct2Class_.find(st)->second;
    errorf( c->loc(), "class '%s' is not vectorizable", c->cid() );

    ctxt_->result_ = false;
}

} // namespace swift
