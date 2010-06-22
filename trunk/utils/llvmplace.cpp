#include "utils/llvmplace.h"

#include <llvm/Support/IRBuilder.h>

#include "fe/llvmtypebuilder.h"

using namespace llvm;

//----------------------------------------------------------------------

Value* Scalar::getScalar(LLVMBuilder& builder) const
{
    return val_;
}

Value* Scalar::getAddr(LLVMBuilder& builder) const
{
    AllocaInst* alloca = createEntryAlloca( builder, val_->getType(), val_->getName() );
    builder.CreateStore(val_, alloca);

    return alloca;
}

void Scalar::writeBack(LLVMBuilder& builder) const
{
}

//----------------------------------------------------------------------

Value* Addr::getScalar(LLVMBuilder& builder) const
{
    return builder.CreateLoad(val_, val_->getName() );
}

Value* Addr::getAddr(LLVMBuilder& builder) const
{
    return val_;
}

void Addr::writeBack(LLVMBuilder& builder) const
{
}

//----------------------------------------------------------------------

Value* SimdAddr::getScalar(LLVMBuilder& builder) const
{
    return builder.CreateLoad(val_, val_->getName() );
}

Value* SimdAddr::getAddr(LLVMBuilder& builder) const
{
    return val_;
}

void SimdAddr::writeBack(LLVMBuilder& builder) const
{
}

void SimdAddr::extract(llvm::LLVMContext& lctxt, LLVMBuilder& builder)
{
    //Value* slv = createInt64(lctxt, simdLength_);
    //Value* div = builder.CreateUDiv(idx, slv);
    //Value* rem = builder.CreateURem(idx, slv);
    //Value* mod = builder.CreateTrunc( rem , llvm::IntegerType::getInt32Ty(lctxt) );
    //Value* aElem = createLoadInBoundsGEP_x(lctxt, builder, val_, div);

    //if ( const llvm::structtype* vecstruct = 
            //dynamic_cast<const llvm::structtype*>(aelem->gettype()) )
    //{
        //const usertype* ut = inner->cast<usertype>();
        //const llvm::type* scalarstruct = ut->lookupclass(ctxt_->module_)->getllvmtype();

        //typedef llvm::structtype::element_iterator eiter;
        //int memidx = 0;
        //value* res = llvm::undefvalue::get(scalarstruct);
        //for (eiter iter = vecstruct->element_begin(); iter != vecstruct->element_end(); ++iter)
        //{
            //value* velem = builder.CreateExtractValue(aElem, memIdx);
            //value*  elem = builder.CreateExtractElement(vElem, mod);
            //res = builder.createInsertValue(res, elem, memIdx);

            //++memIdx;
        //}

        ////setresult( new scalar(res) );
    //}
}
