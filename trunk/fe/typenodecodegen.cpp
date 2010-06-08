#include "fe/typenodecodegen.h"

#include <sstream>

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APSInt.h>

#include "fe/context.h"
#include "fe/class.h"
#include "fe/scope.h"
#include "fe/tnlist.h"
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

void TypeNodeCodeGen::visit(ErrorExpr* d)
{
    swiftAssert(false, "unreachable");
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
    Method* m = llvm::cast<Method>(ctxt_->memberFct_);
    llvmValue_ = ctxt_->builder_.CreateLoad( m->getSelfValue(), "self" );
    isAddr_ = true;
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

    // build and get value
    llvmValue_ = builder.CreateInBoundsGEP(idx, ptr);
    isAddr_ = true;
}

void TypeNodeCodeGen::visit(MemberAccess* m)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    // get address of the prefix expr
    m->prefixExpr_->accept(this);
    llvm::Value* addr = getAddr();

    if ( const Ptr* ptr = m->getType()->cast<Ptr>() )
        addr = ptr->recDerefAddr(ctxt_->builder_, addr);

    // build input
    llvm::Value* input[2];
    input[0] = llvm::ConstantInt::get( *ctxt_->module_->llvmCtxt_, llvm::APInt(64, 0) );
    input[1] = llvm::ConstantInt::get( *ctxt_->module_->llvmCtxt_, llvm::APInt(32, m->memberVar_->getIndex()) );

    // build and get value
    std::ostringstream oss;
    oss << m->prefixExpr_->getType()->toString() << '.' << m->cid();
    llvmValue_ = builder.CreateInBoundsGEP( addr, input, input+2, oss.str() );
    isAddr_ = true;
}

void TypeNodeCodeGen::visit(CCall* c)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;
    llvm::Module* llvmModule = ctxt_->module_->getLLVMModule();

    c->exprList_->accept(this);
    const TypeList& inTypes = c->exprList_->typeList();

    /*
     * build function type
     */

    const llvm::Type* retType = c->retType_->getLLVMType(ctxt_->module_);
    std::vector<const llvm::Type*> params( inTypes.size() );

    for (size_t i = 0; i < inTypes.size(); ++i)
        params[i] = inTypes[i]->getLLVMType(ctxt_->module_);

    const llvm::FunctionType* fctType = llvm::FunctionType::get(
            retType, params, false);

    // declare function
    llvm::Function* fct = llvm::cast<llvm::Function>(
        llvmModule->getOrInsertFunction( c->cid(), fctType) );

    // copy over values
    std::vector<llvm::Value*> args( c->exprList_->size() );
    for (size_t i = 0; i < args.size(); ++i)
        args[i] = c->exprList_->getScalar(i, builder);

    fct->setCallingConv(llvm::CallingConv::C);
    fct->setLinkage(llvm::Function::ExternalLinkage);
    //fct->addFnAttr(llvm::Attribute::NoUnwind);
    swiftAssert( fct->isDeclaration(), "may only be a declaration" );

    llvm::CallInst* call = llvm::CallInst::Create( fct, args.begin(), args.end() );
    call->setTailCall();
    call->addAttribute(~0, llvm::Attribute::NoUnwind);

    llvmValue_ = builder.Insert(call);
    isAddr_ = false;
}

void TypeNodeCodeGen::visit(ReaderCall* r)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    getSelf(r);

    if ( const ScalarType* from = r->expr_->getType()->cast<ScalarType>() )
    {
        // -> assumes that r is a cast

        const ScalarType* to = llvm::cast<ScalarType>( r->memberFct_->sig_.out_[0]->getType() );
        const llvm::Type* llvmTo = to->getLLVMType(ctxt_->module_);
        const llvm::Type* llvmFrom = from->getLLVMType(ctxt_->module_);

        if ( llvmTo == llvmFrom )
            return; // llvmValue_ and isAddr_ are still correct, so nothing to do

        if (isAddr_)
            llvmValue_ = builder.CreateLoad(llvmValue_, llvmValue_->getName() );
        isAddr_ = false;

        llvm::StringRef name = llvmValue_->getName();

        if ( from->isInteger() && to->isInteger() )
        {
            if ( from->sizeOf() > to->sizeOf() )
                llvmValue_ = builder.CreateTrunc(llvmValue_, llvmTo, name);
            else
            {
                // -> sizeof(from) < sizeof(to)
                if ( from->isUnsigned() )
                    llvmValue_ = builder.CreateZExt(llvmValue_, llvmTo, name);
                else
                    llvmValue_ = builder.CreateSExt(llvmValue_, llvmTo, name);
            }
        }
        else if ( from->isFloat() && to->isSigned() )   // fp -> si
            llvmValue_ = builder.CreateFPToSI(llvmValue_, llvmTo, name);
        else if ( from->isFloat() && to->isUnsigned() ) // fp -> ui
            llvmValue_ = builder.CreateFPToUI(llvmValue_, llvmTo, name);
        else if ( from->isSigned() && to->isFloat() )   // si -> fp
            llvmValue_ = builder.CreateSIToFP(llvmValue_, llvmTo, name);
        else if ( from->isUnsigned() && to->isFloat() ) // ui -> fp
            llvmValue_ = builder.CreateUIToFP(llvmValue_, llvmTo, name);
        else
        {
            swiftAssert( from->isFloat() && to->isFloat(), "must both be floats" );

            if ( from->sizeOf() > to->sizeOf() )
                llvmValue_ = builder.CreateFPTrunc(llvmValue_, llvmTo, name);
            else // -> sizeof(from) < sizeof(to)
                llvmValue_ = builder.CreateFPExt(llvmValue_, llvmTo, name);
        }
    }
    else
        emitCall(r, llvmValue_);
}

void TypeNodeCodeGen::visit(WriterCall* w)
{
    getSelf(w);
    emitCall(w, llvmValue_);
}

void TypeNodeCodeGen::visit(BinExpr* b)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    if ( b->builtin_ )
    {
        b->op1_->accept(this);
        llvm::Value* v1 = getScalar();
        b->op2_->accept(this);
        llvm::Value* v2 = getScalar();

        const ScalarType* scalar = llvm::cast<ScalarType>( b->op1_->getType() );

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
                if ( scalar->isFloat() )
                    llvmValue_ = builder.CreateFDiv(v1, v2);
                else
                {
                    if ( scalar->isSigned() )
                        llvmValue_ = builder.CreateSDiv(v1, v2);
                    else // -> unsigned
                        llvmValue_ = builder.CreateUDiv(v1, v2);
                }
                return;

            /*
             * comparisons
             */

            case Token::EQ:
                if ( scalar->isFloat() )
                    llvmValue_ = builder.CreateFCmpOEQ(v1, v2);
                else
                    llvmValue_ = builder.CreateICmpEQ(v1, v2);
                return;
            case Token::NE:
                if ( scalar->isFloat() )
                    llvmValue_ = builder.CreateFCmpONE(v1, v2);
                else
                    llvmValue_ = builder.CreateICmpNE(v1, v2);
                return;

#define SWIFT_EMIT_CMP(token, fcmp, scmp, ucmp) \
    case Token:: token : \
        if ( scalar->isFloat() ) \
            llvmValue_ = builder. fcmp (v1, v2); \
        else \
        { \
            if ( scalar->isSigned() ) \
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
    else
        emitCall(b, 0);
}

void TypeNodeCodeGen::visit(UnExpr* u)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    if ( u->builtin_ )
    {
        u->op1_->accept(this);
        llvm::Value* v1 = getScalar();
        const ScalarType* scalar = llvm::cast<ScalarType>( u->op1_->getType() );

        isAddr_ = false;

        switch (u->token_)
        {
            case Token::ADD: 
                // nothing to do
                return;
            case Token::SUB:
                llvmValue_ = builder.CreateSub( 
                        llvm::Constant::getNullValue(
                            scalar->getLLVMType(ctxt_->module_)), 
                        v1 );
                return;
            default:
                swiftAssert(false, "TODO");
        }
    }
    else
        emitCall(u, 0);
}

void TypeNodeCodeGen::visit(RoutineCall* r)
{
    emitCall(r, 0);
}

void TypeNodeCodeGen::getSelf(MethodCall* m)
{
    m->expr_->accept(this);

    if ( const Ptr* ptr = m->expr_->getType()->cast<Ptr>() )
        llvmValue_ = ptr->recDerefAddr(ctxt_->builder_, llvmValue_);
}

void TypeNodeCodeGen::emitCall(MemberFctCall* call, llvm::Value* self)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;
    std::vector<llvm::Value*> args;
    MemberFct* fct = call->getMemberFct();
    const TNList* tuple = call->tuple_;

    call->exprList_->accept(this);
    TypeList& out = fct->sig_.outTypes_;

    /* 
     * prepare arguments
     */

    if (self)
        args.push_back(self);

    llvm::Value* perRefMainRetAlloca = 0;

    // append return-value arguments
    for (size_t i = 0; i < out.size(); ++i)
    {
        const Type* type = out[i];

        if ( type->perRef() )
        {
            llvm::Value* arg;
            if (tuple && dynamic_cast<Decl*>( tuple->getTypeNode(i) ) )
                arg = tuple->getAddr(i, ctxt_);
            else
            {
                const llvm::Type* llvmType = type->getLLVMType( ctxt_->module_);
                llvm::AllocaInst* alloca = ctxt_->createEntryAlloca(llvmType);
                arg = builder.CreateLoad(alloca);

                if (i == 0)
                    perRefMainRetAlloca = alloca;
            }

            args.push_back(arg);
        }
    }

    // append regular arguments
    for (size_t i = 0; i < call->exprList_->size(); ++i)
    {
        const Type* type = call->exprList_->typeList()[i];

        llvm::Value* arg = type->perRef() 
            ? call->exprList_->getAddr(i, ctxt_)
            : call->exprList_->getScalar(i, builder);

        args.push_back(arg);
    }

    // create actual call
    llvm::CallInst* callInst = llvm::CallInst::Create( 
            fct->llvmFct_, args.begin(), args.end() );
    callInst->setCallingConv(llvm::CallingConv::Fast);
    llvm::Value* retValue = builder.Insert(callInst);

    // write results back
    if (tuple)
    {
        size_t index = 0;
        for (size_t i = 0; i < out.size(); ++i)
        {
            const Type* type = out[i];

            if ( type->perRef() )
                continue;

            swiftAssert( tuple->isAddr(i), "must be an address" );
            llvm::Value* retElem = builder.CreateExtractValue(retValue, index);
            builder.CreateStore( retElem, tuple->getAddr(i, ctxt_) );

            ++index;
        }
    }
    else if ( !fct->sig_.out_.empty() )
    {
        if ( perRefMainRetAlloca )
        {
            llvmValue_ = perRefMainRetAlloca;
            isAddr_ = true;
        }
        else
        {
            llvmValue_ = builder.CreateExtractValue(retValue, 0);
            isAddr_ = false;
        }
    }
    else
        llvmValue_ = 0;
}

llvm::Value* TypeNodeCodeGen::getLLVMValue() const
{
    return llvmValue_;
}

llvm::Value* TypeNodeCodeGen::getAddr() const
{
    if (!isAddr_)
    {
        llvm::AllocaInst* alloca = ctxt_->createEntryAlloca( 
                llvmValue_->getType(), llvmValue_->getName() );
        ctxt_->builder_.CreateStore(llvmValue_, alloca);

        return alloca;
    }
    else
        return llvmValue_;
}

llvm::Value* TypeNodeCodeGen::getScalar() const
{
    if (isAddr_)
        return ctxt_->builder_.CreateLoad( llvmValue_, llvmValue_->getName() );
    else
        return llvmValue_;
}

bool TypeNodeCodeGen::isAddr() const
{
    return isAddr_;
}

} // namespace swift
