#ifndef SWIFT_NODE_H
#define SWIFT_NODE_H

#include "utils/map.h"
#include "utils/stringhelper.h"

#include "fe/location.hh"

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

    Node(location loc);
    virtual ~Node() {}

    const location& loc() const;

protected:

    location loc_;
};

//------------------------------------------------------------------------------

class Def : public Node
{
public:

    Def(location loc, std::string* id);
    virtual ~Def();

protected:

    std::string* id_;
};

//------------------------------------------------------------------------------

class Module : public Node
{
public:

    Module(location loc, std::string* id);
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
