#include "utils/llvmhelper.h"

#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/Support/TypeBuilder.h>
#include <llvm/Support/IRBuilder.h>

llvm::Value* createInt8(llvm::LLVMContext& lctxt, uint64_t val /*= 0*/)
{
    return llvm::ConstantInt::get( llvm::IntegerType::getInt8Ty(lctxt), val );
}

llvm::Value* createInt16(llvm::LLVMContext& lctxt, uint64_t val /*= 0*/)
{
    return llvm::ConstantInt::get( llvm::IntegerType::getInt16Ty(lctxt), val );
}

llvm::Value* createInt32(llvm::LLVMContext& lctxt, uint64_t val /*= 0*/)
{
    return llvm::ConstantInt::get( llvm::IntegerType::getInt32Ty(lctxt), val );
}

llvm::Value* createInt64(llvm::LLVMContext& lctxt, uint64_t val /*= 0*/)
{
    return llvm::ConstantInt::get( llvm::IntegerType::getInt64Ty(lctxt), val );
}

const llvm::Type* createVoid(llvm::LLVMContext& lctxt)
{
    return llvm::TypeBuilder<void, true>::get(lctxt);
}

llvm::Value* createFloat(llvm::LLVMContext& lctxt, float val)
{
    return llvm::ConstantFP::get(lctxt, llvm::APFloat(val) );
}

llvm::Value* createDouble(llvm::LLVMContext& lctxt, double val)
{
    return llvm::ConstantFP::get(lctxt, llvm::APFloat(val) );
}

llvm::Value* createInBoundsGEP_0_x(llvm::LLVMContext& lctxt, 
                                  LLVMBuilder& builder, 
                                  llvm::Value* ptr,
                                  llvm::Value* x,
                                  const std::string& name /*= ""*/)
{
    llvm::Value* input[2];
    input[0] = createInt64(lctxt);
    input[1] = x;
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

llvm::Value* createInBoundsGEP_0_i32(llvm::LLVMContext& lctxt, 
                                     LLVMBuilder& builder, 
                                     llvm::Value* ptr,
                                     uint64_t i,
                                     const std::string& name /*= ""*/)
{
    llvm::Value* input[2];
    input[0] = createInt64(lctxt);
    input[1] = createInt32(lctxt, i);
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

llvm::Value* createInBoundsGEP_0_i64(llvm::LLVMContext& lctxt, 
                                     LLVMBuilder& builder, 
                                     llvm::Value* ptr,
                                     uint64_t i,
                                     const std::string& name /*= ""*/)
{
    llvm::Value* input[2];
    input[0] = createInt64(lctxt);
    input[1] = createInt64(lctxt, i);
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

llvm::Value* createLoadInBoundsGEP_0_x  (llvm::LLVMContext& lctxt, 
                                         LLVMBuilder& builder, 
                                         llvm::Value* ptr,
                                         llvm::Value* x,
                                         const std::string& name /*= ""*/)
{
    return builder.CreateLoad( 
            createInBoundsGEP_0_x(lctxt, builder, ptr, x, name) );
}

llvm::Value* createLoadInBoundsGEP_0_i32(llvm::LLVMContext& lctxt, 
                                         LLVMBuilder& builder, 
                                         llvm::Value* ptr,
                                         uint64_t i,
                                         const std::string& name /*= ""*/)
{
    return builder.CreateLoad( 
        createInBoundsGEP_0_i32(lctxt, builder, ptr, i, name) );
}

llvm::Value* createLoadInBoundsGEP_0_i64(llvm::LLVMContext& lctxt, 
                                         LLVMBuilder& builder, 
                                         llvm::Value* ptr,
                                         uint64_t i,
                                         const std::string& name /*= ""*/)
{
    return builder.CreateLoad( 
        createInBoundsGEP_0_i64(lctxt, builder, ptr, i, name) );
}
