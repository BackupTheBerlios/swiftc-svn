#include "fe/typenode.h"

#include <cmath>

#include <llvm/Support/TypeBuilder.h>

#include "fe/context.h"
#include "fe/token2str.h"
#include "fe/type.h"
#include "fe/var.h"

namespace swift {

//------------------------------------------------------------------------------

TypeNode::TypeNode(location loc, Type* type /*= 0*/)
    : Node(loc)
    , type_(type)
{}

TypeNode::~TypeNode()
{
    delete type_;
}

const Type* TypeNode::getType() const
{
    return type_;
}

//------------------------------------------------------------------------------

Decl::Decl(location loc, Type* type, std::string* id)
    : TypeNode(loc, type)
    , id_(id)
    , local_(0)
{}

Decl::~Decl()
{
    delete local_;
    delete id_;
}

void Decl::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const std::string* Decl::id() const
{
    return id_;
}

const char* Decl::cid() const
{
    return id_->c_str();
}

//------------------------------------------------------------------------------

Expr::Expr(location loc)
    : TypeNode(loc, 0)
    , lvalue_(false)
{}

//------------------------------------------------------------------------------

Id::Id(location loc, std::string* id)
    : Expr(loc)
    , id_(id)
{}

Id::~Id()
{
    delete id_;
}

void Id::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const std::string* Id::id() const
{
    return id_;
}

const char* Id::cid() const
{
    return id_->c_str();
}

//------------------------------------------------------------------------------

Access::Access(location loc, Expr* prefixExpr)
    : Expr(loc)
    , prefixExpr_(prefixExpr)
    , class_(0)
    , next_(0)
{}

Access::~Access()
{
    delete prefixExpr_;
}

//------------------------------------------------------------------------------

IndexExpr::IndexExpr(location loc, Expr* prefixExpr, Expr* indexExpr)
    : Access(loc, prefixExpr)
    , indexExpr_(indexExpr)
{}

IndexExpr::~IndexExpr()
{
    delete indexExpr_;
}

void IndexExpr::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

//------------------------------------------------------------------------------

MemberAccess::MemberAccess(location loc, Expr* prefixExpr, std::string* id)
    : Access(loc, prefixExpr)
    , id_(id)
{}

MemberAccess::~MemberAccess()
{
    delete id_;
}

void MemberAccess::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const std::string* MemberAccess::id() const
{
    return id_;
}

const char* MemberAccess::cid() const
{
    return id_->c_str();
}

//------------------------------------------------------------------------------

FctCall::FctCall(location loc, std::string* id, ExprList* exprList)
    : Expr(loc)
    , id_(id)
    , exprList_(exprList)
{}

FctCall::~FctCall()
{
    delete exprList_;
    delete id_;
}

const std::string* FctCall::id() const
{
    return id_;
}

const char* FctCall::cid() const
{
    return id_->c_str();
}

//------------------------------------------------------------------------------

CCall::CCall(location loc, Type* retType, TokenType token, std::string* id, ExprList* exprList)
    : FctCall(loc, id, exprList)
    , retType_(retType)
    , token_(token)
{}

CCall::~CCall()
{
    delete retType_;
}

void CCall::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const char* CCall::qualifierStr() const
{
    static const char* str = "c_call";
    return str;
}


//------------------------------------------------------------------------------

MemberFctCall::MemberFctCall(location loc, std::string* id, ExprList* exprList)
    : FctCall(loc, id, exprList)
    , class_(0)
    , memberFct_(0)
{}

//------------------------------------------------------------------------------

StaticMethodCall::StaticMethodCall(location loc, std::string* id, ExprList* exprList)
    : MemberFctCall(loc, id, exprList)
{}

//------------------------------------------------------------------------------

RoutineCall::RoutineCall(location loc, std::string* classId, std::string* id, ExprList* exprList)
    : StaticMethodCall(loc, id, exprList)
    , classId_(classId)
{}

RoutineCall::~RoutineCall()
{
    delete classId_;
}

void RoutineCall::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const char* RoutineCall::qualifierStr() const
{
    static const char* str = "routine";
    return str;
}

//------------------------------------------------------------------------------

OperatorCall::OperatorCall(location loc, TokenType token, Expr* op1, ExprList* exprList)
    : StaticMethodCall(loc, token2str(token), exprList) 
    , token_(token)
    , op1_(op1)
{}

//------------------------------------------------------------------------------

BinExpr::BinExpr(location loc, TokenType token, Expr* op1, Expr* op2)
    : OperatorCall( loc, token, op1, new ExprList(loc, op1, new ExprList(loc, op2, 0)) )
    , op2_(op2)
{}

void BinExpr::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const char* BinExpr::qualifierStr() const
{
    static const char* str = "operator";
    return str;
}

//------------------------------------------------------------------------------

UnExpr::UnExpr(location loc, TokenType token, Expr* op)
    : OperatorCall( loc, token, op, new ExprList(loc, op, 0) )
{}

void UnExpr::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const char* UnExpr::qualifierStr() const
{
    static const char* str = "operator";
    return str;
}

//------------------------------------------------------------------------------

MethodCall::MethodCall(location loc, Expr* expr, std::string* id, ExprList* exprList)
    : MemberFctCall(loc, id, exprList)
    , expr_(expr)
{}

MethodCall::~MethodCall()
{
    delete expr_;
}

//------------------------------------------------------------------------------

ReaderCall::ReaderCall(location loc, Expr* expr, std::string* id, ExprList* exprList)
    : MethodCall(loc, expr, id, exprList)
{}

void ReaderCall::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const char* ReaderCall::qualifierStr() const
{
    static const char* str = "reader";
    return str;
}

//------------------------------------------------------------------------------

WriterCall::WriterCall(location loc, Expr* expr, std::string* id, ExprList* exprList)
    : MethodCall(loc, expr, id, exprList)
{}

void WriterCall::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const char* WriterCall::qualifierStr() const
{
    static const char* str = "writer";
    return str;
}

//------------------------------------------------------------------------------

Literal::Literal(location loc, Box box, TokenType token)
    : Expr(loc)
    , box_(box)
    , token_(token)
{}

void Literal::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

TokenType Literal::getToken() const
{
    return token_;
}

//------------------------------------------------------------------------------

Nil::Nil(location loc, Type* innerType)
    : Expr(loc)
    , innerType_(innerType)
{}

Nil::~Nil()
{
    delete innerType_;
}

void Nil::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

//------------------------------------------------------------------------------

Self::Self(location loc)
    : Expr(loc)
{}

void Self::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

//------------------------------------------------------------------------------

ExprList::ExprList(location loc, Expr* expr, ExprList* next)
    : Node(loc)
    , expr_(expr)
    , next_(next)
{}

ExprList::~ExprList()
{
    delete expr_;
    delete next_;
}

void ExprList::accept(TypeNodeVisitorBase* t)
{

    for (ExprList* iter = this; iter != 0; iter = iter->next_)
    {
        if (iter->expr_)
            iter->expr_->accept(t);
    }
}

bool ExprList::isValid() const
{
    bool result = expr_ && expr_->getType();

    if (next_ && result)
        return result &= next_->isValid();

    return result;
}

TypeList ExprList::buildTypeList()
{
    TypeList types;

    for (ExprList* iter = this; iter != 0; iter = iter->next_)
        types.push_back( iter->expr_->getType() );

    return types;
}

//------------------------------------------------------------------------------

Tuple::Tuple(location loc, TypeNode* typeNode, Tuple* next)
    : Node(loc)
    , typeNode_(typeNode)
    , next_(next)
{}

Tuple::~Tuple()
{
    delete typeNode_;
    delete next_;
}

void Tuple::accept(TypeNodeVisitorBase* t)
{

    for (Tuple* iter = this; iter != 0; iter = iter->next_)
    {
        if (iter->typeNode_)
            iter->typeNode_->accept(t);
    }
}

bool Tuple::isValid() const
{
    bool result = typeNode_ && typeNode_->getType();

    if (next_ && result)
        return result &= next_->isValid();

    return result;
}

TypeList Tuple::buildTypeList()
{
    TypeList types;

    for (Tuple* iter = this; iter != 0; iter = iter->next_)
        types.push_back( iter->typeNode_->getType() );

    return types;
}


//------------------------------------------------------------------------------

TypeNodeVisitorBase::TypeNodeVisitorBase(Context* ctxt)
    : ctxt_(ctxt)
{}

} // namespace swift
