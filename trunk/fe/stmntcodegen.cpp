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
    , tncg_( new TypeNodeCodeGen(ctxt) )
{}

StmntCodeGen::~StmntVisitor() {}

void StmntCodeGen::visit(ErrorStmnt* s) 
{
    swiftAssert(false, "unreachable");
}

void StmntCodeGen::visit(CFStmnt* s)
{
    MemberFct* memberFct = ctxt_->memberFct_;

    switch (s->token_)
    {
        case Token::RETURN:
        {
            if ( memberFct->sig().out_.empty() )
                builder_.CreateRetVoid();
            else
                builder_.CreateBr(ctxt_->memberFct_->returnBB());
            break;

            return;
        }
        case Token::BREAK:
        {
            builder_.CreateBr( ctxt_->currentLoop_->getOutBB() );
            break;
        }
        case Token::CONTINUE:
        {
            builder_.CreateBr( ctxt_->currentLoop_->getLoopBB() );
            break;
        }
        default:
            swiftAssert(false, "unreachable");
    }

    llvm::BasicBlock* bb = llvm::BasicBlock::Create(lctxt_, "unreachable");
    ctxt_->llvmFct_->getBasicBlockList().push_back(bb);
    builder_.SetInsertPoint(bb);
}

void StmntCodeGen::visit(DeclStmnt* s)
{
    s->decl_->accept(tncg_);
}

void StmntCodeGen::visit(IfElStmnt* s)
{
    llvm::Function* llvmFct = ctxt_->llvmFct_;

    s->expr_->accept(tncg_);
    Value* cond = s->expr_->get().place_->getScalar(builder_);

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
    s->ifScope_->accept(this);
    builder_.CreateBr(mergeBB);

    /*
     * emit code for elseBB if applicable
     */

    if (elseBB)
    {
        llvmFct->getBasicBlockList().push_back(elseBB);
        builder_.SetInsertPoint(elseBB);
        s->elScope_->accept(this);
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

    l->loopBB_ = llvm::BasicBlock::Create(lctxt_, "rep");
    l->outBB_  = llvm::BasicBlock::Create(lctxt_, "rep-out");

    /*
     * close current bb
     */

    builder_.CreateBr(l->loopBB_);

    /*
     * emit code for l->loopBB_
     */

    llvmFct->getBasicBlockList().push_back(l->loopBB_);
    builder_.SetInsertPoint(l->loopBB_);
    //l->scope_->accept(this);
    SWIFT_ENTER_LOOP;
    l->expr_->accept(tncg_);
    Value* cond = l->expr_->get().place_->getScalar(builder_);
    builder_.CreateCondBr(cond, l->outBB_, l->loopBB_);

    /*
     * emit code for l->outBB_
     */

    llvmFct->getBasicBlockList().push_back(l->outBB_);
    builder_.SetInsertPoint(l->outBB_);
}

void StmntCodeGen::visit(WhileLoop* l)
{
    llvm::Function* llvmFct = ctxt_->llvmFct_;

    /*
     * create new basic blocks
     */

    l->headerBB_ = llvm::BasicBlock::Create(lctxt_, "while-header");
    l->loopBB_   = llvm::BasicBlock::Create(lctxt_, "while");
    l->outBB_    = llvm::BasicBlock::Create(lctxt_, "while-out");

    /*
     * close current bb
     */

    builder_.CreateBr(l->headerBB_);

    /*
     * emit code for l->headerBB_
     */

    llvmFct->getBasicBlockList().push_back(l->headerBB_);
    builder_.SetInsertPoint(l->headerBB_);
    l->expr_->accept(tncg_);
    Value* cond = l->expr_->get().place_->getScalar(builder_);
    builder_.CreateCondBr(cond, l->loopBB_, l->outBB_);

    /*
     * emit code for l->loopBB_
     */

    llvmFct->getBasicBlockList().push_back(l->loopBB_);
    builder_.SetInsertPoint(l->loopBB_);
    //l->scope_->accept(this);
    SWIFT_ENTER_LOOP;
    builder_.CreateBr(l->headerBB_);

    /*
     * emit code for l->outBB_
     */

    llvmFct->getBasicBlockList().push_back(l->outBB_);
    builder_.SetInsertPoint(l->outBB_);
}

void StmntCodeGen::visit(SimdLoop* l)
{
    llvm::Function* llvmFct = ctxt_->llvmFct_;

    /*
     * create new basic blocks
     */

    l->headerBB_ = llvm::BasicBlock::Create(lctxt_, "simd-header");
    l->loopBB_   = llvm::BasicBlock::Create(lctxt_, "simd");
    l->outBB_    = llvm::BasicBlock::Create(lctxt_, "simd-out");

    //ctxt_->memberFct_->


    /*
     * close current bb
     */

    // init loop index
    ctxt_->simdIndex_ = createEntryAlloca( 
            builder_, 
            llvm::IntegerType::getInt64Ty(lctxt_), 
            l->id_ ? l->id_->c_str() : "simdindex" );
    l->index_->setAlloca( cast<llvm::AllocaInst>(ctxt_->simdIndex_) );

    l->lExpr_->accept(tncg_);
    builder_.CreateStore( l->lExpr_->get().place_->getScalar(builder_), ctxt_->simdIndex_ );

    l->rExpr_->accept(tncg_);
    Value* upper = l->rExpr_->get().place_->getScalar(builder_);


    builder_.CreateBr(l->headerBB_);

    /*
     * emit code for l->headerBB_
     */

    llvmFct->getBasicBlockList().push_back(l->headerBB_);
    builder_.SetInsertPoint(l->headerBB_);

    Value* lower = builder_.CreateLoad(ctxt_->simdIndex_);

    Value* cond = builder_.CreateICmpULT(lower, upper);
    builder_.CreateCondBr(cond, l->loopBB_, l->outBB_);

    /*
     * emit code for l->loopBB_
     */

    llvmFct->getBasicBlockList().push_back(l->loopBB_);
    builder_.SetInsertPoint(l->loopBB_);
    l->scope_->accept(this);

    builder_.CreateStore( 
            builder_.CreateAdd( builder_.CreateLoad(ctxt_->simdIndex_), 
            ::createInt64(lctxt_, 4) ), ctxt_->simdIndex_ ); // HACK
    builder_.CreateBr(l->headerBB_);

    /*
     * emit code for l->outBB_
     */

    llvmFct->getBasicBlockList().push_back(l->outBB_);
    builder_.SetInsertPoint(l->outBB_);
}

void StmntCodeGen::visit(ScopeStmnt* s) 
{
    s->scope_->accept(this);
}

void StmntCodeGen::visit(AssignStmnt* s)
{
    s->tuple_->accept(tncg_);

    switch (s->kind_)
    {
        case AssignStmnt::SINGLE:
        {
            s->exprList_->accept(tncg_);
            s->acs_[0].genCode();
            return;
        }
        case AssignStmnt::PAIRWISE:
        {
            // propagate inits for return value optimization
            size_t left = 0; // iterates over rTN's corresponding lhs items
            // for each rhs TypeNode
            for (size_t i = 0; i < s->exprList_->numTypeNodes(); ++i)
            {
                TypeNode* rTN = s->exprList_->getTypeNode(i);


                Places places;
                // for each result of rTN
                for (size_t j = 0; j < rTN->numResults(); ++j, ++left)
                {
                    TypeNode* lTN = s->tuple_->getTypeNode(left);

                    Place* place = s->acs_[left].initsRhs() ? lTN->get().place_ : 0;
                    places.push_back(place);
                }

                if ( MemberFctCall* call = dynamic<MemberFctCall>(rTN) )
                    call->initPlaces_ = &places;
                    
                rTN->accept(tncg_);
            }

            for (size_t i = 0; i < s->acs_.size(); ++i)
                s->acs_[i].genCode();

            return;
        }
    }
}

void StmntCodeGen::visit(ExprStmnt* s)
{
    s->expr_->accept(tncg_);
}

} // namespace swift
