#include "fe/stmntanalyzer.h"

#include "fe/error.h"
#include "fe/scope.h"
#include "fe/type.h"
#include "fe/typenodeanalyzer.h"

namespace swift {

StmntAnalyzer::StmntAnalyzer(Context& ctxt)
    : StmntVisitor(ctxt)
{}

void StmntAnalyzer::visit(CFStmnt* s)
{
}

void StmntAnalyzer::visit(DeclStmnt* s)
{
    TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
    s->decl_->accept(&typeNodeAnalyzer);
}

void StmntAnalyzer::visit(IfElStmnt* s)
{
    TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
    s->expr_->accept(&typeNodeAnalyzer);
}

void StmntAnalyzer::visit(RepeatUntilStmnt* s)
{
    TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
    s->expr_->accept(&typeNodeAnalyzer);

    if ( !s->expr_->type_->isBool() )
    {
        errorf(s->expr_->loc(), 
                "the exit condition of a repeat-unitl statement must return a 'bool'");
        ctxt_.result_ = false;
    }
}

void StmntAnalyzer::visit(ScopeStmnt* s) {}

void StmntAnalyzer::visit(WhileStmnt* s)
{
    TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
    s->expr_->accept(&typeNodeAnalyzer);

    if ( !s->expr_->type_->isBool() )
    {
        errorf(s->expr_->loc(), 
                "the exit condition of a while statement must return a 'bool'");
        ctxt_.result_ = false;
    }
}

void StmntAnalyzer::visit(AssignStmnt* s)
{
    TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
    s->tuple_->accept(&typeNodeAnalyzer);
    s->exprList_->accept(&typeNodeAnalyzer);
}

void StmntAnalyzer::visit(ExprStmnt* s)
{
    if (s->expr_)
    {
        TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
        s->expr_->accept(&typeNodeAnalyzer);
    }
}

} // namespace swift
