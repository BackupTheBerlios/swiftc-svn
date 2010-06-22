#ifndef SWIFT_CLASS_CODE_GEN_H
#define SWIFT_CLASS_CODE_GEN_H

#include <memory>

#include "utils/llvmhelper.h"

#include "fe/class.h"

namespace swift {

//------------------------------------------------------------------------------

template <class T> class StmntVisitor;
typedef StmntVisitor<class CodeGen> StmntCodeGen;

//------------------------------------------------------------------------------

template<>
class ClassVisitor<class CodeGen> : public ClassVisitorBase
{
public:

    ClassVisitor(Context* ctxt);
    ~ClassVisitor();

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

private:

    void codeGen(MemberFct* m);

    std::auto_ptr<StmntCodeGen> scg_;
    LLVMBuilder& builder_;
    llvm::LLVMContext& lctxt_;
};

typedef ClassVisitor<class CodeGen> ClassCodeGen;

} // namespace swift

#endif // SWIFT_CLASS_CODE_GEN_H
