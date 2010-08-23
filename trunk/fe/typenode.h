#ifndef SWIFT_TYPENODE_H
#define SWIFT_TYPENODE_H

#include <map>

#include "utils/box.h"

#include "fe/auto.h"
#include "fe/node.h"
#include "fe/typelist.h"

class Place;
typedef std::vector<Place*> Places;

namespace llvm {
    class Type;
    class AllocaInst;
}

namespace swift {

class Local;
class MemberVar;
class TNList;
class Type;
class TypeNodeVisitorBase;
class Var;

//------------------------------------------------------------------------------

struct TNResult
{
    const Type* type_;
    Place* place_;
    int simdLength_;
    bool inits_;
    bool lvalue_;
};

typedef std::vector<TNResult> TNResults;

class TypeNode : public Node
{
public:

    TypeNode(location loc_, Type* type = 0);
    virtual ~TypeNode();

    virtual void accept(TypeNodeVisitorBase* t) = 0;

    const TNResult& get(size_t i = 0) const { return results_[i]; }
    size_t numResults() const { return results_.size(); }

protected:

    TNResult& set(size_t i = 0) { return results_[i]; }

    TNResults results_;

    friend class TNList;       // TODO clean up this
    friend class AssignCreate; // TODO clean up this
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
    llvm::AllocaInst* alloca_;

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

class SimdIndexExpr : public Access
{
public:

    SimdIndexExpr(location loc, Expr* prefixExpr, std::string* id);
    ~SimdIndexExpr();

    virtual void accept(TypeNodeVisitorBase* t);

protected:

    std::string* id_;

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

    MemberFct* getMemberFct() const;

protected:

    Class* class_;
    MemberFct* memberFct_;
    bool simd_;
    Places* initPlaces_;

    template<class T> friend class StmntVisitor; // HACK
    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class MethodCall : public MemberFctCall
{
public:

    MethodCall(location loc, Expr* expr, std::string* id, TNList* exprList);
    virtual ~MethodCall();

    virtual void accept(TypeNodeVisitorBase* t);
    virtual const char* qualifierStr() const;

protected:

    Expr* expr_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class OperatorCall : public MethodCall
{
public:

    OperatorCall(location loc, std::string* id, Expr* op1);

protected:

    Expr* op1_;
    bool builtin_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class BinExpr : public OperatorCall
{
public:

    BinExpr(location loc, std::string* id, Expr* op1, Expr* op2);

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

    UnExpr(location loc, std::string* id, Expr* op);

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

class CreateCall : public StaticMethodCall
{
public:

    CreateCall(location loc, std::string* classId, TNList* exprList);
    virtual ~CreateCall();

    virtual void accept(TypeNodeVisitorBase* t);
    virtual const char* qualifierStr() const;

protected:

    std::string* classId_;

    template<class T> friend class TypeNodeVisitor;
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

class Literal : public Expr
{
public:

    Literal(location loc, Box box, TokenType kind);

    virtual void accept(TypeNodeVisitorBase* t);
    TokenType getToken() const;

    static void initTypeMap(llvm::LLVMContext* lctxt);
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

    Nil(location loc, Type* type);
    virtual ~Nil();

    virtual void accept(TypeNodeVisitorBase* t);

protected:

    Type* type_;

    template<class T> friend class TypeNodeVisitor;
};

//------------------------------------------------------------------------------

class Self : public Expr
{
public:

    Self(location loc);

    virtual void accept(TypeNodeVisitorBase* t);
};

//----------------------------------------------------------------------

class TypeNodeVisitorBase
{
public:
    
    TypeNodeVisitorBase(Context* ctxt);
    virtual ~TypeNodeVisitorBase() {}

    virtual void visit(ErrorExpr* e) = 0;
    virtual void visit(Decl* d) = 0;

    // TypeNode -> Expr 
    virtual void visit(Id* id) = 0;
    virtual void visit(Literal* l) = 0;
    virtual void visit(Nil* n) = 0;
    virtual void visit(Self* n) = 0;

    // TypeNode -> Expr -> Access
    virtual void visit(IndexExpr* i) = 0;
    virtual void visit(SimdIndexExpr* i) = 0;
    virtual void visit(MemberAccess* m) = 0;

    // TypeNode -> Expr -> FctCall
    virtual void visit(CCall* c) = 0;
    virtual void visit(MethodCall* m) = 0;
    virtual void visit(CreateCall* c) = 0;
    virtual void visit(RoutineCall* r) = 0;
    virtual void visit(BinExpr* b) = 0;
    virtual void visit(UnExpr* u) = 0;

protected:

    Context* ctxt_;
};

//------------------------------------------------------------------------------

template<class T> class TypeNodeVisitor;

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPENODE_H
