#include "fe/typenode.h"

#include <cmath>

#include <llvm/Support/TypeBuilder.h>

#include "utils/llvmplace.h"

#include "fe/context.h"
#include "fe/tnlist.h"
#include "fe/type.h"
#include "fe/var.h"

namespace swift {

//------------------------------------------------------------------------------

TypeNode::TypeNode(location loc, Type* type /*= 0*/)
    : Node(loc)
{
    if (type)
    {
        results_.resize(1);
        results_[0].type_ = type;
        results_[0].inits_ = false;
    }
}

TypeNode::~TypeNode()
{
    for (size_t i = 0; i < results_.size(); ++i)
    {
        delete results_[i].place_;
        delete results_[i].type_;
    }
}

//------------------------------------------------------------------------------

Decl::Decl(location loc, Type* type, std::string* id)
    : TypeNode( loc, type->clone() )
    , id_(id)
    , local_(0)
    , alloca_(0)
{
    delete type;
}

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
{}

//------------------------------------------------------------------------------

ErrorExpr::ErrorExpr(location loc)
    : Expr(loc)
{}

void ErrorExpr::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

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

SimdIndexExpr::SimdIndexExpr(location loc, Expr* prefixExpr, std::string* id)
    : Access(loc, prefixExpr)
    , id_(id)
{}

SimdIndexExpr::~SimdIndexExpr()
{
    delete id_;
}

void SimdIndexExpr::accept(TypeNodeVisitorBase* t)
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

FctCall::FctCall(location loc, std::string* id, TNList* exprList)
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

CCall::CCall(location loc, Type* retType, TokenType token, std::string* id, TNList* exprList)
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

MemberFctCall::MemberFctCall(location loc, std::string* id, TNList* exprList)
    : FctCall(loc, id, exprList)
    , class_(0)
    , memberFct_(0)
    , simd_(false)
    , initPlaces_(0)
{}

MemberFct* MemberFctCall::getMemberFct() const
{
    return memberFct_;
}

//------------------------------------------------------------------------------

StaticMethodCall::StaticMethodCall(location loc, std::string* id, TNList* exprList)
    : MemberFctCall(loc, id, exprList)
{}

//------------------------------------------------------------------------------

CreateCall::CreateCall(location loc, std::string* classId, TNList* exprList)
    : StaticMethodCall( loc, new std::string("create"), exprList )
    , classId_(classId)
{}

CreateCall::~CreateCall()
{
    delete classId_;
}

void CreateCall::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const char* CreateCall::qualifierStr() const
{
    static const char* str = "create";
    return str;
}

//------------------------------------------------------------------------------

RoutineCall::RoutineCall(location loc, std::string* classId, std::string* id, TNList* exprList)
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

OperatorCall::OperatorCall(location loc, std::string* id, Expr* op1)
    : MethodCall(loc, op1, id, new TNList() ) 
    , op1_(op1) // alias
    , builtin_(true)
{
    //exprList_->append(op1);
}

//------------------------------------------------------------------------------

BinExpr::BinExpr(location loc, std::string* id, Expr* op1, Expr* op2)
    : OperatorCall(loc, id, op1)
    , op2_(op2)
{
    exprList_->append(op2);
}

void BinExpr::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const char* BinExpr::qualifierStr() const
{
    static const char* str = "binary method";
    return str;
}

//------------------------------------------------------------------------------

UnExpr::UnExpr(location loc, std::string* id, Expr* op)
    : OperatorCall(loc, id, op)
{}

void UnExpr::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const char* UnExpr::qualifierStr() const
{
    static const char* str = "unary method";
    return str;
}

//------------------------------------------------------------------------------

MethodCall::MethodCall(location loc, Expr* expr, std::string* id, TNList* exprList)
    : MemberFctCall(loc, id, exprList)
    , expr_(expr)
{}

MethodCall::~MethodCall()
{
    delete expr_;
}

void MethodCall::accept(TypeNodeVisitorBase* t)
{
    t->visit(this);
}

const char* MethodCall::qualifierStr() const
{
    static const char* str = "reader";
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

Nil::Nil(location loc, Type* type)
    : Expr(loc)
    , type_(type)
{}

Nil::~Nil()
{
    delete type_;
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

TypeNodeVisitorBase::TypeNodeVisitorBase(Context* ctxt)
    : ctxt_(ctxt)
{}

} // namespace swift
