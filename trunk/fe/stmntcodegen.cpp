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
    l->scope_->accept(this);
    l->expr_->accept(tncg_);
    Value* cond = l->expr_->get().place_->getScalar(builder_);
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
    l->expr_->accept(tncg_);
    Value* cond = l->expr_->get().place_->getScalar(builder_);
    builder_.CreateCondBr(cond, loopBB, outBB);

    /*
     * emit code for loopBB
     */

    llvmFct->getBasicBlockList().push_back(loopBB);
    builder_.SetInsertPoint(loopBB);
    l->scope_->accept(this);
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

    l->lExpr_->accept(tncg_);
    builder_.CreateStore( l->lExpr_->get().place_->getScalar(builder_), ctxt_->simdIndex_ );

    l->rExpr_->accept(tncg_);
    Value* upper = l->rExpr_->get().place_->getScalar(builder_);


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
    l->scope_->accept(this);

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
    s->scope_->accept(this);
}

void StmntCodeGen::visit(AssignStmnt* s)
{
    typedef AssignStmnt::Call ASCall;

    TNList& lhs = *s->tuple_;
    TNList& rhs = *s->exprList_;

    size_t numLhs = s->tuple_->numTypeNodes();

    rhs.accept(tncg_);

    switch (s->kind_)
    {
        case AssignStmnt::CREATE:
        case AssignStmnt::ASSIGN:
        {
            lhs.accept(tncg_);
            swiftAssert(lhs.numResults() == 1 && numLhs == 1, 
                    "there must be exactly one item and "
                    "exactly one return value on the lhs");

            const TNResult& lRes = lhs.getResult(0);
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
                    args.push_back( lRes.place_->getAddr(builder_) );
                    // push the rest
                    rhs.getArgs(args, builder_);

                    // create call
                    llvm::CallInst* call = 
                        llvm::CallInst::Create( llvmFct, args.begin(), args.end() );
                    call->setCallingConv(llvm::CallingConv::Fast);
                    builder_.Insert(call);
                    lRes.place_->writeBack(builder_);

                    return;
                }
                case ASCall::COPY:
                {
                    swiftAssert(rhs.numResults() == 1, 
                            "only a copy constructor/assignment is in question here");
                    const TNResult& rRes = rhs.getResult(0);

                    Value* rvalue = rRes.place_->getScalar(builder_);
                    Value* lvalue = lRes.place_->getAddr(builder_);
                    builder_.CreateStore(rvalue, lvalue);
                    lRes.place_->writeBack(builder_);

                    return;
                }
                default:
                    swiftAssert(false, "unreachable"); 
            }
        }

        case AssignStmnt::PAIRWISE:
        {
            swiftAssert( rhs.numResults() == numLhs, "sizes must match" );

            // set initial value for created decls
            // TODO do this the other way round and pass these values to the constructors
            for (size_t i = 0; i < numLhs; ++i)
            {
                if ( !rhs.getResult(i).inits_ )
                    continue;

                if ( Decl* decl = dynamic<Decl>(lhs.getTypeNode(i)) )
                {
                    Place* rPlace = rhs.getResult(i).place_;
                    swiftAssert( typeid(*rPlace) == typeid(Addr), "must be an Addr" );
                    llvm::AllocaInst* alloca = 
                        cast<llvm::AllocaInst>( rPlace->getAddr(builder_) );
                    decl->setAlloca(alloca);
                }
            }

            // now emit code for the lhs
            lhs.accept(tncg_);
            swiftAssert( lhs.numResults() == numLhs, "sizes must match" );
            swiftAssert( s->calls_.size() == numLhs, "sizes must match" );
            size_t& num = numLhs;

            Values args(2);
            for (size_t i = 0; i < num; ++i)
            {
                ASCall& call = s->calls_[i];
                const TNResult& lRes = lhs.getResult(i);
                const TNResult& rRes = rhs.getResult(i);

                switch (call.kind_)
                {
                    case ASCall::EMPTY:
                    case ASCall::GETS_INITIALIZED_BY_CALLER:
                        continue;

                    case ASCall::COPY:
                    {
                        // create store
                        Value* lvalue = lRes.place_->getAddr(builder_);
                        Value* rvalue = rRes.place_->getScalar(builder_);
                        builder_.CreateStore(rvalue, lvalue);
                        lRes.place_->writeBack(builder_);

                        continue;
                    }
                    case ASCall::USER:
                    {
                        llvm::Function* llvmFct = call.fct_->llvmFct_;
                        swiftAssert(llvmFct, "must be valid");

                        // set arg
                        args[0] = lhs.getArg(i, builder_);
                        args[1] = rhs.getArg(i, builder_);

                        // create call
                        llvm::CallInst* call = 
                            llvm::CallInst::Create( llvmFct, args.begin(), args.end() );
                        call->setCallingConv(llvm::CallingConv::Fast);
                        builder_.Insert(call);
                        lRes.place_->writeBack(builder_);
                        continue;
                    }
                    case ASCall::CONTAINER_COPY:
                    {
                        const Container* c = cast<Container>( lRes.type_ );

                        Value* dst = lRes.place_->getAddr(builder_);
                        Value* src = rRes.place_->getAddr(builder_);
                        c->emitCopy(ctxt_, dst, src);
                        continue;
                    }
                    case ASCall::CONTAINER_CREATE:
                    {
                        const Container* c = cast<Container>( lRes.type_ );

                        Value* lvalue = lRes.place_->getAddr(builder_);
                        Value* size = rRes.place_->getScalar(builder_);
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
    s->expr_->accept(tncg_);
}

} // namespace swift
