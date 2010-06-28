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

Value* createInt8(LLVMContext& lctxt, uint64_t val /*= 0*/)
{
    return ConstantInt::get( IntegerType::getInt8Ty(lctxt), val );
}

Value* createInt16(LLVMContext& lctxt, uint64_t val /*= 0*/)
{
    return ConstantInt::get( IntegerType::getInt16Ty(lctxt), val );
}

Value* createInt32(LLVMContext& lctxt, uint64_t val /*= 0*/)
{
    return ConstantInt::get( IntegerType::getInt32Ty(lctxt), val );
}

Value* createInt64(LLVMContext& lctxt, uint64_t val /*= 0*/)
{
    return ConstantInt::get( IntegerType::getInt64Ty(lctxt), val );
}

const Type* createVoid(LLVMContext& lctxt)
{
    return TypeBuilder<void, true>::get(lctxt);
}

Value* createFloat(LLVMContext& lctxt, float val)
{
    return ConstantFP::get(lctxt, APFloat(val) );
}

Value* createDouble(LLVMContext& lctxt, double val)
{
    return ConstantFP::get(lctxt, APFloat(val) );
}

Value* createInBoundsGEP_0_x(LLVMContext& lctxt, LLVMBuilder& builder, 
                             Value* ptr, Value* x,
                             const std::string& name /*= ""*/)
{
    Value* input[2];
    input[0] = createInt64(lctxt);
    input[1] = x;
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

Value* createInBoundsGEP_0_i32(LLVMContext& lctxt, LLVMBuilder& builder, 
                               Value* ptr, uint64_t i,
                               const std::string& name /*= ""*/)
{
    Value* input[2];
    input[0] = createInt64(lctxt);
    input[1] = createInt32(lctxt, i);
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

Value* createInBoundsGEP_0_i64(LLVMContext& lctxt, LLVMBuilder& builder, 
                               Value* ptr, uint64_t i,
                               const std::string& name /*= ""*/)
{
    Value* input[2];
    input[0] = createInt64(lctxt);
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

//void createCopy(LLVMBuilder& builder, Place* src, Value* dst)
//{
    //if ( Addr* addr = dynamic<Addr>(src) )
        //createCopy(builder, addr->getAddr(builder), dst);
    //else
        //builder.CreateStore( src->getScalar(builder), dst );
//}

//void createCopy(LLVMBuilder& builder, Value* src, Value* dst)
//{
    //swiftAssert(src->getType() == dst->getType(), "types must match");
    //LLVMContext& lctxt = src->getContext();

    //const PointerType* ptrType = cast<PointerType>( src->getType() );
    //const Type* type = ptrType->getContainedType(0);
    
    //if ( const StructType* st = dynamic<StructType>(type) )
    //{
        //size_t i = 0;
        //StructType::element_iterator iter = st->element_begin();
        //while (iter != st->element_end())
        //{
            //Value* srcMemPtr = createInBoundsGEP_0_i32(lctxt, builder, src, i);
            //Value* dstMemPtr = createInBoundsGEP_0_i32(lctxt, builder, dst, i);
            //createCopy(builder, srcMemPtr, dstMemPtr);

            //++iter;
            //++i;
        //}
    //}
    //else
        //builder.CreateStore( builder.CreateLoad(src), dst );
//}
