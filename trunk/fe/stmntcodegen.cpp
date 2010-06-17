#include "fe/stmntcodegen.h"

#include <llvm/BasicBlock.h>
#include <llvm/Function.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>

#include "utils/cast.h"

#include "fe/context.h"
#include "fe/class.h"
#include "fe/tnlist.h"
#include "fe/scope.h"
#include "fe/type.h"
#include "fe/typenodecodegen.h"

namespace swift {

StmntCodeGen::StmntVisitor(Context* ctxt)
    : StmntVisitorBase(ctxt)
    , tncg_( new TypeNodeCodeGen(ctxt) )
    , builder_(ctxt->builder_)
    , lctxt_( ctxt->lctxt() )
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
    MemberFct* memberFct = ctxt_->memberFct_;

    if (s->token_ == Token::RETURN)
    {
        if ( memberFct->sig_.out_.empty() )
            builder_.CreateRetVoid();
        else
            builder_.CreateBr(ctxt_->memberFct_->returnBB_);

        llvm::BasicBlock* bb = llvm::BasicBlock::Create(lctxt_, "unreachable");
        ctxt_->llvmFct_->getBasicBlockList().push_back(bb);
        builder_.SetInsertPoint(bb);

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
    llvm::Function* llvmFct = ctxt_->llvmFct_;

    s->expr_->accept( tncg_.get() );
    llvm::Value* cond = tncg_->getScalar(0);

    /*
     * create new basic blocks
     */

    typedef llvm::BasicBlock BB;
    BB* thenBB  = llvm::BasicBlock::Create(lctxt_, "then");
    BB* mergeBB = llvm::BasicBlock::Create(lctxt_, "merge");
    BB* elseBB = s->elScope_ ? llvm::BasicBlock::Create(lctxt_, "else") : 0;

    /*
     * create branch
     */

    if (elseBB)
        builder_.CreateCondBr(cond, thenBB, elseBB);
    else
        builder_.CreateCondBr(cond, thenBB, mergeBB);

    /*
     * emit code for thenBB
     */

    llvmFct->getBasicBlockList().push_back(thenBB);
    builder_.SetInsertPoint(thenBB);
    s->ifScope_->accept(this, ctxt_);
    builder_.CreateBr(mergeBB);

    /*
     * emit code for elseBB if applicable
     */

    if (elseBB)
    {
        llvmFct->getBasicBlockList().push_back(elseBB);
        builder_.SetInsertPoint(elseBB);
        s->elScope_->accept(this, ctxt_);
        builder_.CreateBr(mergeBB);
    }

    llvmFct->getBasicBlockList().push_back(mergeBB);
    builder_.SetInsertPoint(mergeBB);
}

void StmntCodeGen::visit(RepeatUntilStmnt* s)
{
    llvm::Function* llvmFct = ctxt_->llvmFct_;

    /*
     * create new basic blocks
     */

    typedef llvm::BasicBlock BB;
    BB* loopBB   = llvm::BasicBlock::Create(lctxt_, "rep");
    BB*  outBB   = llvm::BasicBlock::Create(lctxt_, "rep-out");

    /*
     * close current bb
     */

    builder_.CreateBr(loopBB);

    /*
     * emit code for loopBB
     */

    llvmFct->getBasicBlockList().push_back(loopBB);
    builder_.SetInsertPoint(loopBB);
    s->scope_->accept(this, ctxt_);
    s->expr_->accept( tncg_.get() );
    llvm::Value* cond = tncg_->getScalar(0);
    builder_.CreateCondBr(cond, outBB, loopBB);

    /*
     * emit code for outBB
     */

    llvmFct->getBasicBlockList().push_back(outBB);
    builder_.SetInsertPoint(outBB);
}

void StmntCodeGen::visit(ScopeStmnt* s) 
{
    s->scope_->accept(this, ctxt_);
}

void StmntCodeGen::visit(WhileStmnt* s)
{
    llvm::Function* llvmFct = ctxt_->llvmFct_;

    /*
     * create new basic blocks
     */

    typedef llvm::BasicBlock BB;
    BB* headerBB = llvm::BasicBlock::Create(lctxt_, "while-header");
    BB* loopBB   = llvm::BasicBlock::Create(lctxt_, "while");
    BB*  outBB   = llvm::BasicBlock::Create(lctxt_, "while-out");

    /*
     * close current bb
     */

    builder_.CreateBr(headerBB);

    /*
     * emit code for headerBB
     */

    llvmFct->getBasicBlockList().push_back(headerBB);
    builder_.SetInsertPoint(headerBB);
    s->expr_->accept( tncg_.get() );
    llvm::Value* cond = tncg_->getScalar(0);
    builder_.CreateCondBr(cond, loopBB, outBB);

    /*
     * emit code for loopBB
     */

    llvmFct->getBasicBlockList().push_back(loopBB);
    builder_.SetInsertPoint(loopBB);
    s->scope_->accept(this, ctxt_);
    builder_.CreateBr(headerBB);

    /*
     * emit code for outBB
     */

    llvmFct->getBasicBlockList().push_back(outBB);
    builder_.SetInsertPoint(outBB);
}

void StmntCodeGen::visit(AssignStmnt* s)
{
    typedef AssignStmnt::Call ASCall;

    s->exprList_->accept( tncg_.get() );
    size_t numLhs = s->tuple_->numItems();

    switch (s->kind_)
    {
        case AssignStmnt::CREATE:
        case AssignStmnt::ASSIGN:
        {
            s->tuple_->accept( tncg_.get() );
            swiftAssert(s->tuple_->numRetValues() == 1 && numLhs == 1, 
                    "there must be exactly one item and "
                    "exactly one return value on the lhs");

            ASCall& call = s->calls_[0];

            switch (call.kind_)
            {
                case ASCall::EMPTY:
                    return; // do nothing
                case ASCall::USER:
                {
                    llvm::Function* llvmFct = call.fct_->llvmFct_;
                    swiftAssert(llvmFct, "must be valid");

                    Values args;
                    // push self arg
                    args.push_back( s->tuple_->getAddr(0, ctxt_) );
                    // push the rest
                    s->exprList_->getArgs(args, ctxt_);

                    // create call
                    llvm::CallInst* call = 
                        llvm::CallInst::Create( llvmFct, args.begin(), args.end() );
                    call->setCallingConv(llvm::CallingConv::Fast);
                    builder_.Insert(call);

                    return;
                }
                case ASCall::COPY:
                {
                    swiftAssert(s->exprList_->numRetValues() == 1, 
                            "only a copy constructor/assignment is in question here");

                    llvm::Value* rvalue = s->exprList_->getScalar(0, builder_);
                    llvm::Value* lvalue = s->tuple_->getAddr(0, ctxt_);
                    builder_.CreateStore(rvalue, lvalue);

                    return;
                }
                default:
                    swiftAssert(false, "unreachable"); 
            }
        }

        case AssignStmnt::PAIRWISE:
        {
            swiftAssert( s->exprList_->numRetValues() == numLhs, "sizes must match" );

            // set initial value for created decls
            for (size_t i = 0; i < numLhs; ++i)
            {
                if ( !s->exprList_->isInit(i) )
                    continue;

                if ( Decl* decl = dynamic_cast<Decl*>(s->tuple_->getTypeNode(i)) )
                    decl->setAlloca( cast<llvm::AllocaInst>(s->exprList_->getValue(i)) );
            }

            // now emit code for the lhs
            s->tuple_->accept( tncg_.get() );
            swiftAssert( s->tuple_->numRetValues() == numLhs, "sizes must match" );
            swiftAssert( s->calls_.size() == numLhs, "sizes must match" );
            size_t& num = numLhs;

            Values args(2);
            for (size_t i = 0; i < num; ++i)
            {
                ASCall& call = s->calls_[i];

                switch (call.kind_)
                {
                    case ASCall::EMPTY:
                    case ASCall::GETS_INITIALIZED_BY_CALLER:
                        continue;

                    case ASCall::COPY:
                    {
                        // create store
                        llvm::Value* rvalue = s->exprList_->getScalar(i, builder_);
                        llvm::Value* lvalue = s->tuple_->getAddr(i, ctxt_);
                        builder_.CreateStore(rvalue, lvalue);
                        continue;
                    }
                    case ASCall::USER:
                    {
                        llvm::Function* llvmFct = call.fct_->llvmFct_;
                        swiftAssert(llvmFct, "must be valid");

                        // set arg
                        args[0] = s->tuple_->getArg(0, ctxt_);
                        args[1] = s->exprList_->getArg(i, ctxt_);

                        // create call
                        llvm::CallInst* call = 
                            llvm::CallInst::Create( llvmFct, args.begin(), args.end() );
                        call->setCallingConv(llvm::CallingConv::Fast);
                        builder_.Insert(call);
                        continue;
                    }
                    case ASCall::CONTAINER_COPY:
                    {
                        const Container* c = 
                            cast<Container>( s->tuple_->getTypeNode(i)->getType() );

                        llvm::Value* dst = s->tuple_->getAddr(i, ctxt_);
                        llvm::Value* src = s->exprList_->getAddr(i, ctxt_);
                        c->emitCopy(ctxt_, dst, src);
                        continue;
                    }
                    case ASCall::CONTAINER_CREATE:
                    {
                        const Container* c = 
                            cast<Container>( s->tuple_->getTypeNode(i)->getType() );

                        llvm::Value* size = s->exprList_->getScalar(i, builder_);
                        llvm::Value* lvalue = s->tuple_->getAddr(i, ctxt_);
                        c->emitCreate(ctxt_, lvalue, size);
                        continue;
                    }
                }
            } // fo each lhs/rhs pair
        } // case ASCall::PAIRWISE
    } // switch (s->kind_)
}

void StmntCodeGen::visit(ExprStmnt* s)
{
    s->expr_->accept( tncg_.get() );
}

} // namespace swift
