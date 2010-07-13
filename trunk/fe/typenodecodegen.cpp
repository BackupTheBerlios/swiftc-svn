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
    // has it already been initialized via a return value?
    if (d->alloca_)
    {
        d->local_->setAlloca(d->alloca_);
        d->alloca_->setName( d->cid() );
    }
    else
        d->local_->createEntryAlloca(ctxt_);

    setResult( d, new Addr(d->local_->getAddr(builder_)) );
}

void TypeNodeCodeGen::visit(ErrorExpr* d)
{
    swiftAssert(false, "unreachable");
}

void TypeNodeCodeGen::visit(Broadcast* b)
{
    b->expr_->accept(this);

    int simdLength;
    const llvm::Type* vType = b->get().type_->getVecLLVMType(ctxt_->module_, simdLength);

    Value* sVal = b->expr_->get().place_->getScalar(builder_);
    Value* vVal = simdBroadcast(sVal, vType, builder_);

    setResult( b, new Scalar(vVal) );
}

void TypeNodeCodeGen::visit(Id* id)
{
    Var* var = ctxt_->scope()->lookupVar( id->id() );
    swiftAssert(var, "must be found");

    setResult( id, new Addr(var->getAddr(builder_)) );
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

    setResult( l, new Scalar(val) );
}

void TypeNodeCodeGen::visit(Nil* n)
{
    // TODO
}

void TypeNodeCodeGen::visit(Range* r)
{
    //const Type* type = r->expr_->get().type_;
    // TODO

    //int simdLength;
    //const llvm::Type* vType = b->get().type_->getVecLLVMType(ctxt_->module_, simdLength);

    //Value* sVal = tncg.getPlace()->getScalar(builder_);
    //Value* vVal = simdBroadcast(sVal, vType, builder_);

    //setResult( new Scalar(vVal) );
}

void TypeNodeCodeGen::visit(Self* n)
{
    Method* m = cast<Method>(ctxt_->memberFct_);
    setResult( n, new Addr(builder_.CreateLoad( m->getSelfValue(), "self" )) );
}

// TODO writeback?
Value* TypeNodeCodeGen::resolvePrefixExpr(Access* a)
{
    // get address of the prefix expr
    a->prefixExpr_->accept(this);
    Value* addr = a->prefixExpr_->get().place_->getAddr(builder_);

    if ( const Ptr* ptr = a->prefixExpr_->get().type_->cast<Ptr>() )
        addr = ptr->recDerefAddr(builder_, addr);

    return addr;
}

void TypeNodeCodeGen::visit(IndexExpr* i)
{
    Value* addr = resolvePrefixExpr(i);

    i->indexExpr_->accept(this);
    Value* idx = i->indexExpr_->get().place_->getScalar(builder_);

    Value* ptr = createLoadInBoundsGEP_0_i32( lctxt_, builder_, addr, 
            Container::POINTER, addr->getNameStr() + ".ptr" );

    const Type* prefixType = i->prefixExpr_->get().type_;
    if ( dynamic<Array>(prefixType) )
        setResult( i, new Addr(builder_.CreateInBoundsGEP(ptr, idx)) );
    else
    {
        const Simd* simd = cast<Simd>(prefixType);
        const Type* inner = simd->getInnerType();

        int simdLength;
        /* const llvm::Type* vecType = */
        inner->getVecLLVMType(ctxt_->module_, simdLength);

        Value* slv = createInt64(lctxt_, simdLength);
        llvm::Value* div = builder_.CreateUDiv(idx, slv);
        Value* rem = builder_.CreateURem(idx, slv);
        Value* mod = builder_.CreateTrunc( rem , llvm::IntegerType::getInt32Ty(lctxt_) );
        Value* aggPtr = builder_.CreateInBoundsGEP(ptr, div, addr->getNameStr() + ".idx");

        setResult( i, new SimdAddr(aggPtr, mod, inner->getLLVMType(ctxt_->module_), builder_) );
    }
}

void TypeNodeCodeGen::visit(SimdIndexExpr* s)
{
    swiftAssert(ctxt_->simdIndex_, "can only be valid within simd loops");

    Value* addr = resolvePrefixExpr(s);
    Value* ptr = createLoadInBoundsGEP_0_i32( lctxt_, builder_, addr, 
            Container::POINTER, addr->getNameStr() + ".ptr" );
    const Type* prefixType = s->prefixExpr_->get().type_;

    if ( dynamic<Simd>(prefixType) )
    {
        Value* index = builder_.CreateUDiv( 
                builder_.CreateLoad(ctxt_->simdIndex_),
                createInt64(lctxt_, 4) ); // HACK
        setResult( s, new Addr(builder_.CreateInBoundsGEP(ptr, index)) );
        //Value* val = createInt64(lctxt_, 7);
        //setResult( s, new Addr( abuilder_.CreateInBoundsGEP(ptr, val)) );
        return;
    }
    else
    {
        swiftAssert( dynamic<Array>(prefixType), "must be an array" );
        //setResult( i, new ScalarAddr(builder_.CreateInBoundsGEP(ptr, ctxt_->simdIndex_)) );
        swiftAssert(false, "TODO");
        return;
    }
}

void TypeNodeCodeGen::visit(MemberAccess* m)
{
    Value* addr = resolvePrefixExpr(m);

    // build and get value
    std::ostringstream oss;
    oss << m->prefixExpr_->get().type_->toString() << '.' << m->cid();
    int index = m->memberVar_->getIndex();
    Value* result = createInBoundsGEP_0_i32( lctxt_, builder_, addr, index, oss.str() );

    setResult( m, new Addr(result) );
}

void TypeNodeCodeGen::visit(CCall* c)
{
    TNList& args = *c->exprList_;
    args.accept(this);
    const TypeList& inTypes = args.typeList();

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
    Values values( args.numResults() );
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = args.getResult(i).place_->getScalar(builder_);

    fct->setCallingConv(llvm::CallingConv::C);
    fct->setLinkage(llvm::Function::ExternalLinkage);
    //fct->addFnAttr(llvm::Attribute::NoUnwind);
    swiftAssert( fct->isDeclaration(), "may only be a declaration" );

    llvm::CallInst* call = llvm::CallInst::Create( fct, values.begin(), values.end() );
    call->setTailCall();
    call->addAttribute(~0, llvm::Attribute::NoUnwind);
    Value* retVal = builder_.Insert(call);

    if (c->retType_)
        setResult( c, new Scalar(retVal) );
}

void TypeNodeCodeGen::visit(MethodCall* m)
{
    Place* self = getSelf(m);

    if ( const ScalarType* from = m->expr_->get().type_->cast<ScalarType>() )
    {
        // -> assumes that r is a cast

        const ScalarType* to = cast<ScalarType>( m->memberFct_->sig_.out_[0]->getType() );
        const llvm::Type* llvmTo = to->getLLVMType(ctxt_->module_);
        const llvm::Type* llvmFrom = from->getLLVMType(ctxt_->module_);

        if ( llvmTo == llvmFrom )
            return; // value_ and isAddr_ are still correct, so nothing to do

        Value* val = self->getScalar(builder_);
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

        setResult( m, new Scalar(val) );
    }
    else
        emitCall(m, self);
}

void TypeNodeCodeGen::visit(CreateCall* r)
{
    //emitCall(r, 0);
    // TODO
}

void TypeNodeCodeGen::visit(RoutineCall* r)
{
    emitCall(r, 0);
}

void TypeNodeCodeGen::visit(UnExpr* u)
{
    u->op1_->accept(this);

    if ( u->builtin_ )
    {
        Value* val = u->op1_->get().place_->getScalar(builder_);

        switch ( (*u->id_)[0] )
        {
            case '+': break; // nothing to do
            case '-': val = builder_.CreateNeg(val); break;
            default:         swiftAssert(false, "TODO");
        }

        setResult( u, new Scalar(val) );
    }
    else
        emitCall(u, u->op1_->set().place_);
}

static inline int tok2val(const char* str)
{
    return str[0] + ((str + 1 != 0) ? str[1] * 0x100 : 0);
}

#define TOK2VAL(c1, c2) (c1) + ((c2) * 0x100)

void TypeNodeCodeGen::visit(BinExpr* b)
{
    // accept self value in all cases
    b->op1_->accept(this);

    if ( b->builtin_ )
    {
        b->op2_->accept(this);

        Value* v1 = b->op1_->get().place_->getScalar(builder_);
        Value* v2 = b->op2_->get().place_->getScalar(builder_);

        const ScalarType* scalar = cast<ScalarType>( b->op1_->get().type_ );

        Value* val;

        // calc unique value
        std::string& id = *b->id_;
        int token = id[0] + ((id.size() > 1) ? id[1] * 0x100 : 0 );

        switch (token)
        {
            /*
             * arithmetic operators
             */

            case '+': val = builder_.CreateAdd(v1, v2); break;
            case '-': val = builder_.CreateSub(v1, v2); break;
            case '*': val = builder_.CreateMul(v1, v2); break;
            case '/':
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

            case TOK2VAL('=', '='):
                if ( scalar->isFloat() )
                    val = builder_.CreateFCmpOEQ(v1, v2);
                else
                    val = builder_.CreateICmpEQ(v1, v2);
                break;
            case TOK2VAL('!', '='):
                if ( scalar->isFloat() )
                    val = builder_.CreateFCmpONE(v1, v2);
                else
                    val = builder_.CreateICmpNE(v1, v2);
                break;

#define SWIFT_EMIT_CMP(token, fcmp, scmp, ucmp) \
    case token : \
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

            SWIFT_EMIT_CMP('<', CreateFCmpOLT, CreateICmpSLT, CreateICmpULT);
            SWIFT_EMIT_CMP('>', CreateFCmpOGT, CreateICmpSGT, CreateICmpUGT);
            SWIFT_EMIT_CMP(TOK2VAL('<', '='), CreateFCmpOLE, CreateICmpSLE, CreateICmpULE);
            SWIFT_EMIT_CMP(TOK2VAL('>', '='), CreateFCmpOGE, CreateICmpSGE, CreateICmpUGE);

#undef SWIFT_EMIT_CMP

            default:
                val = 0;
                swiftAssert(false, "TODO");
        }

        setResult( b, new Scalar(val) );
    }
    else
        emitCall(b, b->op1_->set().place_);
}

Place* TypeNodeCodeGen::getSelf(MethodCall* m)
{
    swiftAssert(m->expr_->numResults() == 1, "must exactly have one result" );

    m->expr_->accept(this);
    Place*& self = m->expr_->set().place_;

    Value* addr = self->getAddr(builder_);

    if ( const Ptr* ptr = m->expr_->get().type_->cast<Ptr>() )
        addr = ptr->recDerefAddr(ctxt_->builder_, addr);

    // replace result;
    delete self;
    self = new Addr(addr);

    return self;
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
            int simdLength = call->simd_ ? 4 : 0; // HACK
            const llvm::Type* llvmType = type->getRawLLVMType(ctxt_->module_, simdLength);

            // do return value optimization or create temporary
            Place* place = 0;

            if ( call->initPlaces_ && (*call->initPlaces_)[i] )
                place = (*call->initPlaces_)[i];

            Value* arg = place 
                       ? place->getAddr(builder_) 
                       : createEntryAlloca(builder_, llvmType);

            args.push_back(arg);
            perRefRetValues.push_back(arg);
        }
    }

    // append regular arguments
    call->exprList_->getArgs(builder_, args);

    llvm::Function* llvmFct = call->simd_ ? fct->simdFct_ : fct->llvmFct_;
    swiftAssert(llvmFct, "must exist");

    // create actual call
    llvm::CallInst* callInst = llvm::CallInst::Create( 
            llvmFct, args.begin(), args.end() );
    callInst->setCallingConv(llvm::CallingConv::Fast);
    Value* retValue = builder_.Insert(callInst);

    /*
     * write results back
     */

    swiftAssert( call->numResults() == out.size(), "sizes must match" );

    size_t idxRetType = 0;
    size_t idxPerRef = 0;
    for (size_t i = 0; i < out.size(); ++i)
    {
        call->set(i).place_ = out[i]->perRef() 
            ? (Place*) new Addr( perRefRetValues[idxPerRef++] )
            : (Place*) new Scalar( builder_.CreateExtractValue(retValue, idxRetType++) );
    }
}

void TypeNodeCodeGen::setResult(TypeNode* tn, Place* place)
{
    swiftAssert( tn->numResults() == 1, "must exactly have one result" );
    tn->set().place_ = place;
}

} // namespace swift
