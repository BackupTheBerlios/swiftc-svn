#ifndef SWIFT_TYPENODE_H
#define SWIFT_TYPENODE_H

#include "utils/box.h"

#include "fe/auto.h"
#include "fe/node.h"
#include "fe/typelist.h"

namespace swift {

class Local;
class Type;
class Var;

class TypeNodeVisitor;

//------------------------------------------------------------------------------

class TypeNode : public Node
{
public:

    TypeNode(location loc_, Type* type = 0);
    virtual ~TypeNode();

    virtual void accept(TypeNodeVisitor* t) = 0;

    Type* type_;
};

//------------------------------------------------------------------------------

class Decl : public TypeNode
{
public:

    Decl(location loc, Type* type, std::string* id);
    virtual ~Decl();

    virtual void accept(TypeNodeVisitor* t);
    const std::string* id() const;
    const char* cid() const;

protected:

    std::string* id_;

public:

    Local* local_;
};

//------------------------------------------------------------------------------

class Expr : public TypeNode
{
public:

    Expr(location loc);

protected:

    bool lvalue_;
};

//------------------------------------------------------------------------------

class Access : public Expr
{
public:

    Access(location loc, Expr* postfixExpr);
    virtual ~Access();

    Expr* postfixExpr_;
    Access* nextAccess_;
    Class* class_;
};

//------------------------------------------------------------------------------

class IndexExpr : public Access
{
public:

    IndexExpr(location loc, Expr* postfixExpr, Expr* indexExpr);
    virtual ~IndexExpr();

    virtual void accept(TypeNodeVisitor* t);

private:

    Expr* indexExpr_;
};

//------------------------------------------------------------------------------

class MemberAccess : public Access
{
public:

    MemberAccess(location loc, Expr* expr_, std::string* id);
    virtual ~MemberAccess();

    virtual void accept(TypeNodeVisitor* t);

    const std::string* id() const;
    const char* cid() const;

private:

    std::string* id_;
};

//------------------------------------------------------------------------------

class FctCall : public Expr
{
public:

    FctCall(location loc, std::string* id, ExprList* exprList);
    virtual ~FctCall();

    virtual const char* qualifierStr() const = 0;
    const std::string* id() const;
    const char* cid() const;

protected:

    std::string* id_;

public:

    ExprList* exprList_;
    TypeList inTypes_;
};

//------------------------------------------------------------------------------

class CCall : public FctCall
{
public:

    CCall(location loc, Type* retType, TokenType token, std::string* id, ExprList* exprList);
    virtual ~CCall();

    virtual void accept(TypeNodeVisitor* t);
    virtual const char* qualifierStr() const;

    Type* retType_;
    TokenType token_;
};

//------------------------------------------------------------------------------

class MemberFctCall : public FctCall
{
public:

    MemberFctCall(location loc, std::string* id, ExprList* exprList);

    Class* class_;
    MemberFct* memberFct_;
};

//------------------------------------------------------------------------------

class MethodCall : public MemberFctCall
{
public:

    MethodCall(location loc, Expr* expr, std::string* id, ExprList* exprList);
    virtual ~MethodCall();

    Expr* expr_;
};

//------------------------------------------------------------------------------

class ReaderCall : public MethodCall
{
public:

    ReaderCall(location loc, Expr* expr, std::string* id, ExprList* exprList);

    virtual void accept(TypeNodeVisitor* t);
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class WriterCall : public MethodCall
{
public:

    WriterCall(location loc, Expr* expr, std::string* id, ExprList* exprList);

    virtual void accept(TypeNodeVisitor* t);
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class StaticMethodCall : public MemberFctCall
{
public:

    StaticMethodCall(location loc, std::string* id, ExprList* exprList);
};

//------------------------------------------------------------------------------

class RoutineCall : public StaticMethodCall
{
public:

    RoutineCall(location loc, std::string* classId, std::string* id, ExprList* exprList);
    virtual ~RoutineCall();

    virtual void accept(TypeNodeVisitor* t);
    virtual const char* qualifierStr() const;

    std::string* classId_;
};

//------------------------------------------------------------------------------

class OperatorCall : public StaticMethodCall
{
public:

    OperatorCall(location loc, TokenType token, Expr* op1, ExprList* exprList);

public:

    TokenType token_;
    Expr* op1_;
};

//------------------------------------------------------------------------------

class BinExpr : public OperatorCall
{
public:

    BinExpr(location loc, TokenType token, Expr* op1, Expr* op2);

    virtual void accept(TypeNodeVisitor* t);
    virtual const char* qualifierStr() const;

protected:

    Expr* op2_;
};

//------------------------------------------------------------------------------

class UnExpr : public OperatorCall
{
public:

    UnExpr(location loc, TokenType token, Expr* op);

    virtual void accept(TypeNodeVisitor* t);
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class Literal : public Expr
{
public:

    Literal(location loc, Box box, TokenType kind);

    virtual void accept(TypeNodeVisitor* t);
    TokenType getToken() const;

protected:

    Box box_;
    TokenType token_;
};

//------------------------------------------------------------------------------

class Id : public Expr
{
public:

    Id(location loc, std::string* id);
    virtual ~Id();

    virtual void accept(TypeNodeVisitor* t);

    const std::string* id() const;
    const char* cid() const;

protected:

    std::string* id_;
    Var* var_;
};

//------------------------------------------------------------------------------

class Nil : public Expr
{
public:

    Nil(location loc, Type* innerType);
    virtual ~Nil();

    virtual void accept(TypeNodeVisitor* t);

    Type* innerType_;
};

//------------------------------------------------------------------------------

class Self : public Expr
{
public:

    Self(location loc);

    virtual void accept(TypeNodeVisitor* t);
};

//------------------------------------------------------------------------------

class ExprList : public Node
{
public:

    ExprList(location loc, Expr* expr, ExprList* next);
    virtual ~ExprList();

    virtual void accept(TypeNodeVisitor* t);
    bool isValid() const;

    Expr* expr_;     ///< the Expr owned by this instance.
    ExprList* next_; ///< next element in the list, 0 if this is the last one.
};

//------------------------------------------------------------------------------

class Tuple : public Node
{
public:

    Tuple(location loc, TypeNode* typeNode, Tuple* next);
    virtual ~Tuple();

    virtual void accept(TypeNodeVisitor* t);
    bool isValid() const;

private:

    TypeNode* typeNode_; ///< Actual data.
    Tuple* next_;        ///< Next element in the list, 0 if this is the last one.
};

//------------------------------------------------------------------------------

class TypeNodeVisitor
{
public:
    
    TypeNodeVisitor(Context& ctxt);

    virtual void visit(Decl* d) = 0;

    // TypeNode -> Expr 
    virtual void visit(Id* id) = 0;
    virtual void visit(Literal* l) = 0;
    virtual void visit(Nil* n) = 0;
    virtual void visit(Self* n) = 0;

    // TypeNode -> Expr -> Access
    virtual void  preVisit(IndexExpr* i) = 0;
    virtual void postVisit(IndexExpr* i) = 0;
    virtual void  preVisit(MemberAccess* m) = 0;
    virtual void postVisit(MemberAccess* m) = 0;

    // TypeNode -> Expr -> FctCall -> CCall
    virtual void  preVisit(CCall* c) = 0;
    virtual void postVisit(CCall* c) = 0;

    // TypeNode -> Expr -> FctCall -> MemberFctCall -> MethodCall
    virtual void  preVisit(ReaderCall* r) = 0;
    virtual void postVisit(ReaderCall* r) = 0;
    virtual void  preVisit(WriterCall* w) = 0;
    virtual void postVisit(WriterCall* w) = 0;

    // TypeNode -> Expr -> FctCall -> MemberFctCall -> StaticMethodCall
    virtual void  preVisit(BinExpr* b) = 0;
    virtual void postVisit(BinExpr* b) = 0;
    virtual void  preVisit(RoutineCall* r) = 0;
    virtual void postVisit(RoutineCall* r) = 0;
    virtual void  preVisit(UnExpr* u) = 0;
    virtual void postVisit(UnExpr* u) = 0;

protected:

    Context& ctxt_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPENODE_H
