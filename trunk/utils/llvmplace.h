#ifndef SWIFT_LLVM_PLACE_H
#define SWIFT_LLVM_PLACE_H

#include <vector>

#include "utils/llvmhelper.h"

//----------------------------------------------------------------------

class Place
{
public:

    Place(llvm::Value* val)
        : val_(val)
    {}
    virtual ~Place() {}

    //virtual Place* clone() const = 0;

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

    //Scalar* clone() const;

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

    //Addr* clone() const;

    virtual llvm::Value* getScalar(LLVMBuilder& builder) const;
    virtual llvm::Value* getAddr(LLVMBuilder& builder) const;
    virtual void writeBack(LLVMBuilder& builder) const;
};

//----------------------------------------------------------------------

class SimdAddr : public Addr
{
public:

    SimdAddr(llvm::Value* ptr, 
             llvm::Value* mod, 
             const llvm::Type* scalarType,
             LLVMBuilder& builder);
    virtual ~SimdAddr() {}

    //SimdAddr* clone() const;

    virtual llvm::Value* getScalar(LLVMBuilder& builder) const;
    virtual llvm::Value* getAddr(LLVMBuilder& builder) const;
    virtual void writeBack(LLVMBuilder& builder) const;

protected:

    llvm::Value* mod_;
    llvm::Value* alloca_;
};

//----------------------------------------------------------------------

typedef std::vector<Place*> Places;

//----------------------------------------------------------------------

#endif // SWIFT_LLVM_PLACE_H
