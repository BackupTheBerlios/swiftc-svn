#ifndef SWIFT_LLVM_PLACE_H
#define SWIFT_LLVM_PLACE_H

#include "utils/llvmhelper.h"

//----------------------------------------------------------------------

class Place
{
public:

    Place(llvm::Value* val)
        : val_(val)
    {}
    virtual ~Place() {}

    virtual llvm::Value* getScalar(LLVMBuilder& builder) const = 0;
    virtual llvm::Value* getAddr(LLVMBuilder& builder) const = 0;
    virtual void writeBack(LLVMBuilder& builder) const = 0;

protected:

    llvm::Value* val_;
};

//----------------------------------------------------------------------

class Scalar : public Place
{
public:

    Scalar(llvm::Value* scalar)
        : Place(scalar)
    {}
    virtual ~Scalar() {}

    virtual llvm::Value* getScalar(LLVMBuilder& builder) const;
    virtual llvm::Value* getAddr(LLVMBuilder& builder) const;
    virtual void writeBack(LLVMBuilder& builder) const;
};

//----------------------------------------------------------------------

class Addr : public Place
{
public:

    Addr(llvm::Value* ptr)
        : Place(ptr)
    {}
    virtual ~Addr() {}

    virtual llvm::Value* getScalar(LLVMBuilder& builder) const;
    virtual llvm::Value* getAddr(LLVMBuilder& builder) const;
    virtual void writeBack(LLVMBuilder& builder) const;
};

//----------------------------------------------------------------------

//class SimdAddr : public Addr
//{
//public:

    /** 
     * @param ptr Pointer to the vector element.
     * @param agg The temporary extracted scalar value.
     * @param mod The index within the vector element.
     */
    //SimdAddr(llvm::Value* ptr, llvm::Value* agg, llvm::Value* mod)
        //: Addr(ptr)
        //, mod_(mod)
    //{}
    //virtual ~SimdAddr() {}

    //virtual llvm::Value* load(LLVMBuilder& builder, const std::string& name) const;
    //virtual void store(LLVMBuilder& builder, llvm::Value value, const std::string& name) const;
    //virtual llvm::Value* getScalar(LLVMBuilder& builder) const;
    //virtual llvm::Value* getAddr(LLVMBuilder& builder) const;
    //virtual void writeBack(LLVMBuilder& builder) const;

//protected:

    //llvm::Value* mod_;
    //llvm::Value* agg_;
//};

//----------------------------------------------------------------------

#endif // SWIFT_LLVM_PLACE_H
