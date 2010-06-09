#include "fe/context.h"

#include "fe/tnlist.h"
#include "fe/node.h"
#include "fe/scope.h"

#include <llvm/BasicBlock.h>
#include <llvm/Function.h>

namespace swift {

Context::Context(Module* module)
    : result_(true)
    , module_(module)
    , tuple_( new TNList() )
    , builder_( llvm::IRBuilder<>(*module->llvmCtxt_) )
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

llvm::AllocaInst* Context::createEntryAlloca(
        const llvm::Type* llvmType, 
        const llvm::Twine& name /*= ""*/) const
{
    llvm::BasicBlock* entry = &llvmFct_->getEntryBlock();
    llvm::IRBuilder<> tmpBuilder( entry, entry->begin() );

    return tmpBuilder.CreateAlloca(llvmType, 0, name);
}

} // namespace swift
