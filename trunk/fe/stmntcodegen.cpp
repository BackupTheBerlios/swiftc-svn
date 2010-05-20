#include "fe/stmntcodegen.h"

#include <llvm/BasicBlock.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>

#include "fe/context.h"
#include "fe/scope.h"
#include "fe/typenodecodegen.h"

namespace swift {

StmntCodeGen::StmntVisitor(Context* ctxt)
    : StmntVisitorBase(ctxt)
{}

void StmntCodeGen::visit(CFStmnt* s)
{
}

void StmntCodeGen::visit(DeclStmnt* s)
{
    TypeNodeCodeGen typeNodeCodeGen(ctxt_);
    s->decl_->accept(&typeNodeCodeGen);
}

void StmntCodeGen::visit(IfElStmnt* s)
{
    TypeNodeCodeGen typeNodeCodeGen(ctxt_);
    s->expr_->accept(&typeNodeCodeGen);

    // create new sub basic block
    //s->ifScope_->bb_ = llvm::BasicBlock::Create(
            //ctxt_->module_->getLLVMModule()->getContext(),
            //"if",
            //ctxt_->llvmFct_);

    //if (s->elScope_)
    //{
        //s->elScope_->bb_ = llvm::BasicBlock::Create(
                //ctxt_->module_->getLLVMModule()->getContext(),
                //"else",
                //ctxt_->llvmFct_);
    //}
}

void StmntCodeGen::visit(RepeatUntilStmnt* s)
{
    TypeNodeCodeGen typeNodeCodeGen(ctxt_);
    s->expr_->accept(&typeNodeCodeGen);

    // create new sub basic block
    //s->scope_->bb_ = llvm::BasicBlock::Create(
            //ctxt_->module_->getLLVMModule()->getContext(),
            //"repeat-until",
            //ctxt_->llvmFct_);
}

void StmntCodeGen::visit(ScopeStmnt* s)
{
}

void StmntCodeGen::visit(WhileStmnt* s)
{
    TypeNodeCodeGen typeNodeCodeGen(ctxt_);
    s->expr_->accept(&typeNodeCodeGen);
}

void StmntCodeGen::visit(AssignStmnt* s)
{
    TypeNodeCodeGen typeNodeCodeGen(ctxt_);

    s->exprList_->accept(&typeNodeCodeGen);
    s->tuple_->accept(&typeNodeCodeGen);
}

void StmntCodeGen::visit(ExprStmnt* s)
{
    TypeNodeCodeGen typeNodeCodeGen(ctxt_);
    s->expr_->accept(&typeNodeCodeGen);
}

} // namespace swift
