#include "fe/context.h"

#include "fe/scope.h"

namespace swift {

Context::Context()
    : result_(true)
{}

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

} // namespace swift
