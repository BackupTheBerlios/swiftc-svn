#include "fe/stmntcodegen.h"

#include <llvm/BasicBlock.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>

#include "utils/cast.h"

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
    llvm::IRBuilder<>& builder = ctxt_->builder_;

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

            MemberFct* fct = s->fcts_[0];

            if (fct) // a user defined fct is called
            {
                if ( fct->isEmpty() )
                    return; // do nothing

                llvm::Function* llvmFct = fct->llvmFct_;
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
                builder.Insert(call);

                return;
            }
            else // an autogenerated fct i.e. a simple copy will be generated
            {
                swiftAssert(s->exprList_->numRetValues() == 1, 
                        "only a copy constructor/assignment is in question here");

                llvm::Value* rvalue = s->exprList_->getScalar(0, ctxt_->builder_);
                llvm::Value* lvalue = s->tuple_->getAddr(0, ctxt_);
                ctxt_->builder_.CreateStore(rvalue, lvalue);

                return;
            }
        }

        case AssignStmnt::MULTIPLE:
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
            swiftAssert( s->fcts_.size() == numLhs, "sizes must match" );
            size_t& num = numLhs;


            Values args(2);
            for (size_t i = 0; i < num; ++i)
            {
                if ( s->exprList_->isInit(i) 
                        && dynamic_cast<Decl*>(s->tuple_->getTypeNode(i) ) )
                {
                    continue; // this has already been set
                }

                MemberFct* fct = s->fcts_[i];

                if (fct)
                {
                    if ( Create* create = dynamic_cast<Create*>(fct) )
                    {
                        if ( create->isAutoCopy() )
                            goto emit_normal_copy;
                    }
                    else
                    {
                        Assign* assign = cast<Assign>(fct);
                        if ( assign->isAutoCopy() )
                            goto emit_normal_copy;
                    }

                    if ( fct->isEmpty() )
                        continue; // do nothing

                    llvm::Function* llvmFct = fct->llvmFct_;
                    swiftAssert(llvmFct, "must be valid");

                    // set arg
                    args[0] = s->tuple_->getArg(0, ctxt_);
                    args[1] = s->exprList_->getArg(i, ctxt_);

                    // create call
                    llvm::CallInst* call = 
                        llvm::CallInst::Create( llvmFct, args.begin(), args.end() );
                    call->setCallingConv(llvm::CallingConv::Fast);
                    builder.Insert(call);

                    continue;
                }

emit_normal_copy:
                // create store
                llvm::Value* rvalue = s->exprList_->getScalar(i, ctxt_->builder_);
                llvm::Value* lvalue = s->tuple_->getAddr(i, ctxt_);
                builder.CreateStore(rvalue, lvalue);
            }

            return;
        }
    }
}

void StmntCodeGen::visit(ExprStmnt* s)
{
    s->expr_->accept( tncg_.get() );
}

} // namespace swift
