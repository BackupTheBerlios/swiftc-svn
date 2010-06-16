#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Support/TypeBuilder.h>

#include "utils/llvmhelper.h"

llvm::Value* createInt32(llvm::LLVMContext& lc, uint64_t val /*= 0*/)
{
    return llvm::ConstantInt::get( llvm::IntegerType::getInt32Ty(lc), val );
}

llvm::Value* createInt64(llvm::LLVMContext& lc, uint64_t val /*= 0*/)
{
    return llvm::ConstantInt::get( llvm::IntegerType::getInt64Ty(lc), val );
}

const llvm::Type* createVoid(llvm::LLVMContext& lc)
{
    return llvm::TypeBuilder<void, true>::get(lc);
}

llvm::Value* createInBoundsGEP_0_x(llvm::LLVMContext& lc, 
                                  llvm::IRBuilder<>& builder, 
                                  llvm::Value* ptr,
                                  llvm::Value* x,
                                  const llvm::Twine& name /*= ""*/)
{
    llvm::Value* input[2];
    input[0] = createInt64(lc);
    input[1] = x;
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

llvm::Value* createInBoundsGEP_0_i32(llvm::LLVMContext& lc, 
                                     llvm::IRBuilder<>& builder, 
                                     llvm::Value* ptr,
                                     uint64_t i,
                                     const llvm::Twine& name /*= ""*/)
{
    llvm::Value* input[2];
    input[0] = createInt64(lc);
    input[1] = createInt32(lc, i);
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

llvm::Value* createInBoundsGEP_0_i64(llvm::LLVMContext& lc, 
                                     llvm::IRBuilder<>& builder, 
                                     llvm::Value* ptr,
                                     uint64_t i,
                                     const llvm::Twine& name /*= ""*/)
{
    llvm::Value* input[2];
    input[0] = createInt64(lc);
    input[1] = createInt64(lc, i);
    return builder.CreateInBoundsGEP(ptr, input, input+2, name);
}

llvm::Value* createLoadInBoundsGEP_0_x  (llvm::LLVMContext& lc, 
                                         llvm::IRBuilder<>& builder, 
                                         llvm::Value* ptr,
                                         llvm::Value* x,
                                         const llvm::Twine& name /*= ""*/)
{
    return builder.CreateLoad( 
            createInBoundsGEP_0_x(lc, builder, ptr, x, name) );
}

llvm::Value* createLoadInBoundsGEP_0_i32(llvm::LLVMContext& lc, 
                                         llvm::IRBuilder<>& builder, 
                                         llvm::Value* ptr,
                                         uint64_t i,
                                         const llvm::Twine& name /*= ""*/)
{
    return builder.CreateLoad( 
        createInBoundsGEP_0_i32(lc, builder, ptr, i, name) );
}

llvm::Value* createLoadInBoundsGEP_0_i64(llvm::LLVMContext& lc, 
                                         llvm::IRBuilder<>& builder, 
                                         llvm::Value* ptr,
                                         uint64_t i,
                                         const llvm::Twine& name /*= ""*/)
{
    return builder.CreateLoad( 
        createInBoundsGEP_0_i64(lc, builder, ptr, i, name) );
}
