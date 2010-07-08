#ifndef SWIFt_STMNT_CODE_GEN_H
#define SWIFt_STMNT_CODE_GEN_H

#include <memory>

#include "utils/llvmhelper.h"

#include "fe/stmnt.h"

namespace swift {

//------------------------------------------------------------------------------

template <class T> class TypeNodeVisitor;
typedef TypeNodeVisitor<class CodeGen> TypeNodeCodeGen;

//------------------------------------------------------------------------------

template <>
class StmntVisitor<class CodeGen> : public StmntVisitorBase
{
public:
    StmntVisitor(Context* ctxt);
    virtual ~StmntVisitor();

    virtual void visit(ErrorStmnt* s);
    virtual void visit(CFStmnt* s);
    virtual void visit(DeclStmnt* s);
    virtual void visit(IfElStmnt* s);
    virtual void visit(RepeatUntilLoop* l);
    virtual void visit(WhileLoop* l);
    virtual void visit(SimdLoop* l);
    virtual void visit(ScopeStmnt* s);

    // Stmnt -> ActionStmnt
    virtual void visit(AssignStmnt* s);
    virtual void visit(ExprStmnt* s);

private:

    LLVMBuilder& builder_;
    llvm::LLVMContext& lctxt_;
    TypeNodeCodeGen* tncg_;
};

typedef StmntVisitor<class CodeGen> StmntCodeGen;

} // namespace swift

#endif // SWIFt_STMNT_CODE_GEN_H
