#include "fe/classcodegen.h"

#include <llvm/LLVMContext.h>
#include <llvm/Support/TypeBuilder.h>

#include "fe/type.h"

namespace swift {

ClassCodeGen::ClassVisitor(Context* ctxt, llvm::LLVMContext* llvmCtxt)
    : ClassVisitorBase(ctxt)
    , llvmCtxt_(llvmCtxt)
{}

void ClassCodeGen::visit(Class* c)
{
    if ( BaseType::isBuiltin(c->id()) )
        return;

    //std::cout << c->cid() << std::endl;
}

void ClassCodeGen::visit(Create* c)
{
}

void ClassCodeGen::visit(Reader* r)
{
}

void ClassCodeGen::visit(Writer* w)
{
}

void ClassCodeGen::visit(Assign* a)
{
}

void ClassCodeGen::visit(Operator* o)
{
}

void ClassCodeGen::visit(Routine* r)
{
}

void ClassCodeGen::visit(MemberVar* m)
{
}

} // namespace swift
