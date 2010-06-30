#include "fe/stmntcodegen.h"

#include <typeinfo>

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

using llvm::Value;

namespace swift {

StmntCodeGen::StmntVisitor(Context* ctxt)
    : StmntVisitorBase(ctxt)
    , builder_(ctxt->builder_)
    , lctxt_( ctxt->lctxt() )
{}

StmntCodeGen::~StmntVisitor() {}

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
    TypeNodeCodeGen tncg(ctxt_);
    s->decl_->accept(&tncg);
}

void StmntCodeGen::visit(IfElStmnt* s)
{
    llvm::Function* llvmFct = ctxt_->llvmFct_;

    TypeNodeCodeGen tncg(ctxt_);
    s->expr_->accept(&tncg);
    Value* cond = tncg.getPlace()->getScalar(builder_);

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
    StmntCodeGen scg(ctxt_);
    s->ifScope_->accept(&scg);
    builder_.CreateBr(mergeBB);

    /*
     * emit code for elseBB if applicable
     */

    if (elseBB)
    {
        llvmFct->getBasicBlockList().push_back(elseBB);
        builder_.SetInsertPoint(elseBB);
        StmntCodeGen scg(ctxt_);
        s->elScope_->accept(&scg);
        builder_.CreateBr(mergeBB);
    }

    llvmFct->getBasicBlockList().push_back(mergeBB);
    builder_.SetInsertPoint(mergeBB);
}

void StmntCodeGen::visit(RepeatUntilLoop* l)
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
    StmntCodeGen scg(ctxt_);
    l->scope_->accept(&scg);
    TypeNodeCodeGen tncg(ctxt_);
    l->expr_->accept(&tncg);
    Value* cond = tncg.getPlace()->getScalar(builder_);
    builder_.CreateCondBr(cond, outBB, loopBB);

    /*
     * emit code for outBB
     */

    llvmFct->getBasicBlockList().push_back(outBB);
    builder_.SetInsertPoint(outBB);
}

void StmntCodeGen::visit(WhileLoop* l)
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
    TypeNodeCodeGen tncg(ctxt_);
    l->expr_->accept(&tncg);
    Value* cond = tncg.getPlace()->getScalar(builder_);
    builder_.CreateCondBr(cond, loopBB, outBB);

    /*
     * emit code for loopBB
     */

    llvmFct->getBasicBlockList().push_back(loopBB);
    builder_.SetInsertPoint(loopBB);
    StmntCodeGen scg(ctxt_);
    l->scope_->accept(&scg);
    builder_.CreateBr(headerBB);

    /*
     * emit code for outBB
     */

    llvmFct->getBasicBlockList().push_back(outBB);
    builder_.SetInsertPoint(outBB);
}

void StmntCodeGen::visit(SimdLoop* l)
{
    llvm::Function* llvmFct = ctxt_->llvmFct_;

    /*
     * create new basic blocks
     */

    typedef llvm::BasicBlock BB;
    BB* headerBB = llvm::BasicBlock::Create(lctxt_, "simd-header");
    BB* loopBB   = llvm::BasicBlock::Create(lctxt_, "simd");
    BB*  outBB   = llvm::BasicBlock::Create(lctxt_, "simd-out");

    /*
     * close current bb
     */

    // init loop index
    ctxt_->simdIndex_ = createEntryAlloca( 
            builder_, llvm::IntegerType::getInt64Ty(lctxt_), "simdindex" );

    TypeNodeCodeGen lTncg(ctxt_);
    l->lExpr_->accept(&lTncg);
    builder_.CreateStore( lTncg.getPlace()->getScalar(builder_), ctxt_->simdIndex_ );

    TypeNodeCodeGen rTncg(ctxt_);
    l->rExpr_->accept(&rTncg);
    Value* upper = rTncg.getPlace()->getScalar(builder_);


    builder_.CreateBr(headerBB);

    /*
     * emit code for headerBB
     */

    llvmFct->getBasicBlockList().push_back(headerBB);
    builder_.SetInsertPoint(headerBB);

    Value* lower = builder_.CreateLoad(ctxt_->simdIndex_);

    Value* cond = builder_.CreateICmpULT(lower, upper);
    builder_.CreateCondBr(cond, loopBB, outBB);

    /*
     * emit code for loopBB
     */

    llvmFct->getBasicBlockList().push_back(loopBB);
    builder_.SetInsertPoint(loopBB);
    StmntCodeGen scg(ctxt_);
    l->scope_->accept(&scg);

    builder_.CreateStore( builder_.CreateAdd( builder_.CreateLoad(ctxt_->simdIndex_), ::createInt64(lctxt_, 1) ), ctxt_->simdIndex_ );
    builder_.CreateBr(headerBB);

    /*
     * emit code for outBB
     */

    llvmFct->getBasicBlockList().push_back(outBB);
    builder_.SetInsertPoint(outBB);
}

void StmntCodeGen::visit(ScopeStmnt* s) 
{
    StmntCodeGen scg(ctxt_);
    s->scope_->accept(&scg);
}

void StmntCodeGen::visit(AssignStmnt* s)
{
    typedef AssignStmnt::Call ASCall;

    TypeNodeCodeGen rTncg(ctxt_);
    s->exprList_->accept(rTncg);
    size_t numLhs = s->tuple_->numItems();

    switch (s->kind_)
    {
        case AssignStmnt::CREATE:
        case AssignStmnt::ASSIGN:
        {
            TypeNodeCodeGen lTncg(ctxt_);
            s->tuple_->accept(lTncg);
            swiftAssert(s->tuple_->numRetValues() == 1 && numLhs == 1, 
                    "there must be exactly one item and "
                    "exactly one return value on the lhs");
            Place* lPlace = s->tuple_->getPlace(0);
            ASCall& call = s->calls_[0];

            swiftAssert(call.kind_ == ASCall::USER, "sole possibility"); // TODO
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
                    args.push_back( lPlace->getAddr(builder_) );
                    // push the rest
                    s->exprList_->getArgs(args, builder_);

                    // create call
                    llvm::CallInst* call = 
                        llvm::CallInst::Create( llvmFct, args.begin(), args.end() );
                    call->setCallingConv(llvm::CallingConv::Fast);
                    builder_.Insert(call);
                    lPlace->writeBack(builder_);

                    return;
                }
                case ASCall::COPY:
                {
                    swiftAssert(s->exprList_->numRetValues() == 1, 
                            "only a copy constructor/assignment is in question here");

                    Value* rvalue = s->exprList_->getPlace(0)->getScalar(builder_);
                    Value* lvalue = lPlace->getAddr(builder_);
                    builder_.CreateStore(rvalue, lvalue);
                    lPlace->writeBack(builder_);

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

                if ( Decl* decl = dynamic<Decl>(s->tuple_->getTypeNode(i)) )
                {
                    Place* rPlace = s->exprList_->getPlace(i);
                    swiftAssert( typeid(*rPlace) == typeid(Addr), "must be an Addr" );
                    llvm::AllocaInst* alloca = 
                        cast<llvm::AllocaInst>( rPlace->getAddr(builder_) );
                    decl->setAlloca(alloca);
                }
            }

            // now emit code for the lhs
            TypeNodeCodeGen lTncg(ctxt_);
            s->tuple_->accept(lTncg);
            swiftAssert( s->tuple_->numRetValues() == numLhs, "sizes must match" );
            swiftAssert( s->calls_.size() == numLhs, "sizes must match" );
            size_t& num = numLhs;

            Values args(2);
            for (size_t i = 0; i < num; ++i)
            {
                ASCall& call = s->calls_[i];
                Place* lPlace = s->tuple_->getPlace(i);
                Place* rPlace = s->exprList_->getPlace(i);

                switch (call.kind_)
                {
                    case ASCall::EMPTY:
                    case ASCall::GETS_INITIALIZED_BY_CALLER:
                        continue;

                    case ASCall::COPY:
                    {
                        // create store
                        Value* lvalue = lPlace->getAddr(builder_);
                        Value* rvalue = rPlace->getScalar(builder_);
                        builder_.CreateStore(rvalue, lvalue);
                        lPlace->writeBack(builder_);

                        continue;
                    }
                    case ASCall::USER:
                    {
                        llvm::Function* llvmFct = call.fct_->llvmFct_;
                        swiftAssert(llvmFct, "must be valid");

                        // set arg
                        args[0] = s->tuple_->getArg(i, builder_);
                        args[1] = s->exprList_->getArg(i, builder_);

                        // create call
                        llvm::CallInst* call = 
                            llvm::CallInst::Create( llvmFct, args.begin(), args.end() );
                        call->setCallingConv(llvm::CallingConv::Fast);
                        builder_.Insert(call);
                        s->tuple_->getPlace(i)->writeBack(builder_);
                        continue;
                    }
                    case ASCall::CONTAINER_COPY:
                    {
                        const Container* c = 
                            cast<Container>( s->tuple_->getTypeNode(i)->getType() );

                        Value* dst = lPlace->getAddr(builder_);
                        Value* src = rPlace->getAddr(builder_);
                        c->emitCopy(ctxt_, dst, src);
                        continue;
                    }
                    case ASCall::CONTAINER_CREATE:
                    {
                        const Container* c = 
                            cast<Container>( s->tuple_->getTypeNode(i)->getType() );

                        Value* lvalue = lPlace->getAddr(builder_);
                        Value* size = rPlace->getScalar(builder_);
                        c->emitCreate(ctxt_, lvalue, size);
                        continue;
                    }
                }
            } // for each lhs/rhs pair
        } // case ASCall::PAIRWISE
    } // switch (s->kind_)
}

void StmntCodeGen::visit(ExprStmnt* s)
{
    TypeNodeCodeGen tncg(ctxt_);
    s->expr_->accept(&tncg);
}

} // namespace swift
