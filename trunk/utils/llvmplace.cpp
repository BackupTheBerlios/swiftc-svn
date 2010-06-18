#include "utils/llvmplace.h"

#include <llvm/Support/IRBuilder.h>

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

