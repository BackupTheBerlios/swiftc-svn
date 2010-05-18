#ifndef SWIFt_STMNT_CODE_GEN_H
#define SWIFt_STMNT_CODE_GEN_H

#include <llvm/LLVMContext.h>

#include "fe/stmnt.h"

namespace swift {

template <>
class StmntVisitor<class CodeGen> : public StmntVisitorBase
{
public:
    StmntVisitor(Context& ctxt, llvm::LLVMContext& llvmCtxt);

    virtual void visit(CFStmnt* s);
    virtual void visit(DeclStmnt* s);
    virtual void visit(IfElStmnt* s);
    virtual void visit(RepeatUntilStmnt* s);
    virtual void visit(ScopeStmnt* s);
    virtual void visit(WhileStmnt* s);

    // Stmnt -> ActionStmnt
    virtual void visit(AssignStmnt* s);
    virtual void visit(ExprStmnt* s);

protected:

    llvm::LLVMContext& llvmCtxt_;
};

typedef StmntVisitor<class CodeGen> StmntCodeGen;

} // namespace swift

#endif // SWIFt_STMNT_CODE_GEN_H
