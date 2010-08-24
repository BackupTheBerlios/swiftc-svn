/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef SWIFT_CLASS_H
#define SWIFT_CLASS_H

#include <map>
#include <set>
#include <vector>

#include "fe/auto.h"
#include "fe/location.hh"
#include "fe/var.h"
#include "fe/sig.h"

namespace llvm {
    class BasicBlock;
    class Function;
    class StructType;
    class Type;
}

namespace swift {

class ClassVisitorBase;
class Context;
class InOut;
class MemberVar;
class RetVal;
class Scope;
class Type;

//------------------------------------------------------------------------------

class Class : public Def
{
public:

    Class(location loc, bool simd, std::string* id);
    virtual ~Class();

    virtual void accept(ClassVisitorBase* m);
    const std::string* id() const;
    const char* cid() const;
    void insert(Context* ctxt, MemberVar* m);
    void insert(Context* ctxt, MemberFct* m);
    MemberFct* lookupMemberFct(Module* module, const std::string* id, const TypeList&) const;
    MemberVar* lookupMemberVar(const std::string* id) const;
    void addAssignCreate(Context* ctxt);

    typedef std::vector<MemberVar*> MemberVars;
    typedef std::vector<MemberFct*> MemberFcts;

    const MemberVars& memberVars() const;
    const MemberFcts& memberFcts() const;

    enum Impl
    {
        NONE,
        USER,
        DEFAULT,
        NOT_ANALYZED
    };

    Impl getCopyCreate() const;
    Impl getDefaultCreate() const;
    Impl getAssign() const;

    bool isSimd() const;
    const llvm::StructType* getLLVMType() const;
    const llvm::StructType* getVecType() const;
    int getSimdLength() const;

protected:

    bool simd_;

    typedef std::     map<const std::string*, MemberVar*, StringPtrCmp> MemberVarMap;
    typedef std::multimap<const std::string*, MemberFct*, StringPtrCmp> MemberFctMap;

    MemberVars memberVars_;
    MemberFcts memberFcts_;
    MemberFctMap memberFctMap_;
    MemberVarMap memberVarMap_;         

private:

    void setLLVMType(const llvm::Type* llvmType);
    void setVecType(const llvm::Type* vecType);

    Impl copyCreate_;
    Impl defaultCreate_;
    Impl copyAssign_;

    const llvm::StructType* llvmType_;
    const llvm::StructType* vecType_;
    int simdLength_;

    friend class LLVMTypebuilder;
};

//------------------------------------------------------------------------------

class ClassMember : public Node
{
public:

    ClassMember(location loc, std::string* id);
    virtual ~ClassMember();

    virtual void accept(ClassVisitorBase* c) = 0;
    const std::string* id() const;
    const char* cid() const;
    std::string getLLVMName() const;
    void setLLVMName(const std::string& name);

private:

    std::string* id_;
    std::string llvmName_;

    template<class T> friend class ClassVisitor;
};

//------------------------------------------------------------------------------

class MemberFct : public ClassMember
{
public:

    MemberFct(location loc, bool simd, std::string* id, Scope* scope);
    virtual ~MemberFct();

    virtual const char* qualifierStr() const = 0;
    virtual bool isAutoGenerated() const = 0;

    bool isEmpty() const;
    bool isSimd() const;

    Sig& sig() { return sig_; }
    llvm::Function* llvmFct() { return llvmFct_; }
    llvm::Function* simdFct() { return simdFct_; }
    llvm::BasicBlock* returnBB() { return returnBB_; }
    Scope* scope() { return scope_; }

protected:

    bool simd_;
    Scope* scope_;
    std::vector<const llvm::Type*> params_;
    std::vector<InOut*> realIn_;
    std::vector<RetVal*> realOut_;
    bool main_;
    Sig sig_;
    llvm::Function* llvmFct_;
    llvm::Function* simdFct_;
    const llvm::Type* retType_; 
    llvm::AllocaInst* retAlloca_;
    llvm::BasicBlock* returnBB_;

    template<class T> friend class ClassVisitor;
    friend class LLVMFctDeclarer;
};

//------------------------------------------------------------------------------

class Method : public MemberFct
{
public:

    Method(location loc, bool simd, std::string* id, Scope* scope);

    virtual TokenType getModifier() const = 0;
    virtual bool isAutoGenerated() const;

    llvm::Value* getSelfValue() const;

    bool isAutoAssign() const;

private:

    llvm::Value* selfValue_;

    bool isAutoAssign_;

    friend void Class::addAssignCreate(Context* ctxt);
    template<class T> friend class ClassVisitor;
};

//------------------------------------------------------------------------------

class Create : public Method
{
public:

    Create(location loc, bool simd, Scope* scope);

    virtual void accept(ClassVisitorBase* c);
    virtual bool isAutoGenerated() const;
    virtual const char* qualifierStr() const;
    virtual TokenType getModifier() const;

    bool isAutoCopy() const;
    bool isAutoDefault() const;

private:

    bool isAutoCopy_;
    bool isAutoDefault_;

    friend class Class;
    template<class T> friend class ClassVisitor;
};

//------------------------------------------------------------------------------

class Reader : public Method
{
public:

    Reader(location loc, bool simd, std::string* id, Scope* scope);

    virtual void accept(ClassVisitorBase* c);
    virtual TokenType getModifier() const;
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class Writer : public Method
{
public:

    Writer(location loc, bool simd, std::string* id, Scope* scope);

    virtual void accept(ClassVisitorBase* c);
    virtual TokenType getModifier() const;
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class Routine : public MemberFct
{
public:

    Routine(location loc, bool simd, std::string *id, Scope* scope);

    virtual void accept(ClassVisitorBase* c);
    virtual const char* qualifierStr() const;
    virtual bool isAutoGenerated() const;
};

//------------------------------------------------------------------------------

class MemberVar : public ClassMember
{
public:

    MemberVar(location loc, Type* type, std::string* id);
    virtual ~MemberVar();

    virtual void accept(ClassVisitorBase* c);
    const Type* getType() const;
    int getIndex() const;

private:

    Type* type_;
    int index_;

    template<class T> friend class ClassVisitor;
    friend class LLVMTypebuilder;
};

//------------------------------------------------------------------------------

class ClassVisitorBase
{
public:

    ClassVisitorBase(Context* ctxt);
    virtual ~ClassVisitorBase() {}

    // ClassMember -> MemberFct -> Method
    virtual void visit(Create* c) = 0;
    virtual void visit(Reader* r) = 0;
    virtual void visit(Writer* w) = 0;

    // ClassMember -> MemberFct -> StaticMethod
    virtual void visit(Routine* r) = 0;

    // ClassMember -> MemberVar
    virtual void visit(MemberVar* m) = 0;

    friend void Class  ::accept(ClassVisitorBase* m);
    friend void Create ::accept(ClassVisitorBase* m);
    friend void Reader ::accept(ClassVisitorBase* m);
    friend void Writer ::accept(ClassVisitorBase* m);
    friend void Routine::accept(ClassVisitorBase* m);

protected:

    Context* ctxt_;
};

//------------------------------------------------------------------------------

template<class T> class ClassVisitor; 

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_CLASS_H
