#include "fe/fct.h"

#include "utils/cast.h"
#include "utils/llvmplace.h"

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/tnlist.h"
#include "fe/type.h"
#include "fe/typenode.h"
#include "fe/typenodecodegen.h"

#include <llvm/Support/IRBuilder.h>
#include <llvm/Function.h>

using llvm::Value;

static void copy(LLVMBuilder& builder, Place* lPlace, Place* rPlace)
{
    Value* lvalue = lPlace->getAddr(builder);
    Value* rvalue = rPlace->getScalar(builder);
    builder.CreateStore(rvalue, lvalue);
    lPlace->writeBack(builder);
}

static void appendCall(LLVMBuilder& builder, llvm::Function* fct, Values::iterator begin, Values::iterator end)
{
    llvm::CallInst* call = llvm::CallInst::Create(fct, begin, end);
    call->setCallingConv(llvm::CallingConv::Fast);
    builder.Insert(call);
}

static void missingMemberFctError(const swift::location& loc,
                                  const std::string& kind, 
                                  const std::string& name,
                                  const swift::TypeList& types,
                                  const char* classId)
{
    errorf(loc, "there is no %s '%s(%s)' defined in class '%s'",
            kind.c_str(), 
            name.c_str(), 
            types.toString().c_str(), 
            classId);
}

namespace swift {

AssignCreate::AssignCreate(Context* ctxt,
                           const location& loc, const std::string* id,
                           TypeNode* lhs, TNList* rhs,
                           size_t rBegin /*= 0*/, size_t rEnd /*= INTPTR_MAX*/)
    : ctxt_(ctxt)
    , loc_(loc)
    , id_(id)
    , lhs_(lhs)
    , rhs_(rhs)
    , rBegin_(rBegin)
    , rEnd_( (rEnd == INTPTR_MAX) ? rhs->numResults() : rEnd )
    , lType_( lhs_->get().type_) 
    , isDecl_( dynamic<Decl>(lhs_) )
    , initsRhs_(false)
{
    swiftAssert( lhs_->numResults() == 1, "must exactly return one value" );

    for (size_t i = rBegin_; i < rEnd_; ++i)
        rTypes_.push_back( rhs_->getResult(i).type_ );
}

inline bool AssignCreate::isPairwise() const
{
    return rEnd_ - rBegin_ == 1;
}

void AssignCreate::check()
{
    std::string name = isDecl_ ? "create"      : *id_;
    std::string kind = isDecl_ ? "constructor" : "assignment";

    info_ = lType_->hasMemberFct(name, rTypes_, ctxt_->module_);

    if ( info_.kind_ == MemberFctInfo::FALSE )
    {
        missingMemberFctError( loc_, kind, name, rTypes_, lType_->toString().c_str() );
        ctxt_->result_ = false;
        return;
    }

    if (isPairwise() && rhs_->getResult(rBegin_).inits_ && isDecl_)
        initsRhs_ = true;

    if ( lType_->isSimd() )
    {
        bool allSimd = lType_->isSimd();
        bool noneSimd = !allSimd;

        for (size_t i = 0; i < rTypes_.size(); ++i)
        {
            bool simd = rTypes_[i]->isSimd();
            allSimd  &=  simd;
            noneSimd &= !simd;
        }

        bool error = false;

        if (!info_.simd_)
        {
            errorf( loc_,
                    "%s '%s(%s)' in class '%s' is not declared as "
                    "simd function but is used within a simd loop",
                    kind.c_str(), 
                    name.c_str(), 
                    rTypes_.toString().c_str(),  
                    lType_->toString().c_str() );

            error = true;
        }

        if ( !lType_->isSimd() )
        {
            errorf(loc_, "left hand value of assignment is not a simd value" );
            error = true;
        }

        for (size_t i = 0; i < rTypes_.size(); ++i)
        {
            if ( !rTypes_[i]->isSimd() )
            {
                errorf(loc_, "argument number %i in simd assignment is not a simd value", i);
                error = true;
            }
        }

        if (error)
        {
            ctxt_->result_ = false;
            return;
        }
    }
}

void AssignCreate::genCode()
{
    // return value optimization?
    if (initsRhs_)
        return;

    LLVMBuilder& builder = ctxt_->builder_;

    Place*& lPlace = lhs_->set().place_;

    switch (info_.kind_)
    {
        case MemberFctInfo::FALSE:
            swiftAssert(false, "unreachable");
            return;
        case MemberFctInfo::COPY:
        {
            swiftAssert(isPairwise(), "must exactly return one value");
            Place*& rPlace = rhs_->setResult(rBegin_).place_;
            copy(builder, lPlace, rPlace);
            break;
        }
        case MemberFctInfo::CONTAINER_COPY:
        {
            const Container* c = cast<Container>( lType_ );
            swiftAssert(isPairwise(), "must exactly return one value");
            Place*& rPlace = rhs_->setResult(rBegin_).place_;

            Value* dst = lPlace->getAddr(builder);
            Value* src = rPlace->getAddr(builder);
            c->emitCopy(ctxt_, dst, src);
            break;
        }
        case MemberFctInfo::CONTAINER_CREATE:
        {
            const Container* c = cast<Container>( lType_ );
            swiftAssert(isPairwise(), "must exactly return one value");
            Place*& rPlace = rhs_->setResult(rBegin_).place_;

            Value* lvalue = lPlace->getAddr(builder);
            Value* size = rPlace->getScalar(builder);
            c->emitCreate(ctxt_, lvalue, size);
            break;
        }
        case MemberFctInfo::USER:
        {
            MemberFct* memFct = info_.fct_;

            llvm::Function* llvmFct = lType_->isSimd() 
                                    ? memFct->simdFct() 
                                    : memFct->llvmFct();

            swiftAssert(llvmFct, "must be valid");

            // prepare args
            Values args;
            args.push_back( lPlace->getAddr(builder) );
            rhs_->getArgs(builder, args, rBegin_, rEnd_);

            // create call
            appendCall( builder, llvmFct, args.begin(), args.end() );

            break;
        }
    }

    lPlace->writeBack(builder);
}

} // namespace swift
