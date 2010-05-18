#ifndef SWIFT_TYPENODE_CODE_GEN_H
#define SWIFT_TYPENODE_CODE_GEN_H

#include "fe/typenode.h"

namespace swift {

template <>
class TypeNodeVisitor<class CodeGen> : public TypeNodeVisitorBase
{
public:

    TypeNodeVisitor(Context& ctxt);

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
};

typedef TypeNodeVisitor<class CodeGen> TypeNodeCodeGen;

} // namespace swift

#endif // SWIFT_TYPENODE_CODE_GEN_H
