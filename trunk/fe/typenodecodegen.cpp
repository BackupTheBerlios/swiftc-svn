#include "fe/typenodecodegen.h"

#include <sstream>

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/Support/TypeBuilder.h>

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
    if (d->alloca_)
    {
        d->local_->setAlloca(d->alloca_);
        setResult( d->alloca_, true );
    }
    else
    {
        d->local_->createEntryAlloca(ctxt_);
        setResult( d->local_->getAddr(ctxt_), true );
    }
}

void TypeNodeCodeGen::visit(ErrorExpr* d)
{
    swiftAssert(false, "unreachable");
}

void TypeNodeCodeGen::visit(Id* id)
{
    Var* var = ctxt_->scope()->lookupVar( id->id() );
    swiftAssert(var, "must be found");

    setResult( var->getAddr(ctxt_), true );
}

void TypeNodeCodeGen::visit(Literal* l)
{
    using llvm::ConstantInt;
    using llvm::ConstantFP;
    using llvm::APInt;
    using llvm::APFloat;

    llvm::LLVMContext& llvmCtxt = *ctxt_->module_->llvmCtxt_;
    llvm::Value* val;

    switch ( l->getToken() )
    {
        /*
         * signed ints
         */

        case Token::L_SAT8:
        case Token::L_INT8: 
            val = ConstantInt::get(llvmCtxt, APInt( 8, l->box_.int64_, true)); break;

        case Token::L_SAT16:
        case Token::L_INT16:
            val = ConstantInt::get(llvmCtxt, APInt(16, l->box_.int64_, true)); break;

        case Token::L_INT:  
        case Token::L_INT32:
            val = ConstantInt::get(llvmCtxt, APInt(32, l->box_.int64_, true)); break;   

        case Token::L_INT64:
            val = ConstantInt::get(llvmCtxt, APInt(64, l->box_.int64_, true)); break;   

        /*
         * unsigned ints
         */

        case Token::L_USAT8:
        case Token::L_UINT8: 
            val = ConstantInt::get(llvmCtxt, APInt( 8, l->box_.uint64_)); break;   

        case Token::L_USAT16:
        case Token::L_UINT16:
            val = ConstantInt::get(llvmCtxt, APInt(16, l->box_.uint64_)); break;   

        case Token::L_UINT:  
        case Token::L_UINT32:
            val = ConstantInt::get(llvmCtxt, APInt(32, l->box_.uint64_)); break;   

        case Token::L_INDEX:
        case Token::L_UINT64:
            val = ConstantInt::get(llvmCtxt, APInt(64, l->box_.uint64_)); break;   

        /*
         * floats
         */

        case Token::L_REAL:
        case Token::L_REAL32: 
            val = ConstantFP::get(llvmCtxt, APFloat(l->box_.float_) ); break;
        case Token::L_REAL64: 
            val = ConstantFP::get(llvmCtxt, APFloat(l->box_.double_) ); break;

        case Token::L_TRUE:
        case Token::L_FALSE:
            //val = new BaseType(l->loc(), Token::CONST, new std::string("bool")  ); return;

        default:
            val = 0;
            swiftAssert(false, "illegal switch-case-value");
    }

    setResult(val, false);
}

void TypeNodeCodeGen::visit(Nil* n)
{
    // TODO
}

void TypeNodeCodeGen::visit(Self* n)
{
    Method* m = llvm::cast<Method>(ctxt_->memberFct_);
    setResult( ctxt_->builder_.CreateLoad( m->getSelfValue(), "self" ), true );
}

void TypeNodeCodeGen::visit(IndexExpr* i)
{
    // TODO
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
    setResult( builder.CreateInBoundsGEP( addr, input, input+2, oss.str() ), true );
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

    const llvm::Type* retType;
    if (c->retType_)
        retType = c->retType_->getLLVMType(ctxt_->module_);
    else
        retType = llvm::TypeBuilder<void, true>::get(*ctxt_->module_->llvmCtxt_);

    std::vector<const llvm::Type*> params( inTypes.size() );

    for (size_t i = 0; i < inTypes.size(); ++i)
        params[i] = inTypes[i]->getLLVMType(ctxt_->module_);

    const llvm::FunctionType* fctType = llvm::FunctionType::get(
            retType, params, false);

    // declare function
    llvm::Function* fct = llvm::cast<llvm::Function>(
        llvmModule->getOrInsertFunction( c->cid(), fctType) );

    // copy over values
    std::vector<llvm::Value*> args( c->exprList_->numRetValues() );
    for (size_t i = 0; i < args.size(); ++i)
        args[i] = c->exprList_->getScalar(i, builder);

    fct->setCallingConv(llvm::CallingConv::C);
    fct->setLinkage(llvm::Function::ExternalLinkage);
    //fct->addFnAttr(llvm::Attribute::NoUnwind);
    swiftAssert( fct->isDeclaration(), "may only be a declaration" );

    llvm::CallInst* call = llvm::CallInst::Create( fct, args.begin(), args.end() );
    call->setTailCall();
    call->addAttribute(~0, llvm::Attribute::NoUnwind);

    setResult( builder.Insert(call), false );
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
            return; // value_ and isAddr_ are still correct, so nothing to do

        llvm::Value* val = getValue();

        if ( isAddr() )
            val = builder.CreateLoad(val, val->getName() );

        llvm::StringRef name = val->getName();

        if ( from->isInteger() && to->isInteger() )
        {
            if ( from->sizeOf() > to->sizeOf() )
                val = builder.CreateTrunc(val, llvmTo, name);
            else
            {
                // -> sizeof(from) < sizeof(to)
                if ( from->isUnsigned() )
                    val = builder.CreateZExt(val, llvmTo, name);
                else
                    val = builder.CreateSExt(val, llvmTo, name);
            }
        }
        else if ( from->isFloat() && to->isSigned() )   // fp -> si
            val = builder.CreateFPToSI(val, llvmTo, name);
        else if ( from->isFloat() && to->isUnsigned() ) // fp -> ui
            val = builder.CreateFPToUI(val, llvmTo, name);
        else if ( from->isSigned() && to->isFloat() )   // si -> fp
            val = builder.CreateSIToFP(val, llvmTo, name);
        else if ( from->isUnsigned() && to->isFloat() ) // ui -> fp
            val = builder.CreateUIToFP(val, llvmTo, name);
        else
        {
            swiftAssert( from->isFloat() && to->isFloat(), "must both be floats" );

            if ( from->sizeOf() > to->sizeOf() )
                val = builder.CreateFPTrunc(val, llvmTo, name);
            else // -> sizeof(from) < sizeof(to)
                val = builder.CreateFPExt(val, llvmTo, name);
        }

        setResult(val, false);
    }
    else
        emitCall( r, getValue() );
}

void TypeNodeCodeGen::visit(WriterCall* w)
{
    getSelf(w);
    emitCall( w, getValue() );
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

        llvm::Value* val;

        switch (b->token_)
        {
            /*
             * arithmetic operators
             */

            case Token::ADD: val = builder.CreateAdd(v1, v2); break;
            case Token::SUB: val = builder.CreateSub(v1, v2); break;
            case Token::MUL: val = builder.CreateMul(v1, v2); break;
            case Token::DIV:
                if ( scalar->isFloat() )
                    val = builder.CreateFDiv(v1, v2);
                else
                {
                    if ( scalar->isSigned() )
                        val = builder.CreateSDiv(v1, v2);
                    else // -> unsigned
                        val = builder.CreateUDiv(v1, v2);
                }
                break;

            /*
             * comparisons
             */

            case Token::EQ:
                if ( scalar->isFloat() )
                    val = builder.CreateFCmpOEQ(v1, v2);
                else
                    val = builder.CreateICmpEQ(v1, v2);
                break;
            case Token::NE:
                if ( scalar->isFloat() )
                    val = builder.CreateFCmpONE(v1, v2);
                else
                    val = builder.CreateICmpNE(v1, v2);
                break;

#define SWIFT_EMIT_CMP(token, fcmp, scmp, ucmp) \
    case Token:: token : \
        if ( scalar->isFloat() ) \
            val = builder. fcmp (v1, v2); \
        else \
        { \
            if ( scalar->isSigned() ) \
                val = builder. scmp (v1, v2); \
            else \
                val = builder. ucmp (v1, v2); \
        } \
        break;

            SWIFT_EMIT_CMP(LT, CreateFCmpOLT, CreateICmpSLT, CreateICmpULT);
            SWIFT_EMIT_CMP(LE, CreateFCmpOLE, CreateICmpSLE, CreateICmpULE);
            SWIFT_EMIT_CMP(GT, CreateFCmpOGT, CreateICmpSGT, CreateICmpUGT);
            SWIFT_EMIT_CMP(GE, CreateFCmpOGE, CreateICmpSGE, CreateICmpUGE);

#undef SWIFT_EMIT_CMP

            default:
                val = 0;
                swiftAssert(false, "TODO");
        }

        setResult(val, false);
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
        llvm::Value* val = getScalar();
        const ScalarType* scalar = llvm::cast<ScalarType>( u->op1_->getType() );

        switch (u->token_)
        {
            case Token::ADD: 
                // nothing to do
                break;
            case Token::SUB:
                val = builder.CreateSub( llvm::Constant::getNullValue( 
                            scalar->getLLVMType(ctxt_->module_)), val );
                break;
            default:
                swiftAssert(false, "TODO");
        }

        setResult(val, false);
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

    swiftAssert(values_.size() == 1, "must exactly have one item");

    if ( const Ptr* ptr = m->expr_->getType()->cast<Ptr>() )
        values_[0] = ptr->recDerefAddr(ctxt_->builder_, values_[0]);
}

void TypeNodeCodeGen::emitCall(MemberFctCall* call, llvm::Value* self)
{
    llvm::IRBuilder<>& builder = ctxt_->builder_;
    std::vector<llvm::Value*> args;
    MemberFct* fct = call->getMemberFct();

    call->exprList_->accept(this);
    TypeList& out = fct->sig_.outTypes_;

    /* 
     * prepare arguments
     */

    if (self)
        args.push_back(self);

    std::vector<llvm::Value*> perRefRetValues;

    // append return-value arguments
    for (size_t i = 0; i < out.size(); ++i)
    {
        const Type* type = out[i];

        if ( type->perRef() )
        {
            const llvm::Type* llvmType = type->getRawLLVMType( ctxt_->module_);
            llvm::AllocaInst* alloca = ctxt_->createEntryAlloca(llvmType);

            args.push_back(alloca);
            perRefRetValues.push_back(alloca);
        }
    }

    // append regular arguments
    for (size_t i = 0; i < call->exprList_->numRetValues(); ++i)
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

    /*
     * write results back
     */

    values_.clear();
    addresses_.clear();

    size_t idxRetType = 0;
    size_t idxPerRef = 0;
    for (size_t i = 0; i < out.size(); ++i)
    {
        const Type* type = out[i];
        llvm::Value* val;

        if ( type->perRef() )
        {
            val = perRefRetValues[idxPerRef];
            addresses_.push_back(true);
            ++idxPerRef;
        }
        else
        {
            val = builder.CreateExtractValue(retValue, idxRetType);
            addresses_.push_back(false);
            ++idxRetType;
        }

        values_.push_back(val);
    }
}

llvm::Value* TypeNodeCodeGen::getValue(size_t i /*= 0*/) const
{
    return values_[i];
}

llvm::Value* TypeNodeCodeGen::getAddr(size_t i /*= 0*/) const
{
    llvm::Value* val = values_[i];

    if ( !addresses_[i] )
    {
        llvm::AllocaInst* alloca = 
            ctxt_->createEntryAlloca( val->getType(), val->getName() );
        ctxt_->builder_.CreateStore(val, alloca);

        return alloca;
    }
    else
        return val;
}

llvm::Value* TypeNodeCodeGen::getScalar(size_t i /*= 0*/) const
{
    llvm::Value* val = values_[i];

    if ( addresses_[i] )
        return ctxt_->builder_.CreateLoad( val, val->getName() );
    else
        return val;
}

bool TypeNodeCodeGen::isAddr(size_t i /*= 0*/) const
{
    return addresses_[i];
}

void TypeNodeCodeGen::setResult(llvm::Value* value, bool isAddr)
{
    swiftAssert( values_.size() == addresses_.size(), "sizes must match" );

    values_.resize(1);
    addresses_.resize(1);
    values_[0] = value;
    addresses_[0] = isAddr;
}

} // namespace swift
