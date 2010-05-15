#ifndef SWIFT_TYPENODEANALYZER_H
#define SWIFT_TYPENODEANALYZER_H

#include "fe/typenode.h"

namespace swift {

class TypeNodeAnalyzer : public TypeNodeVisitor
{
public:

    TypeNodeAnalyzer(Context& ctxt);

    virtual void visit(Decl* d);

    // TypeNode -> Expr 
    virtual void visit(Id* id);
    virtual void visit(Literal* l);
    virtual void visit(Nil* n);
    virtual void visit(Self* n);

    // TypeNode -> Expr -> Access
    virtual void  preVisit(IndexExpr* i);
    virtual void postVisit(IndexExpr* i);
    virtual void  preVisit(MemberAccess* m);
    virtual void postVisit(MemberAccess* m);

    // TypeNode -> Expr -> FctCall -> CCall
    virtual void  preVisit(CCall* c);
    virtual void postVisit(CCall* c);

    // TypeNode -> Expr -> FctCall -> MemberFctCall -> MethodCall
    virtual void  preVisit(ReaderCall* r);
    virtual void postVisit(ReaderCall* r);
    virtual void  preVisit(WriterCall* w);
    virtual void postVisit(WriterCall* w);

    // TypeNode -> Expr -> FctCall -> MemberFctCall -> StaticMethodCall
    virtual void  preVisit(BinExpr* b);
    virtual void postVisit(BinExpr* b);
    virtual void  preVisit(RoutineCall* r);
    virtual void postVisit(RoutineCall* r);
    virtual void  preVisit(UnExpr* u);
    virtual void postVisit(UnExpr* u);

    void analyzeMemberFctCall(MemberFctCall* m);
    void setClass(MethodCall* m);
    void setClass(OperatorCall* r);
    void setClass(RoutineCall* r);
};

} // namespace swift

#endif // SWIFT_TYPENODEANALYZER_H
