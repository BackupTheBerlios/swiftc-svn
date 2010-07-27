#include "utils/llvmhelper.h"

#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Function.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/Support/TypeBuilder.h>
#include <llvm/Support/IRBuilder.h>

#include "utils/assert.h"
#include "utils/cast.h"
#include "utils/llvmplace.h"


using namespace llvm;

ConstantInt* createInt1(LLVMContext& lctxt, uint64_t val)
{
    return ConstantInt::get( IntegerType::getInt1Ty(lctxt), val );
}

ConstantInt* createInt8(LLVMContext& lctxt, uint64_t val)
{
    return ConstantInt::get( IntegerType::getInt8Ty(lctxt), val );
}

ConstantInt* createInt16(LLVMContext& lctxt, uint64_t val)
{
    return ConstantInt::get( IntegerType::getInt16Ty(lctxt), val );
}

ConstantInt* createInt32(LLVMContext& lctxt, uint64_t val)
{
    return ConstantInt::get( IntegerType::getInt32Ty(lctxt), val );
}

ConstantInt* createInt64(LLVMContext& lctxt, uint64_t val)
{
    return ConstantInt::get( IntegerType::getInt64Ty(lctxt), val );
}

const Type* createVoid(LLVMContext& lctxt)
{
    return TypeBuilder<void, true>::get(lctxt);
}

ConstantFP* createFloat(LLVMContext& lctxt, float val)
{
    return ConstantFP::get(lctxt, APFloat(val) );
}

ConstantFP* createDouble(LLVMContext& lctxt, double val)
{
    return ConstantFP::get(lctxt, APFloat(val) );
}

Value* createInBoundsGEP_0_x(LLVMContext& lctxt, LLVMBuilder& builder, 
                             Value* ptr, Value* x,
                             const std::string& name /*= ""*/)
{
    Value* input[2];
    input[0] = createInt64(lctxt, 0);
    input[1] = x;
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

Value* createInBoundsGEP_0_i32(LLVMContext& lctxt, LLVMBuilder& builder, 
                               Value* ptr, uint64_t i,
                               const std::string& name /*= ""*/)
{
    Value* input[2];
    input[0] = createInt64(lctxt, 0);
    input[1] = createInt32(lctxt, i);
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

Value* createInBoundsGEP_0_i64(LLVMContext& lctxt, LLVMBuilder& builder, 
                               Value* ptr, uint64_t i,
                               const std::string& name /*= ""*/)
{
    Value* input[2];
    input[0] = createInt64(lctxt, 0);
    input[1] = createInt64(lctxt, i);
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

Value* createLoadInBoundsGEP_x(LLVMContext& lctxt, LLVMBuilder& builder, 
                               Value* ptr, Value* x, 
                               const std::string& name /*= ""*/)
{
    return builder.CreateLoad(
            builder.CreateInBoundsGEP(ptr, x, name), name);
}

Value* createLoadInBoundsGEP_0_x(LLVMContext& lctxt, LLVMBuilder& builder, 
                                 Value* ptr, Value* x,
                                 const std::string& name /*= ""*/)
{
    return builder.CreateLoad( 
            createInBoundsGEP_0_x(lctxt, builder, ptr, x, name), name );
}

Value* createLoadInBoundsGEP_0_i32(LLVMContext& lctxt, LLVMBuilder& builder, 
                                   Value* ptr, uint64_t i,
                                   const std::string& name /*= ""*/)
{
    return builder.CreateLoad( 
        createInBoundsGEP_0_i32(lctxt, builder, ptr, i, name), name );
}

Value* createLoadInBoundsGEP_0_i64(LLVMContext& lctxt, LLVMBuilder& builder, 
                                   Value* ptr, uint64_t i,
                                   const std::string& name /*= ""*/)
{
    return builder.CreateLoad( 
        createInBoundsGEP_0_i64(lctxt, builder, ptr, i, name), name );
}

AllocaInst* createEntryAlloca(LLVMBuilder& builder, const Type* type, 
                              const std::string& name /*= ""*/)
{
    BasicBlock* entry = &builder.GetInsertBlock()->getParent()->getEntryBlock();
    LLVMBuilder tmpBuilder( entry, entry->begin() );

    return tmpBuilder.CreateAlloca(type, 0, name);
}

Value* simdExtract(Value* vVal, Value* mod, const Type* sType, LLVMBuilder& builder)
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
            Value* sElem = simdExtract(vElem, mod, sElemType, builder);
            sVal = builder.CreateInsertValue(sVal, sElem, memIdx);

            ++iter;
            ++memIdx;
        }

        return sVal;
    }
    else
        return builder.CreateExtractElement(vVal, mod);
}

Value* simdPack(Value* sVal, Value* vVal, Value* mod, LLVMBuilder& builder)
{
    if ( const StructType* vStruct = dyn_cast<StructType>(vVal->getType()) )
    {
        int memIdx = 0;
        StructType::element_iterator iter = vStruct->element_begin();
        while ( iter != vStruct->element_end() )
        {
            // get attributes
            Value* vElem = builder.CreateExtractValue(vVal, memIdx);
            Value* sElem = builder.CreateExtractValue(sVal, memIdx);

            // update
            vElem = simdPack(sElem, vElem, mod, builder);
            vVal = builder.CreateInsertValue(vVal, vElem, memIdx);

            // iterate
            ++iter;
            ++memIdx;
        }

        return vVal;
    }
    else
        return builder.CreateInsertElement(vVal, sVal, mod);
}

Value* simdBroadcast(Value* sVal, const Type* vType, LLVMBuilder& builder)
{
    LLVMContext& lctxt = sVal->getContext();
    const Type* sType = sVal->getType();

    Value* vVal = UndefValue::get(vType);

    if ( const StructType* sStruct = dyn_cast<StructType>(sType) )
    {
        const StructType* vStruct = ::cast<StructType>(vType);
        int memIdx = 0;
        StructType::element_iterator iter = sStruct->element_begin();
        while ( iter != sStruct->element_end() )
        {
            // get attributes
            Value* sElem = builder.CreateExtractValue(sVal, memIdx);

            // update
            Value* vElem = simdBroadcast( sElem, vStruct->getElementType(memIdx), builder );
            vVal = builder.CreateInsertValue(vVal, vElem, memIdx);

            // iterate
            ++iter;
            ++memIdx;
        }

        return vVal;
    }
    else
    {
        const VectorType* vecType = ::cast<VectorType>(vType);
        size_t numElems = vecType->getNumElements();

        // put sVal into the first element of vVal
        vVal = builder.CreateInsertElement( vVal, sVal, createInt32(lctxt, 0) );

        std::vector<Constant*> elems(numElems);
        for (size_t i = 0; i < numElems; ++i)
            elems[i] = createInt32(lctxt, 0); // select first element in all cases

        Value* vSelect = ConstantVector::get( VectorType::get( IntegerType::getInt32Ty(lctxt), numElems), elems );
        Value* vUndef = UndefValue::get(vecType);

        vVal = builder.CreateShuffleVector(vVal, vUndef, vSelect);

        return vVal;
    }
}
