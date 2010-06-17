#ifndef SWIFT_TYPENODE_CODE_GEN_H
#define SWIFT_TYPENODE_CODE_GEN_H

#include <memory>

#include "utils/llvmhelper.h"

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

    virtual void visit(Decl* d);

    // TypeNode -> Expr 
    virtual void visit(ErrorExpr* e);
    virtual void visit(Id* id);
    virtual void visit(Literal* l);
    virtual void visit(Nil* n);
    virtual void visit(Self* n);

    // TypeNode -> Expr -> Access
    llvm::Value* resolvePrefixExpr(Access* a);
    virtual void visit(IndexExpr* i);
    virtual void visit(MemberAccess* m);

    // TypeNode -> Expr -> FctCall -> CCall
    virtual void visit(CCall* c);

    // TypeNode -> Expr -> FctCall -> MemberFctCall -> MethodCall
    virtual void visit(ReaderCall* r);
    virtual void visit(WriterCall* w);

    // TypeNode -> Expr -> FctCall -> MemberFctCall -> StaticMethodCall
    virtual void visit(BinExpr* b);
    virtual void visit(UnExpr* u);
    virtual void visit(RoutineCall* r);

    bool isAddr(size_t i = 0) const;
    llvm::Value* getScalar(size_t i = 0) const;
    llvm::Value* getAddr(size_t i = 0) const;
    llvm::Value* getValue(size_t i = 0) const;

    void emitCall(MemberFctCall* call, llvm::Value* self);
    void getSelf(MethodCall* m);

private:

    void setResult(llvm::Value* value, bool isAddr);

    std::vector<llvm::Value*> values_;
    BoolVec addresses_;

    LLVMBuilder& builder_;
    llvm::LLVMContext& lctxt_;
};

typedef TypeNodeVisitor<class CodeGen> TypeNodeCodeGen;

} // namespace swift

#endif // SWIFT_TYPENODE_CODE_GEN_H
