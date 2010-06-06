#ifndef SWIFT_TYPENODE_CODE_GEN_H
#define SWIFT_TYPENODE_CODE_GEN_H

#include <memory>

#include "fe/typenode.h"

namespace llvm {
    class Value;
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

    llvm::Value* getLLVMValue() const;
    bool isAddr() const;
    llvm::Value* getScalar();
    //llvm::Value* createEntryAllocaAndStore(llvm::Value* value);

    void setArgs(MemberFctCall* fct, llvm::Value* self);

private:

    llvm::Value* llvmValue_;
    bool isAddr_;
};

typedef TypeNodeVisitor<class CodeGen> TypeNodeCodeGen;

} // namespace swift

#endif // SWIFT_TYPENODE_CODE_GEN_H
