#include "fe/stmntcodegen.h"

#include <llvm/LLVMContext.h>

#include "fe/context.h"

namespace swift {

StmntCodeGen::StmntVisitor(Context* ctxt, llvm::LLVMContext* llvmCtxt)
    : StmntVisitorBase(ctxt)
    , llvmCtxt_(llvmCtxt)
{}

void StmntCodeGen::visit(CFStmnt* s)
{
}

void StmntCodeGen::visit(DeclStmnt* s)
{
}

void StmntCodeGen::visit(IfElStmnt* s)
{
}

void StmntCodeGen::visit(RepeatUntilStmnt* s)
{
}

void StmntCodeGen::visit(ScopeStmnt* s)
{
}

void StmntCodeGen::visit(WhileStmnt* s)
{
}

void StmntCodeGen::visit(AssignStmnt* s)
{
}

void StmntCodeGen::visit(ExprStmnt* s)
{
}

} // namespace swift
