#ifndef SWIFT_CLASS_CODE_GEN_H
#define SWIFT_CLASS_CODE_GEN_H

#include "fe/class.h"

namespace llvm {
    class LLVMContext;
}

namespace swift {

template<>
class ClassVisitor<class CodeGen> : public ClassVisitorBase
{
public:

    ClassVisitor(Context* ctxt, llvm::LLVMContext* llvmCtxt);

    virtual void visit(Class* c);

    // ClassMember -> MemberFct -> Method
    virtual void visit(Create* c);
    virtual void visit(Reader* r);
    virtual void visit(Writer* w);

    // ClassMember -> MemberFct -> StaticMethod
    virtual void visit(Assign* a);
    virtual void visit(Operator* o);
    virtual void visit(Routine* r);

    // ClassMember -> MemberVar
    virtual void visit(MemberVar* m);

protected:

    llvm::LLVMContext* llvmCtxt_;
};

typedef ClassVisitor<class CodeGen> ClassCodeGen;

} // namespace swift

#endif // SWIFT_CLASS_CODE_GEN_H
