#ifndef SWIFT_NODE_H
#define SWIFT_NODE_H

#include "utils/cast.h"
#include "utils/map.h"
#include "utils/stringhelper.h"

#include "fe/location.h"

namespace llvm {
    class LLVMContext;
    class Module;
}

namespace swift {

class Class;
class ClassMember;
class ClassVisitorBase;
class Context;
class Expr;
class ExprList;
class MemberFct;
class TypeNode;

//------------------------------------------------------------------------------

class Node
{
public:

    Node(const Location& loc, Node* parent = 0);
    virtual ~Node() {}

    const Location& loc() const;
    void setParent(Node* parent) { parent_ = parent; }

    template <class T> T* parent() { return cast<T>(parent_); }
    template <class T> const T* parent() const { return cast<T>(parent_); }

    template <class T> T* mustFind() 
    { 
        if ( T* t = dynamic<T>(parent_) )
            return t;
        if (parent_)
            return parent_->find<T>();
        else
            SWIFT_UNREACHABLE;
    }

    template <class T> T* find() 
    { 
        if ( T* t = dynamic<T>(parent_) )
            return t;
        if (parent_)
            return parent_->find<T>();
        return 0;
    }

protected:

    Location loc_;
    Node* parent_;
};

//------------------------------------------------------------------------------

class Module : public Node
{
public:

    Module(const Location& loc, std::string* id);
    virtual ~Module();

    void insert(Class* c); 
    Class* lookupClass(const std::string* id);
    const std::string* id() const;
    const char* cid() const;
    void analyze();
    void buildLLVMTypes();
    void declareFcts();
    void vectorizeFcts();
    void codeGen();
    void verify();
    llvm::Module* getLLVMModule() const;
    void accept(ClassVisitorBase* c);
    void llvmDump();

    typedef std::map<const std::string*, Class*, StringPtrCmp> ClassMap;
    const ClassMap& classes() const;

private:

    std::string* id_;
    ClassMap classes_;

public:

    llvm::LLVMContext* const lctxt_;

private:

    llvm::Module* llvmModule_;

public:

    Context* const ctxt_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_NODE_H
