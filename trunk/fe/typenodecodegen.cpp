#include "fe/typenodecodegen.h"

namespace swift {

TypeNodeCodeGen::TypeNodeVisitor(Context* ctxt)
    : TypeNodeVisitorBase(ctxt)
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

void TypeNodeCodeGen::visit(IndexExpr* i)
{
}

void TypeNodeCodeGen::visit(MemberAccess* m)
{
}

void TypeNodeCodeGen::visit(CCall* c)
{
}

void TypeNodeCodeGen::visit(ReaderCall* r)
{
}

void TypeNodeCodeGen::visit(WriterCall* w)
{
}

void TypeNodeCodeGen::visit(BinExpr* b)
{
}

void TypeNodeCodeGen::visit(RoutineCall* r)
{
}

void TypeNodeCodeGen::visit(UnExpr* u)
{
}

} // namespace swift
