#include "fe/typenodecodegen.h"

#include <sstream>

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/TypeBuilder.h>

#include "utils/cast.h"
#include "utils/llvmhelper.h"

#include "fe/context.h"
#include "fe/class.h"
#include "fe/scope.h"
#include "fe/tnlist.h"
#include "fe/type.h"
#include "fe/var.h"

using llvm::Value;

namespace swift {

TypeNodeCodeGen::TypeNodeVisitor(Context* ctxt)
    : TypeNodeVisitorBase(ctxt)
    , builder_(ctxt->builder_)
    , lctxt_( ctxt->lctxt() )
{}

void TypeNodeCodeGen::visit(Decl* d)
{
    // has it already been inited via a return value?
    if (d->alloca_)
    {
        d->local_->setAlloca(d->alloca_);
        d->alloca_->setName( d->cid() );
    }
    else
        d->local_->createEntryAlloca(ctxt_);

    setResult( new Addr(d->local_->getAddr(builder_)) );
}

void TypeNodeCodeGen::visit(ErrorExpr* d)
{
    swiftAssert(false, "unreachable");
}

void TypeNodeCodeGen::visit(Id* id)
{
    Var* var = ctxt_->scope()->lookupVar( id->id() );
    swiftAssert(var, "must be found");

    setResult( new Addr(var->getAddr(builder_)) );
}

void TypeNodeCodeGen::visit(Literal* l)
{
    Value* val;

    switch ( l->getToken() )
    {
        /*
         * ints
         */

        case Token::L_SAT8:
        case Token::L_INT8:
        case Token::L_USAT8:
        case Token::L_UINT8:  val = createInt8 (lctxt_, l->box_.uint64_); break;

        case Token::L_SAT16:
        case Token::L_INT16: 
        case Token::L_USAT16:
        case Token::L_UINT16: val = createInt16(lctxt_, l->box_.uint64_); break;

        case Token::L_INT:  
        case Token::L_UINT:  
        case Token::L_INT32: 
        case Token::L_UINT32: val = createInt32(lctxt_, l->box_.uint64_); break;

        case Token::L_INDEX:
        case Token::L_INT64: 
        case Token::L_UINT64: val = createInt64(lctxt_, l->box_.uint64_); break;

        case Token::L_TRUE:
        case Token::L_FALSE:
            //val = new BaseType(l->loc(), Token::CONST, new std::string("bool")  ); return;

        /*
         * floats
         */

        case Token::L_REAL:
        case Token::L_REAL32: val = createFloat (lctxt_, l->box_.float_);  break;
        case Token::L_REAL64: val = createDouble(lctxt_, l->box_.double_); break;

        default: val = 0;
            swiftAssert(false, "illegal switch-case-value");
    }

    setResult( new Scalar(val) );
}

void TypeNodeCodeGen::visit(Nil* n)
{
    // TODO
}

void TypeNodeCodeGen::visit(Self* n)
{
    Method* m = cast<Method>(ctxt_->memberFct_);
    setResult( new Addr(builder_.CreateLoad( m->getSelfValue(), "self" )) );
}

// TODO writeback?
Value* TypeNodeCodeGen::resolvePrefixExpr(Access* a)
{
    // get address of the prefix expr
    a->prefixExpr_->accept(this);
    Value* addr = getPlace()->getAddr(builder_);

    if ( const Ptr* ptr = a->prefixExpr_->getType()->cast<Ptr>() )
        addr = ptr->recDerefAddr(builder_, addr);

    return addr;
}

void TypeNodeCodeGen::visit(IndexExpr* i)
{
    Value* addr = resolvePrefixExpr(i);

    i->indexExpr_->accept(this);
    Value* idx = getPlace()->getScalar(builder_);

    Value* ptr = createLoadInBoundsGEP_0_i32(lctxt_, builder_, addr, Container::POINTER);

    const Type* prefixType = i->prefixExpr_->getType();
    if ( dynamic_cast<const Array*>(prefixType) )
        setResult( new Addr(builder_.CreateInBoundsGEP(ptr, idx)) );
    else
    {
        const Simd* simd = cast<Simd>(prefixType);
        const Type* inner = simd->getInnerType();

        int simdLength;
        /* const llvm::Type* vecType = */
        inner->getVecLLVMType(ctxt_->module_, simdLength);

        Value* slv = createInt64(lctxt_, simdLength);
        Value* div = builder_.CreateUDiv(idx, slv);
        Value* rem = builder_.CreateURem(idx, slv);
        Value* mod = builder_.CreateTrunc( rem , llvm::IntegerType::getInt32Ty(lctxt_) );
        Value* aElem = createLoadInBoundsGEP_x(lctxt_, builder_, ptr, div);

        if ( const llvm::StructType* vecStruct = 
                dynamic_cast<const llvm::StructType*>(aElem->getType()) )
        {
            const UserType* ut = inner->cast<UserType>();
            const llvm::Type* scalarStruct = ut->lookupClass(ctxt_->module_)->getLLVMType();

            typedef llvm::StructType::element_iterator EIter;
            int memIdx = 0;
            Value* res = llvm::UndefValue::get(scalarStruct);
            for (EIter iter = vecStruct->element_begin(); iter != vecStruct->element_end(); ++iter)
            {
                Value* vElem = builder_.CreateExtractValue(aElem, memIdx);
                Value*  elem = builder_.CreateExtractElement(vElem, mod);
                res = builder_.CreateInsertValue(res, elem, memIdx);

                ++memIdx;
            }

            setResult( new Scalar(res) );
        }
        else
            setResult( new Scalar(builder_.CreateExtractElement(aElem, mod)) );
    }
}

void TypeNodeCodeGen::visit(MemberAccess* m)
{
    Value* addr = resolvePrefixExpr(m);

    // build and get value
    std::ostringstream oss;
    oss << m->prefixExpr_->getType()->toString() << '.' << m->cid();
    int index = m->memberVar_->getIndex();
    Value* result = createInBoundsGEP_0_i32( lctxt_, builder_, addr, index, oss.str() );

    setResult( new Addr(result) );
}

void TypeNodeCodeGen::visit(CCall* c)
{
    c->exprList_->accept(this);
    const TypeList& inTypes = c->exprList_->typeList();

    /*
     * build function type
     */

    const llvm::Type* retType = c->retType_ 
        ? c->retType_->getLLVMType(ctxt_->module_) 
        : createVoid(lctxt_);

    LLVMTypes params( inTypes.size() );

    for (size_t i = 0; i < inTypes.size(); ++i)
        params[i] = inTypes[i]->getLLVMType(ctxt_->module_);

    const llvm::FunctionType* fctType = llvm::FunctionType::get(
            retType, params, false);

    // declare function
    llvm::Function* fct = cast<llvm::Function>(
        ctxt_->module_->getLLVMModule()->getOrInsertFunction( c->cid(), fctType) );

    // copy over values
    Values args( c->exprList_->numRetValues() );
    for (size_t i = 0; i < args.size(); ++i)
        args[i] = c->exprList_->getPlace(i)->getScalar(builder_);

    fct->setCallingConv(llvm::CallingConv::C);
    fct->setLinkage(llvm::Function::ExternalLinkage);
    //fct->addFnAttr(llvm::Attribute::NoUnwind);
    swiftAssert( fct->isDeclaration(), "may only be a declaration" );

    llvm::CallInst* call = llvm::CallInst::Create( fct, args.begin(), args.end() );
    call->setTailCall();
    call->addAttribute(~0, llvm::Attribute::NoUnwind);

    setResult( new Scalar(builder_.Insert(call)) );
}

void TypeNodeCodeGen::visit(ReaderCall* r)
{
    getSelf(r);

    if ( const ScalarType* from = r->expr_->getType()->cast<ScalarType>() )
    {
        // -> assumes that r is a cast

        const ScalarType* to = cast<ScalarType>( r->memberFct_->sig_.out_[0]->getType() );
        const llvm::Type* llvmTo = to->getLLVMType(ctxt_->module_);
        const llvm::Type* llvmFrom = from->getLLVMType(ctxt_->module_);

        if ( llvmTo == llvmFrom )
            return; // value_ and isAddr_ are still correct, so nothing to do

        Value* val = getPlace()->getScalar(builder_);
        llvm::StringRef name = val->getName();

        if ( from->isInteger() && to->isInteger() )
        {
            if ( from->sizeOf() > to->sizeOf() )
                val = builder_.CreateTrunc(val, llvmTo, name);
            else
            {
                // -> sizeof(from) < sizeof(to)
                if ( from->isUnsigned() )
                    val = builder_.CreateZExt(val, llvmTo, name);
                else
                    val = builder_.CreateSExt(val, llvmTo, name);
            }
        }
        else if ( from->isFloat() && to->isSigned() )   // fp -> si
            val = builder_.CreateFPToSI(val, llvmTo, name);
        else if ( from->isFloat() && to->isUnsigned() ) // fp -> ui
            val = builder_.CreateFPToUI(val, llvmTo, name);
        else if ( from->isSigned() && to->isFloat() )   // si -> fp
            val = builder_.CreateSIToFP(val, llvmTo, name);
        else if ( from->isUnsigned() && to->isFloat() ) // ui -> fp
            val = builder_.CreateUIToFP(val, llvmTo, name);
        else
        {
            swiftAssert( from->isFloat() && to->isFloat(), "must both be floats" );

            if ( from->sizeOf() > to->sizeOf() )
                val = builder_.CreateFPTrunc(val, llvmTo, name);
            else // -> sizeof(from) < sizeof(to)
                val = builder_.CreateFPExt(val, llvmTo, name);
        }

        setResult( new Scalar(val) );
    }
    else
        emitCall( r, getPlace() );
}

void TypeNodeCodeGen::visit(WriterCall* w)
{
    getSelf(w);
    emitCall( w, getPlace() );
}

void TypeNodeCodeGen::visit(BinExpr* b)
{
    if ( b->builtin_ )
    {
        b->op1_->accept(this);
        Value* v1 = getPlace()->getScalar(builder_);
        b->op2_->accept(this);
        Value* v2 = getPlace()->getScalar(builder_);

        const ScalarType* scalar = cast<ScalarType>( b->op1_->getType() );

        Value* val;

        switch (b->token_)
        {
            /*
             * arithmetic operators
             */

            case Token::ADD: val = builder_.CreateAdd(v1, v2); break;
            case Token::SUB: val = builder_.CreateSub(v1, v2); break;
            case Token::MUL: val = builder_.CreateMul(v1, v2); break;
            case Token::DIV:
                if ( scalar->isFloat() )
                    val = builder_.CreateFDiv(v1, v2);
                else
                {
                    if ( scalar->isSigned() )
                        val = builder_.CreateSDiv(v1, v2);
                    else // -> unsigned
                        val = builder_.CreateUDiv(v1, v2);
                }
                break;

            /*
             * comparisons
             */

            case Token::EQ:
                if ( scalar->isFloat() )
                    val = builder_.CreateFCmpOEQ(v1, v2);
                else
                    val = builder_.CreateICmpEQ(v1, v2);
                break;
            case Token::NE:
                if ( scalar->isFloat() )
                    val = builder_.CreateFCmpONE(v1, v2);
                else
                    val = builder_.CreateICmpNE(v1, v2);
                break;

#define SWIFT_EMIT_CMP(token, fcmp, scmp, ucmp) \
    case Token:: token : \
        if ( scalar->isFloat() ) \
            val = builder_. fcmp (v1, v2); \
        else \
        { \
            if ( scalar->isSigned() ) \
                val = builder_. scmp (v1, v2); \
            else \
                val = builder_. ucmp (v1, v2); \
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

        setResult( new Scalar(val) );
    }
    else
        emitCall(b, 0);
}

void TypeNodeCodeGen::visit(UnExpr* u)
{
    if ( u->builtin_ )
    {
        u->op1_->accept(this);
        Value* val = getPlace()->getScalar(builder_);
        const ScalarType* scalar = cast<ScalarType>( u->op1_->getType() );

        switch (u->token_)
        {
            case Token::ADD: 
                // nothing to do
                break;
            case Token::SUB:
                val = builder_.CreateSub( llvm::Constant::getNullValue( 
                            scalar->getLLVMType(ctxt_->module_)), val );
                break;
            default:
                swiftAssert(false, "TODO");
        }

        setResult( new Scalar(val) );
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
    Value* addr = getPlace()->getAddr(builder_);

    swiftAssert(places_.size() == 1, "must exactly have one item");

    if ( const Ptr* ptr = m->expr_->getType()->cast<Ptr>() )
        addr = ptr->recDerefAddr(ctxt_->builder_, addr);

    places_[0] = new Addr(addr);
}

void TypeNodeCodeGen::emitCall(MemberFctCall* call, Place* self)
{
    Values args;
    MemberFct* fct = call->getMemberFct();

    call->exprList_->accept(this);
    TypeList& out = fct->sig_.outTypes_;

    /* 
     * prepare arguments
     */

    if (self)
        args.push_back( self->getAddr(builder_) );

    Values perRefRetValues;

    // append return-value arguments
    for (size_t i = 0; i < out.size(); ++i)
    {
        const Type* type = out[i];

        if ( type->perRef() )
        {
            const llvm::Type* llvmType = type->getRawLLVMType( ctxt_->module_);
            llvm::AllocaInst* alloca = createEntryAlloca(builder_, llvmType);

            args.push_back(alloca);
            perRefRetValues.push_back(alloca);
        }
    }

    // append regular arguments
    call->exprList_->getArgs(args, builder_);

    // create actual call
    llvm::CallInst* callInst = llvm::CallInst::Create( 
            fct->llvmFct_, args.begin(), args.end() );
    callInst->setCallingConv(llvm::CallingConv::Fast);
    Value* retValue = builder_.Insert(callInst);

    /*
     * write results back
     */

    places_.clear();

    size_t idxRetType = 0;
    size_t idxPerRef = 0;
    for (size_t i = 0; i < out.size(); ++i)
    {
        const Type* type = out[i];

        if ( type->perRef() )
            places_.push_back( new Addr(perRefRetValues[idxPerRef]) );
        else
            places_.push_back( new Scalar(builder_.CreateExtractValue(retValue, idxRetType)) );

        ++idxPerRef;
    }
}

Place* TypeNodeCodeGen::getPlace(size_t i /*= 0*/) const
{
    return places_[i];
}

void TypeNodeCodeGen::setResult(Place* place)
{
    places_.resize(1);
    places_[0] = place;
}

} // namespace swift
