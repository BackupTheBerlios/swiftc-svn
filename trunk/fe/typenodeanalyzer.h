#ifndef SWIFT_TYPENODEANALYZER_H
#define SWIFT_TYPENODEANALYZER_H

#include "fe/typenode.h"

namespace swift {

template<>
class TypeNodeVisitor<class Analyzer> : public TypeNodeVisitorBase
{
public:

    TypeNodeVisitor(Context* ctxt);

    virtual void visit(ErrorExpr* e);
    virtual void visit(Decl* d);

    // TypeNode -> Expr 
    virtual void visit(Broadcast* u);
    virtual void visit(Id* id);
    virtual void visit(Literal* l);
    virtual void visit(Nil* n);
    virtual void visit(Range* n);
    virtual void visit(Self* n);
    virtual void visit(SimdIndex* s);

    // TypeNode -> Expr -> Access
    bool examinePrefixExpr(Access* a);
    virtual void visit(IndexExpr* i);
    virtual void visit(MemberAccess* m);

    // TypeNode -> Expr -> FctCall
    virtual void visit(CCall* c);
    virtual void visit(MethodCall* m);
    virtual void visit(CreateCall* r);
    virtual void visit(RoutineCall* r);
    virtual void visit(UnExpr* u);
    virtual void visit(BinExpr* b);

    bool isLValue(size_t i) const;

private:

    void analyzeMemberFctCall(MemberFctCall* m);
    bool setClass(MethodCall* m);
    bool setClass(RoutineCall* r);

    void setResult(TypeNode* tn, Type* type, bool lvalue);
    void setError(TypeNode* tn, bool lvalue);
};

typedef TypeNodeVisitor<class Analyzer> TypeNodeAnalyzer;

} // namespace swift

#endif // SWIFT_TYPENODEANALYZER_H
