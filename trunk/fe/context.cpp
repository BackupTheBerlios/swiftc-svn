#include "fe/context.h"

#include "utils/cast.h"
#include "utils/llvmhelper.h"

#include "fe/tnlist.h"
#include "fe/node.h"
#include "fe/scope.h"

#include <llvm/BasicBlock.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm/Target/TargetData.h>

using llvm::Value;

namespace swift {

Context::Context(Module* module)
    : result_(true)
    , module_(module)
    , tuple_( new TNList() )
    , builder_( LLVMBuilder(*module->lctxt_) )
    , simdLoop_(false)
{}

Context::~Context()
{
    delete tuple_;
}

Scope* Context::enterScope()
{
    Scope* parent = scopes_.empty() ? 0 : scopes_.top();
    Scope* newScope = new Scope(parent);
    scopes_.push(newScope);

    return newScope;
}

void Context::enterScope(Scope* scope)
{
    scopes_.push(scope);
}

void Context::leaveScope()
{
    scopes_.pop();
}

size_t Context::scopeDepth() const
{
    return scopes_.size();
}

Scope* Context::scope()
{
    return scopes_.top();
}

void Context::pushExprList()
{
    exprLists_.push( new TNList() );
}

TNList* Context::popExprList()
{
    TNList* exprList = exprLists_.top();
    exprLists_.pop();

    return exprList;
}

TNList* Context::topExprList() const
{
    return exprLists_.top();
}

void Context::newTuple()
{
    tuple_ = new TNList();
}

Value* Context::createMalloc(Value* size, const llvm::PointerType* ptrType)
{
    const llvm::Type* allocType = ptrType->getContainedType(0);

    llvm::TargetData td( module_->getLLVMModule() );
    Value* allocSize = createInt64( lctxt(), td.getTypeStoreSize(allocType) );

    Value* mallocSize = builder_.CreateMul(size, allocSize, "malloc-size");

    llvm::CallInst* call = llvm::CallInst::Create(malloc_, mallocSize, "malloc-ptr");
    //call->setTailCall();
    call->addAttribute(0, llvm::Attribute::NoAlias);
    call->addAttribute(~0, llvm::Attribute::NoUnwind);
    builder_.Insert(call);

    return builder_.CreateBitCast(call, ptrType, "malloc-ptr");
}

void Context::createMemCpy(Value* dst, Value* src, Value* size)
{
    const llvm::PointerType* ptrType = ::cast<llvm::PointerType>( src->getType() );
    const llvm::Type* allocType = ptrType->getContainedType(0);

    llvm::TargetData td( module_->getLLVMModule() );
    Value* allocSize = createInt64( lctxt(), td.getTypeStoreSize(allocType) );

    Value* cpySize = builder_.CreateMul(size, allocSize, "memcpy-size");

    Values args(3);
    args[0] = builder_.CreateBitCast(dst, llvm::PointerType::getInt8PtrTy(lctxt()) );
    args[1] = builder_.CreateBitCast(src, llvm::PointerType::getInt8PtrTy(lctxt()) );
    args[2] = cpySize;

    llvm::CallInst* call = llvm::CallInst::Create( memcpy_, args.begin(), args.end() );
    call->setTailCall();
    call->addAttribute(~0, llvm::Attribute::NoUnwind);
    builder_.Insert(call);
}

llvm::LLVMContext& Context::lctxt()
{
    return *module_->lctxt_;
}

llvm::Module* Context::lmodule()
{
    return module_->getLLVMModule();
}

} // namespace swift
