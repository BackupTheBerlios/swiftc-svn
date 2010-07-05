#include "utils/llvmplace.h"

#include <iostream>

#include <llvm/Support/IRBuilder.h>

#include "utils/cast.h"

#include "fe/llvmtypebuilder.h"

using namespace llvm;

//----------------------------------------------------------------------

Scalar* Scalar::clone() const
{
    return new Scalar(val_);
}

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

Addr* Addr::clone() const
{
    return new Addr(val_);
}

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

SimdAddr* SimdAddr::clone() const
{
    return new SimdAddr(*this);
}

SimdAddr::SimdAddr(Value* ptr, Value* mod, const Type* sType, LLVMBuilder& builder)
    : Addr(ptr)
    , mod_(mod)
{
    Value* vVal = builder.CreateLoad( ptr, ptr->getName() );

    Value* sVal = simdExtract(vVal, mod_, sType, builder);

    alloca_ = createEntryAlloca(builder, sType, ptr->getNameStr() + ".tmp" );
    builder.CreateStore(sVal, alloca_);
}

Value* SimdAddr::getScalar(LLVMBuilder& builder) const
{
    return builder.CreateLoad(alloca_);
}

Value* SimdAddr::getAddr(LLVMBuilder& builder) const
{
    return alloca_;
}

void SimdAddr::writeBack(LLVMBuilder& builder) const
{
    Value* vVal = builder.CreateLoad(val_);
    Value* sVal = builder.CreateLoad(alloca_);
    Value* vValNew = simdPack(sVal, vVal, mod_, builder);
    builder.CreateStore(vValNew, val_);
}
