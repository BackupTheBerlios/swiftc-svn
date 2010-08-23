#ifndef SWIFT_TYPENODE_CODE_GEN_H
#define SWIFT_TYPENODE_CODE_GEN_H

#include "utils/llvmhelper.h"
#include "utils/llvmplace.h"

#include "fe/typenode.h"

namespace llvm {
    class Value;
    class LLVMContext;
    class Module;
}

namespace swift {

template <>
class TypeNodeVisitor<class CodeGen> : public TypeNodeVisitorBase
{
public:

    TypeNodeVisitor(Context* ctxt);

    virtual void visit(ErrorExpr* e);
    virtual void visit(Decl* d);

    // TypeNode -> Expr 
    virtual void visit(Id* id);
    virtual void visit(Literal* l);
    virtual void visit(Nil* n);
    virtual void visit(Self* n);

    // TypeNode -> Expr -> Access
    virtual void visit(SimdIndexExpr* s);
    virtual void visit(IndexExpr* i);
    virtual void visit(MemberAccess* m);

    // TypeNode -> Expr -> FctCall
    virtual void visit(CCall* c);
    virtual void visit(MethodCall* m);
    virtual void visit(CreateCall* c);
    virtual void visit(RoutineCall* r);
    virtual void visit(UnExpr* u);
    virtual void visit(BinExpr* b);

private:

    llvm::Value* resolvePrefixExpr(Access* a);
    void emitCall(MemberFctCall* call, Place* self);
    Place* getSelf(MethodCall* m);

    void setResult(TypeNode* tn, Place* place);

    LLVMBuilder& builder_;
    llvm::LLVMContext& lctxt_;
};

typedef TypeNodeVisitor<class CodeGen> TypeNodeCodeGen;

} // namespace swift

#endif // SWIFT_TYPENODE_CODE_GEN_H
