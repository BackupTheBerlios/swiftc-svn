#ifndef UTILS_LLVM_HELPER_H
#define UTILS_LLVM_HELPER_H

#include <string>

#include <llvm/Value.h>

#include "utils/types.h"

//----------------------------------------------------------------------

namespace llvm {

    class AllocaInst;
    class LLVMContext;
    class Type;
    class Value;

    class ConstantFolder;
    template <bool preserveNames> class IRBuilderDefaultInserter;
    template<bool preserveNames, typename T, typename Inserter> class IRBuilder;
}

//----------------------------------------------------------------------

typedef llvm::IRBuilder <true, llvm::ConstantFolder, 
        llvm::IRBuilderDefaultInserter<true> > LLVMBuilder;

//----------------------------------------------------------------------

llvm::Value* createInt8 (llvm::LLVMContext& lctxt, uint64_t val = 0);
llvm::Value* createInt16(llvm::LLVMContext& lctxt, uint64_t val = 0);
llvm::Value* createInt32(llvm::LLVMContext& lctxt, uint64_t val = 0);
llvm::Value* createInt64(llvm::LLVMContext& lctxt, uint64_t val = 0);
llvm::Value* createFloat(llvm::LLVMContext& lctxt, float val);
llvm::Value* createDouble(llvm::LLVMContext& lctxt, double val);

//----------------------------------------------------------------------

const llvm::Type* createVoid(llvm::LLVMContext& lctxt);

//----------------------------------------------------------------------

llvm::Value* createInBoundsGEP_0_x  (llvm::LLVMContext& lctxt, LLVMBuilder& builder, llvm::Value* ptr, llvm::Value* x, const std::string& name = "");
llvm::Value* createInBoundsGEP_0_i32(llvm::LLVMContext& lctxt, LLVMBuilder& builder, llvm::Value* ptr, uint64_t i, const std::string& name = "");
llvm::Value* createInBoundsGEP_0_i64(llvm::LLVMContext& lctxt, LLVMBuilder& builder, llvm::Value* ptr, uint64_t i, const std::string& name = "");

llvm::Value* createLoadInBoundsGEP_x    (llvm::LLVMContext& lctxt, LLVMBuilder& builder, llvm::Value* ptr, llvm::Value* x, const std::string& name = "");
llvm::Value* createLoadInBoundsGEP_0_x  (llvm::LLVMContext& lctxt, LLVMBuilder& builder, llvm::Value* ptr, llvm::Value* x, const std::string& name = "");
llvm::Value* createLoadInBoundsGEP_0_i32(llvm::LLVMContext& lctxt, LLVMBuilder& builder, llvm::Value* ptr, uint64_t i, const std::string& name = "");
llvm::Value* createLoadInBoundsGEP_0_i64(llvm::LLVMContext& lctxt, LLVMBuilder& builder, llvm::Value* ptr, uint64_t i, const std::string& name = "");

//----------------------------------------------------------------------

llvm::AllocaInst* createEntryAlloca(LLVMBuilder& builder, const llvm::Type* type, const std::string& name = "");

//----------------------------------------------------------------------


#endif // UTILS_LLVM_HELPER_H
