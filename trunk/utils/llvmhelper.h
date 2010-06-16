#ifndef UTILS_LLVM_HELPER_H
#define UTILS_LLVM_HELPER_H

#include <llvm/Support/IRBuilder.h>

namespace llvm {
    class LLVMContext;
    class Value;
    class Twine;
}

llvm::Value* createInt32(llvm::LLVMContext& lc, uint64_t val = 0);
llvm::Value* createInt64(llvm::LLVMContext& lc, uint64_t val = 0);

const llvm::Type* createVoid(llvm::LLVMContext& lc);

llvm::Value* createInBoundsGEP_0_x  (llvm::LLVMContext& lc, 
                                     llvm::IRBuilder<>& builder, 
                                     llvm::Value* ptr,
                                     llvm::Value* x,
                                     const llvm::Twine &Name = "");
llvm::Value* createInBoundsGEP_0_i32(llvm::LLVMContext& lc, 
                                     llvm::IRBuilder<>& builder, 
                                     llvm::Value* ptr,
                                     uint64_t i,
                                     const llvm::Twine &Name = "");
llvm::Value* createInBoundsGEP_0_i64(llvm::LLVMContext& lc, 
                                     llvm::IRBuilder<>& builder, 
                                     llvm::Value* ptr,
                                     uint64_t i,
                                     const llvm::Twine &Name = "");

llvm::Value* createLoadInBoundsGEP_0_x  (llvm::LLVMContext& lc, 
                                         llvm::IRBuilder<>& builder, 
                                         llvm::Value* ptr,
                                         llvm::Value* x,
                                         const llvm::Twine &Name = "");
llvm::Value* createLoadInBoundsGEP_0_i32(llvm::LLVMContext& lc, 
                                         llvm::IRBuilder<>& builder, 
                                         llvm::Value* ptr,
                                         uint64_t i,
                                         const llvm::Twine &Name = "");
llvm::Value* createLoadInBoundsGEP_0_i64(llvm::LLVMContext& lc, 
                                         llvm::IRBuilder<>& builder, 
                                         llvm::Value* ptr,
                                         uint64_t i,
                                         const llvm::Twine &Name = "");

#endif // UTILS_LLVM_HELPER_H
