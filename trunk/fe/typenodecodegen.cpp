#include "fe/typenodecodegen.h"

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APSInt.h>

#include "fe/context.h"
#include "fe/class.h"
#include "fe/scope.h"
#include "fe/type.h"
#include "fe/var.h"

namespace swift {

TypeNodeCodeGen::TypeNodeVisitor(Context* ctxt)
    : TypeNodeVisitorBase(ctxt)
{}

void TypeNodeCodeGen::visit(Decl* d)
{
    d->local_->createEntryAlloca(ctxt_);
    llvmValue_ = d->local_->getAddr(ctxt_);
    isAddr_ = true;
}

void TypeNodeCodeGen::visit(Id* id)
{
    Var* var = ctxt_->scope()->lookupVar( id->id() );
    swiftAssert(var, "must be found");
    llvmValue_ = var->getAddr(ctxt_);
    isAddr_ = true;
}

void TypeNodeCodeGen::visit(Literal* l)
{
    using llvm::ConstantInt;
    using llvm::ConstantFP;
    using llvm::APInt;
    using llvm::APFloat;

    llvm::LLVMContext& llvmCtxt = *ctxt_->module_->llvmCtxt_;

    isAddr_ = false;

    switch ( l->getToken() )
    {
        /*
         * signed ints
         */

        case Token::L_SAT8:
        case Token::L_INT8: 
            llvmValue_ = ConstantInt::get(llvmCtxt, APInt( 8, l->box_.int64_, true)); return;

        case Token::L_SAT16:
        case Token::L_INT16:
            llvmValue_ = ConstantInt::get(llvmCtxt, APInt(16, l->box_.int64_, true)); return;

        case Token::L_INT:  
        case Token::L_INT32:
            llvmValue_ = ConstantInt::get(llvmCtxt, APInt(32, l->box_.int64_, true)); return;   

        case Token::L_INT64:
            llvmValue_ = ConstantInt::get(llvmCtxt, APInt(64, l->box_.int64_, true)); return;   

        /*
         * unsigned ints
         */

        case Token::L_USAT8:
        case Token::L_UINT8: 
            llvmValue_ = ConstantInt::get(llvmCtxt, APInt( 8, l->box_.uint64_)); return;   

        case Token::L_USAT16:
        case Token::L_UINT16:
            llvmValue_ = ConstantInt::get(llvmCtxt, APInt(16, l->box_.uint64_)); return;   

        case Token::L_UINT:  
        case Token::L_UINT32:
            llvmValue_ = ConstantInt::get(llvmCtxt, APInt(32, l->box_.uint64_)); return;   

        case Token::L_INDEX:
        case Token::L_UINT64:
            llvmValue_ = ConstantInt::get(llvmCtxt, APInt(64, l->box_.uint64_)); return;   

        /*
         * floats
         */

        case Token::L_REAL:
        case Token::L_REAL32: 
            llvmValue_ = ConstantFP::get(llvmCtxt, APFloat(l->box_.float_) ); return;
        case Token::L_REAL64: 
            llvmValue_ = ConstantFP::get(llvmCtxt, APFloat(l->box_.double_) ); return;

        case Token::L_TRUE:
        case Token::L_FALSE:
            //llvmValue_ = new BaseType(l->loc(), Token::CONST, new std::string("bool")  ); return;

        default:
            swiftAssert(false, "illegal switch-case-value");
    }

}

void TypeNodeCodeGen::visit(Nil* n)
{
    // TODO
}

void TypeNodeCodeGen::visit(Self* n)
{
}

void TypeNodeCodeGen::visit(IndexExpr* i)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    // get ptr of the prefix expr
    i->prefixExpr_->accept(this);
    llvm::Value* ptr = llvmValue_;

    // get index
    i->indexExpr_->accept(this);
    llvm::Value* idx = llvmValue_;

    // build get and get value
    llvmValue_ = builder.CreateGEP(ptr, idx);
    isAddr_ = true;
}

void TypeNodeCodeGen::visit(MemberAccess* m)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    // get ptr of the prefix expr
    m->prefixExpr_->accept(this);
    llvm::Value* ptr = llvmValue_;

    // build input
    llvm::Value* input[2];
    input[0] = llvm::ConstantInt::get( *ctxt_->module_->llvmCtxt_, llvm::APInt(32, 0) );
    input[1] = llvm::ConstantInt::get( *ctxt_->module_->llvmCtxt_, llvm::APInt(32, m->memberVar_->getIndex()) );

    // build get and get value
    llvmValue_ = builder.CreateGEP(ptr, input, input+2);
    isAddr_ = true;
}

void TypeNodeCodeGen::visit(CCall* c)
{
}

void TypeNodeCodeGen::visit(ReaderCall* r)
{
}

void TypeNodeCodeGen::visit(WriterCall* w)
{
}

void TypeNodeCodeGen::visit(BinExpr* b)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    b->op1_->accept(this);
    llvm::Value* v1 = llvmValue_;
    bool isAddr1 = isAddr_;

    b->op2_->accept(this);
    llvm::Value* v2 = llvmValue_;
    bool isAddr2 = isAddr_;

    if ( b->op1_->getType()->isBuiltin() )
    {
        const BaseType* bt = (BaseType*) b->op1_->getType();

        if (isAddr1)
            v1 = builder.CreateLoad(v1);
        if (isAddr2)
            v2 = builder.CreateLoad(v2);

        isAddr_ = false;

        switch (b->token_)
        {
            /*
             * arithmetic operators
             */

            case Token::ADD: llvmValue_ = builder.CreateAdd(v1, v2); return;
            case Token::SUB: llvmValue_ = builder.CreateSub(v1, v2); return;
            case Token::MUL: llvmValue_ = builder.CreateMul(v1, v2); return;
            case Token::DIV:
                if ( bt->isFloat() )
                    llvmValue_ = builder.CreateFDiv(v1, v2);
                else
                {
                    if ( bt->isSigned() )
                        llvmValue_ = builder.CreateSDiv(v1, v2);
                    else // -> unsigned
                        llvmValue_ = builder.CreateUDiv(v1, v2);
                }
                return;

            /*
             * comparisons
             */

            case Token::EQ:
                if ( bt->isFloat() )
                    llvmValue_ = builder.CreateFCmpOEQ(v1, v2);
                else
                    llvmValue_ = builder.CreateICmpEQ(v1, v2);
                return;
            case Token::NE:
                if ( bt->isFloat() )
                    llvmValue_ = builder.CreateFCmpONE(v1, v2);
                else
                    llvmValue_ = builder.CreateICmpNE(v1, v2);
                return;

#define SWIFT_EMIT_CMP(token, fcmp, scmp, ucmp) \
    case Token:: token : \
        if ( bt->isFloat() ) \
            llvmValue_ = builder. fcmp (v1, v2); \
        else \
        { \
            if ( bt->isSigned() ) \
                llvmValue_ = builder. scmp (v1, v2); \
            else \
                llvmValue_ = builder. ucmp (v1, v2); \
        } \
        return;

            SWIFT_EMIT_CMP(LT, CreateFCmpOLT, CreateICmpSLT, CreateICmpULT);
            SWIFT_EMIT_CMP(LE, CreateFCmpOLE, CreateICmpSLE, CreateICmpULE);
            SWIFT_EMIT_CMP(GT, CreateFCmpOGT, CreateICmpSGT, CreateICmpUGT);
            SWIFT_EMIT_CMP(GE, CreateFCmpOGE, CreateICmpSGE, CreateICmpUGE);

#undef SWIFT_EMIT_CMP

            default:
                swiftAssert(false, "TODO");
        }
    }
    // -> is not builtin

    swiftAssert(false, "TODO");
    isAddr_ = true;
}

void TypeNodeCodeGen::visit(RoutineCall* r)
{
}

void TypeNodeCodeGen::visit(UnExpr* u)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    u->op1_->accept(this);
    llvm::Value* v1 = llvmValue_;
    bool isAddr1 = isAddr_;

    if ( u->op1_->getType()->isBuiltin() )
    {
        const BaseType* bt = (BaseType*) u->op1_->getType();

        if (isAddr1)
            v1 = builder.CreateLoad(v1);

        isAddr_ = false;

        switch (u->token_)
        {
            case Token::ADD: 
                // nothing to do
                return;
            case Token::SUB:
                llvmValue_ = builder.CreateSub(
                        llvm::Constant::getNullValue(bt->getLLVMType(ctxt_->module_)), 
                        v1);
                return;
            default:
                swiftAssert(false, "TODO");
        }
    }
    // -> is not builtin

    swiftAssert(false, "TODO");
    isAddr_ = true;
}

llvm::Value* TypeNodeCodeGen::getLLVMValue() const
{
    return llvmValue_;
}

bool TypeNodeCodeGen::isAddr() const
{
    return isAddr_;
}

llvm::Value* TypeNodeCodeGen::getScalar()
{
    if (isAddr_)
        return ctxt_->builder_.CreateLoad(llvmValue_);
    else
        return llvmValue_;
}

} // namespace swift
