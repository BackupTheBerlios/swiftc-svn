#include "utils/llvmplace.h"

#include <iostream>

#include <llvm/Support/IRBuilder.h>

#include "utils/cast.h"

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

SimdAddr::SimdAddr(Value* ptr, Value* mod, const Type* sType, LLVMBuilder& builder)
    : Addr(ptr)
    , mod_(mod)
{
    Value* vVal = builder.CreateLoad( ptr, ptr->getName() );

    Value* sVal = extract(vVal, sType, builder);

    alloca_ = createEntryAlloca(builder, sType, ptr->getNameStr() + ".tmp" );
    builder.CreateStore(sVal, alloca_);
}

Value* SimdAddr::extract(Value* vVal, const Type* sType, LLVMBuilder& builder)
{
    if ( const StructType* vStruct = dynamic<StructType>(vVal->getType()) )
    {
        const StructType* sStruct = cast<StructType>(sType);
        Value* sVal = UndefValue::get(sStruct);

        int memIdx = 0;
        StructType::element_iterator iter = vStruct->element_begin();
        while ( iter != vStruct->element_end() )
        {
            const Type* sElemType = (sStruct->element_begin() + memIdx)->get();

            Value* vElem = builder.CreateExtractValue(vVal, memIdx);
            Value* sElem = extract(vElem, sElemType, builder);
            sVal = builder.CreateInsertValue(sVal, sElem, memIdx);

            ++iter;
            ++memIdx;
        }

        return sVal;
    }
    else
        return builder.CreateExtractElement(vVal, mod_);
}

Value* SimdAddr::pack(Value* sVal, Value* vVal, const Type* sType, LLVMBuilder& builder) const
{
    if ( const StructType* vStruct = dyn_cast<StructType>(vVal->getType()) )
    {
        const StructType* sStruct = cast<StructType>(sType);
        //Value* sVal = UndefValue::get(sStruct);

        int memIdx = 0;
        StructType::element_iterator iter = vStruct->element_begin();
        while ( iter != vStruct->element_end() )
        {
            const Type* sElemType = (sStruct->element_begin() + memIdx)->get();

            // get attributes
            Value* vElem = builder.CreateExtractValue(vVal, memIdx, "hans");
            Value* sElem = builder.CreateExtractValue(sVal, memIdx, "pete");

            // update
            vElem = pack(sElem, vElem, sElemType, builder);
            vVal = builder.CreateInsertValue(vVal, vElem, memIdx);

            // iterate
            ++iter;
            ++memIdx;
        }

        return vVal;
    }
    else
        return builder.CreateInsertElement(vVal, sVal, mod_);
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
    Value* vValNew = pack(sVal, vVal, sVal->getType(), builder);
    builder.CreateStore(vValNew, val_);
}
