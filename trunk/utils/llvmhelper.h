#ifndef UTILS_LLVM_HELPER_H
#define UTILS_LLVM_HELPER_H

#include <string>
#include <vector>

#include <llvm/Value.h>

#include "utils/types.h"

//----------------------------------------------------------------------

namespace llvm {
    class AllocaInst;
    class ConstantInt;
    class ConstantFP;
    class LLVMContext;
    class Type;
    class Value;

    class ConstantFolder;
    template <bool preserveNames> class IRBuilderDefaultInserter;
    template <bool preserveNames, typename T, typename Inserter> class IRBuilder;
}

typedef std::vector<llvm::Value*> Values;
typedef std::vector<const llvm::Type*> LLVMTypes;

typedef llvm::IRBuilder <true, llvm::ConstantFolder, llvm::IRBuilderDefaultInserter<true> > LLVMBuilder;

class Place;

//----------------------------------------------------------------------

llvm::ConstantInt* createInt8 (llvm::LLVMContext& lctxt, uint64_t val);
llvm::ConstantInt* createInt16(llvm::LLVMContext& lctxt, uint64_t val);
llvm::ConstantInt* createInt32(llvm::LLVMContext& lctxt, uint64_t val);
llvm::ConstantInt* createInt64(llvm::LLVMContext& lctxt, uint64_t val);
llvm::ConstantFP*  createFloat(llvm::LLVMContext& lctxt, float val);
llvm::ConstantFP*  createDouble(llvm::LLVMContext& lctxt, double val);

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

llvm::Value* simdExtract(llvm::Value* vVec, llvm::Value* mod, const llvm::Type* scalarType, LLVMBuilder& builder);
llvm::Value* simdPack(llvm::Value* sVal, llvm::Value* vVal, llvm::Value* mod, LLVMBuilder& builder);
llvm::Value* simdBroadcast(llvm::Value* sVal, const llvm::Type* vType, LLVMBuilder& builder);

//----------------------------------------------------------------------

#endif // UTILS_LLVM_HELPER_H
