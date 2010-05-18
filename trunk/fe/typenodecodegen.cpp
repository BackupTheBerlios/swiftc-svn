#include "fe/typenodecodegen.h"

#include <llvm/LLVMContext.h>

namespace swift {

TypeNodeCodeGen::TypeNodeVisitor(Context* ctxt, llvm::LLVMContext* llvmCtxt)
    : TypeNodeVisitorBase(ctxt)
    , llvmCtxt_(llvmCtxt)
{}

void TypeNodeCodeGen::visit(Decl* d)
{
}

void TypeNodeCodeGen::visit(Id* id)
{
}

void TypeNodeCodeGen::visit(Literal* l)
{
}

void TypeNodeCodeGen::visit(Nil* n)
{
}

void TypeNodeCodeGen::visit(Self* n)
{
}

void TypeNodeCodeGen:: preVisit(IndexExpr* i)
{
}

void TypeNodeCodeGen::postVisit(IndexExpr* i)
{
}

void TypeNodeCodeGen:: preVisit(MemberAccess* m)
{
}

void TypeNodeCodeGen::postVisit(MemberAccess* m)
{
}

void TypeNodeCodeGen:: preVisit(CCall* c)
{
}

void TypeNodeCodeGen::postVisit(CCall* c)
{
}

void TypeNodeCodeGen:: preVisit(ReaderCall* r)
{
}

void TypeNodeCodeGen::postVisit(ReaderCall* r)
{
}

void TypeNodeCodeGen:: preVisit(WriterCall* w)
{
}

void TypeNodeCodeGen::postVisit(WriterCall* w)
{
}

void TypeNodeCodeGen:: preVisit(BinExpr* b)
{
}

void TypeNodeCodeGen::postVisit(BinExpr* b)
{
}

void TypeNodeCodeGen:: preVisit(RoutineCall* r)
{
}

void TypeNodeCodeGen::postVisit(RoutineCall* r)
{
}

void TypeNodeCodeGen:: preVisit(UnExpr* u)
{
}

void TypeNodeCodeGen::postVisit(UnExpr* u)
{
}

} // namespace swift
