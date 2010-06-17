#ifndef UTILS_LLVM_HELPER_H
#define UTILS_LLVM_HELPER_H

#include <string>

#include <llvm/Value.h>

#include "utils/types.h"

//----------------------------------------------------------------------

namespace llvm {

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

class Addr 
{
public:

    Addr(llvm::Value* ptr)
        : ptr_(ptr)
    {}
    virtual ~Addr() {}

    virtual llvm::Value* load(LLVMBuilder& builder, const std::string& name);
    virtual void store(LLVMBuilder& builder, llvm::Value value, const std::string& name);

protected:

    llvm::Value* ptr_;
};

class SimdAddr : public Addr
{
public:

    SimdAddr(llvm::Value* ptr, llvm::Value* mod)
        : Addr(ptr)
        , mod_(mod)
    {}
    virtual ~SimdAddr() {}

    virtual llvm::Value* load(LLVMBuilder& builder, const std::string& name);
    virtual void store(LLVMBuilder& builder, llvm::Value value, const std::string& name);

protected:

    llvm::Value* mod_;

};

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

#endif // UTILS_LLVM_HELPER_H
