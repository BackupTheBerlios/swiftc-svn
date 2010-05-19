#ifndef SWIFT_NODE_H
#define SWIFT_NODE_H

#include "utils/list.h"
#include "utils/map.h"
#include "utils/stringhelper.h"

#include "fe/location.hh"

namespace llvm {
    class LLVMContext;
}

namespace swift {

class Class;
class ClassMember;
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

    void insert(Context* ctxt, Class* c); 
    Class* lookupClass(const std::string* id);
    const std::string* id() const;
    const char* cid() const;
    bool analyze(Context* ctxt);
    bool buildLLVMTypes();
    void codeGen(Context* ctxt);
    llvm::LLVMContext* getLLVMContext() const;

    typedef std::map<const std::string*, Class*, StringPtrCmp> ClassMap;
    const ClassMap& classes() const;

private:

    std::string* id_;

private:

    ClassMap classes_;
    llvm::LLVMContext* llvmCtxt_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_NODE_H
