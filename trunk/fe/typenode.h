#ifndef SWIFT_TYPENODE_H
#define SWIFT_TYPENODE_H

#include <map>

#include "utils/box.h"

#include "fe/auto.h"
#include "fe/node.h"
#include "fe/typelist.h"

namespace llvm {
    class Type;
}

namespace swift {

class Local;
class TNList;
class Type;
class TypeNodeVisitorBase;
class Var;

//------------------------------------------------------------------------------

class TypeNode : public Node
{
public:

    TypeNode(location loc_, Type* type = 0);
    virtual ~TypeNode();

    virtual void accept(TypeNodeVisitorBase* t) = 0;

    const Type* getType() const;

protected:

    Type* type_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class Decl : public TypeNode
{
public:

    Decl(location loc, Type* type, std::string* id);
    virtual ~Decl();

    virtual void accept(TypeNodeVisitorBase* t);
    const std::string* id() const;
    const char* cid() const;

protected:

    std::string* id_;
    Local* local_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class Expr : public TypeNode
{
public:

    Expr(location loc);

protected:

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class ErrorExpr : public Expr
{
public:

    ErrorExpr(location loc);

    virtual void accept(TypeNodeVisitorBase* t);

protected:

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class Access : public Expr
{
public:

    Access(location loc, Expr* prefixExpr);
    virtual ~Access();

protected:

    Expr* prefixExpr_;
    Class* class_;
    Access* next_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class IndexExpr : public Access
{
public:

    IndexExpr(location loc, Expr* prefixExpr, Expr* indexExpr);
    virtual ~IndexExpr();

    virtual void accept(TypeNodeVisitorBase* t);

protected:

    Expr* indexExpr_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class MemberAccess : public Access
{
public:

    MemberAccess(location loc, Expr* expr_, std::string* id);
    virtual ~MemberAccess();

    virtual void accept(TypeNodeVisitorBase* t);

    const std::string* id() const;
    const char* cid() const;

protected:

    std::string* id_;
    MemberVar* memberVar_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class FctCall : public Expr
{
public:

    FctCall(location loc, std::string* id, TNList* exprList);
    virtual ~FctCall();

    virtual const char* qualifierStr() const = 0;
    const std::string* id() const;
    const char* cid() const;

protected:

    std::string* id_;

    TNList* exprList_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class CCall : public FctCall
{
public:

    CCall(location loc, Type* retType, TokenType token, std::string* id, TNList* exprList);
    virtual ~CCall();

    virtual void accept(TypeNodeVisitorBase* t);
    virtual const char* qualifierStr() const;

protected:

    Type* retType_;
    TokenType token_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class MemberFctCall : public FctCall
{
public:

    MemberFctCall(location loc, std::string* id, TNList* exprList);

    void setTuple(const TNList* tuple);
    MemberFct* getMemberFct() const;

protected:

    Class* class_;
    MemberFct* memberFct_;
    const TNList* tuple_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class MethodCall : public MemberFctCall
{
public:

    MethodCall(location loc, Expr* expr, std::string* id, TNList* exprList);
    virtual ~MethodCall();

protected:

    Expr* expr_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class ReaderCall : public MethodCall
{
public:

    ReaderCall(location loc, Expr* expr, std::string* id, TNList* exprList);

    virtual void accept(TypeNodeVisitorBase* t);
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class WriterCall : public MethodCall
{
public:

    WriterCall(location loc, Expr* expr, std::string* id, TNList* exprList);

    virtual void accept(TypeNodeVisitorBase* t);
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class StaticMethodCall : public MemberFctCall
{
public:

    StaticMethodCall(location loc, std::string* id, TNList* exprList);
};

//------------------------------------------------------------------------------

class RoutineCall : public StaticMethodCall
{
public:

    RoutineCall(location loc, std::string* classId, std::string* id, TNList* exprList);
    virtual ~RoutineCall();

    virtual void accept(TypeNodeVisitorBase* t);
    virtual const char* qualifierStr() const;

protected:

    std::string* classId_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class OperatorCall : public StaticMethodCall
{
public:

    OperatorCall(location loc, TokenType token, Expr* op1);

protected:

    TokenType token_;
    Expr* op1_;
    bool builtin_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class BinExpr : public OperatorCall
{
public:

    BinExpr(location loc, TokenType token, Expr* op1, Expr* op2);

    virtual void accept(TypeNodeVisitorBase* t);
    virtual const char* qualifierStr() const;

protected:

    Expr* op2_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class UnExpr : public OperatorCall
{
public:

    UnExpr(location loc, TokenType token, Expr* op);

    virtual void accept(TypeNodeVisitorBase* t);
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class Literal : public Expr
{
public:

    Literal(location loc, Box box, TokenType kind);

    virtual void accept(TypeNodeVisitorBase* t);
    TokenType getToken() const;

    static void initTypeMap(llvm::LLVMContext* llvmCtxt);
    static void destroyTypeMap();

protected:

    Box box_;
    TokenType token_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class Id : public Expr
{
public:

    Id(location loc, std::string* id);
    virtual ~Id();

    virtual void accept(TypeNodeVisitorBase* t);

    const std::string* id() const;
    const char* cid() const;

protected:

    std::string* id_;
    Var* var_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class Nil : public Expr
{
public:

    Nil(location loc, Type* innerType);
    virtual ~Nil();

    virtual void accept(TypeNodeVisitorBase* t);

protected:

    Type* innerType_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class Self : public Expr
{
public:

    Self(location loc);

    virtual void accept(TypeNodeVisitorBase* t);
};

//------------------------------------------------------------------------------

class TypeNodeVisitorBase
{
public:
    
    TypeNodeVisitorBase(Context* ctxt);
    virtual ~TypeNodeVisitorBase() {}

    virtual void visit(Decl* d) = 0;
    virtual void visit(ErrorExpr* e) = 0;

    // TypeNode -> Expr 
    virtual void visit(Id* id) = 0;
    virtual void visit(Literal* l) = 0;
    virtual void visit(Nil* n) = 0;
    virtual void visit(Self* n) = 0;

    // TypeNode -> Expr -> Access
    virtual void visit(IndexExpr* i) = 0;
    virtual void visit(MemberAccess* m) = 0;

    // TypeNode -> Expr -> FctCall -> CCall
    virtual void visit(CCall* c) = 0;

    // TypeNode -> Expr -> FctCall -> MemberFctCall -> MethodCall
    virtual void visit(ReaderCall* r) = 0;
    virtual void visit(WriterCall* w) = 0;

    // TypeNode -> Expr -> FctCall -> MemberFctCall -> StaticMethodCall
    virtual void visit(BinExpr* b) = 0;
    virtual void visit(RoutineCall* r) = 0;
    virtual void visit(UnExpr* u) = 0;

protected:

    Context* ctxt_;
};

//------------------------------------------------------------------------------

template<class T> class TypeNodeVisitor;

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPENODE_H
