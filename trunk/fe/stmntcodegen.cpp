#include "fe/stmntcodegen.h"

#include <llvm/BasicBlock.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>

#include "fe/context.h"
#include "fe/class.h"
#include "fe/tnlist.h"
#include "fe/scope.h"
#include "fe/typenodecodegen.h"

namespace swift {

StmntCodeGen::StmntVisitor(Context* ctxt)
    : StmntVisitorBase(ctxt)
    , tncg_( new TypeNodeCodeGen(ctxt) )
{}

StmntCodeGen::~StmntVisitor() 
{
}

void StmntCodeGen::visit(ErrorStmnt* s) 
{
    swiftAssert(false, "unreachable");
}

void StmntCodeGen::visit(CFStmnt* s)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;
    MemberFct* memberFct = ctxt_->memberFct_;

    if (s->token_ == Token::RETURN)
    {
        if ( memberFct->sig_.out_.empty() )
            builder.CreateRetVoid();
        else
            builder.CreateBr(ctxt_->memberFct_->returnBB_);

        llvm::BasicBlock* bb = llvm::BasicBlock::Create(*ctxt_->module_->llvmCtxt_, "unreachable");
        ctxt_->llvmFct_->getBasicBlockList().push_back(bb);
        builder.SetInsertPoint(bb);

        return;
    }

    swiftAssert(false, "TODO");
}

void StmntCodeGen::visit(DeclStmnt* s)
{
    s->decl_->accept( tncg_.get() );
}

void StmntCodeGen::visit(IfElStmnt* s)
{
    llvm::LLVMContext& llvmCtxt = *ctxt_->module_->llvmCtxt_;
    llvm::Function* llvmFct = ctxt_->llvmFct_;
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    s->expr_->accept( tncg_.get() );
    llvm::Value* cond = tncg_->getScalar(0);

    /*
     * create new basic blocks
     */

    typedef llvm::BasicBlock BB;
    BB* thenBB = llvm::BasicBlock::Create(llvmCtxt, "then");
    BB* mergeBB = llvm::BasicBlock::Create(llvmCtxt, "merge");
    BB* elseBB = s->elScope_ ? llvm::BasicBlock::Create(llvmCtxt, "else") : 0;

    /*
     * create branch
     */

    if (elseBB)
        builder.CreateCondBr(cond, thenBB, elseBB);
    else
        builder.CreateCondBr(cond, thenBB, mergeBB);

    /*
     * emit code for thenBB
     */

    llvmFct->getBasicBlockList().push_back(thenBB);
    builder.SetInsertPoint(thenBB);
    s->ifScope_->accept(this, ctxt_);
    builder.CreateBr(mergeBB);

    /*
     * emit code for elseBB if applicable
     */

    if (elseBB)
    {
        llvmFct->getBasicBlockList().push_back(elseBB);
        builder.SetInsertPoint(elseBB);
        s->elScope_->accept(this, ctxt_);
        builder.CreateBr(mergeBB);
    }

    llvmFct->getBasicBlockList().push_back(mergeBB);
    builder.SetInsertPoint(mergeBB);
}

void StmntCodeGen::visit(RepeatUntilStmnt* s)
{
    llvm::LLVMContext& llvmCtxt = *ctxt_->module_->llvmCtxt_;
    llvm::Function* llvmFct = ctxt_->llvmFct_;
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    /*
     * create new basic blocks
     */

    typedef llvm::BasicBlock BB;
    BB* loopBB   = llvm::BasicBlock::Create(llvmCtxt, "rep");
    BB*  outBB   = llvm::BasicBlock::Create(llvmCtxt, "rep-out");

    /*
     * close current bb
     */

    builder.CreateBr(loopBB);

    /*
     * emit code for loopBB
     */

    llvmFct->getBasicBlockList().push_back(loopBB);
    builder.SetInsertPoint(loopBB);
    s->scope_->accept(this, ctxt_);
    s->expr_->accept( tncg_.get() );
    llvm::Value* cond = tncg_->getScalar(0);
    builder.CreateCondBr(cond, outBB, loopBB);

    /*
     * emit code for outBB
     */

    llvmFct->getBasicBlockList().push_back(outBB);
    builder.SetInsertPoint(outBB);
}

void StmntCodeGen::visit(ScopeStmnt* s) 
{
    s->scope_->accept(this, ctxt_);
}

void StmntCodeGen::visit(WhileStmnt* s)
{
    llvm::LLVMContext& llvmCtxt = *ctxt_->module_->llvmCtxt_;
    llvm::Function* llvmFct = ctxt_->llvmFct_;
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    /*
     * create new basic blocks
     */

    typedef llvm::BasicBlock BB;
    BB* headerBB = llvm::BasicBlock::Create(llvmCtxt, "while-header");
    BB* loopBB   = llvm::BasicBlock::Create(llvmCtxt, "while");
    BB*  outBB   = llvm::BasicBlock::Create(llvmCtxt, "while-out");

    /*
     * close current bb
     */

    builder.CreateBr(headerBB);

    /*
     * emit code for headerBB
     */

    llvmFct->getBasicBlockList().push_back(headerBB);
    builder.SetInsertPoint(headerBB);
    s->expr_->accept( tncg_.get() );
    llvm::Value* cond = tncg_->getScalar(0);
    builder.CreateCondBr(cond, loopBB, outBB);

    /*
     * emit code for loopBB
     */

    llvmFct->getBasicBlockList().push_back(loopBB);
    builder.SetInsertPoint(loopBB);
    s->scope_->accept(this, ctxt_);
    builder.CreateBr(headerBB);

    /*
     * emit code for outBB
     */

    llvmFct->getBasicBlockList().push_back(outBB);
    builder.SetInsertPoint(outBB);
}

void StmntCodeGen::visit(AssignStmnt* s)
{
    s->tuple_->accept( tncg_.get() );
    llvm::Value* lvalue = tncg_->getAddr(0); // TODO

    if ( s->tuple_->size() != 1)
    {
        swiftAssert( s->exprList_->size() == 1, "must exactly have one item" );
        MemberFctCall* call = llvm::cast<MemberFctCall>( 
                s->exprList_->getTypeNode(0) );

        call->setTuple(s->tuple_);
        call->accept( tncg_.get() );
    }
    else
    {
        s->exprList_->accept( tncg_.get() );
        llvm::Value* rvalue = tncg_->getScalar(0); // TODO
        ctxt_->builder_.CreateStore(rvalue, lvalue);
    }
}

void StmntCodeGen::visit(ExprStmnt* s)
{
    s->expr_->accept( tncg_.get() );
}

} // namespace swift
