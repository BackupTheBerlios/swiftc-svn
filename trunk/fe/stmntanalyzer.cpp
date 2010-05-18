#include "fe/stmntanalyzer.h"

#include <typeinfo>

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/scope.h"
#include "fe/type.h"
#include "fe/typenodeanalyzer.h"

namespace swift {

StmntAnalyzer::StmntVisitor(Context* ctxt)
    : StmntVisitorBase(ctxt)
{}

void StmntAnalyzer::visit(CFStmnt* s) {}

void StmntAnalyzer::visit(DeclStmnt* s)
{
    TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
    s->decl_->accept(&typeNodeAnalyzer);
}

void StmntAnalyzer::visit(IfElStmnt* s)
{
    TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
    s->expr_->accept(&typeNodeAnalyzer);

    if ( !s->expr_->getType()->isBool() )
    {
        errorf(s->expr_->loc(), 
                "the check condition of an if-clause must return a 'bool'");
        ctxt_->result_ = false;
    }
}

void StmntAnalyzer::visit(RepeatUntilStmnt* s)
{
    TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
    s->expr_->accept(&typeNodeAnalyzer);

    if ( !s->expr_->getType()->isBool() )
    {
        errorf(s->expr_->loc(), 
                "the exit condition of a repeat-unitl statement must return a 'bool'");
        ctxt_->result_ = false;
    }
}

void StmntAnalyzer::visit(ScopeStmnt* s) {}

void StmntAnalyzer::visit(WhileStmnt* s)
{
    TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
    s->expr_->accept(&typeNodeAnalyzer);

    if ( !s->expr_->getType()->isBool() )
    {
        errorf(s->expr_->loc(), 
                "the exit condition of a while statement must return a 'bool'");
        ctxt_->result_ = false;
    }
}

void StmntAnalyzer::visit(AssignStmnt* s)
{
    /*
     * Here are 3 cases:
     *
     * 1. a contructor call
     *      Type t = a, b, ...
     *
     * 2. an assignment
     *      t = a, b, ...
     *
     * 3. a normal function call that returns several return values:
     *      a, b, ... = f()
     */

    TypeNodeAnalyzer typeNodeAnalyzer(ctxt_);
    s->tuple_->accept(&typeNodeAnalyzer);
    s->exprList_->accept(&typeNodeAnalyzer);

    if ( !s->tuple_->isValid() || !s->exprList_->isValid() )
        return;

    TypeList in = s->exprList_->buildTypeList();
    TypeList out = s->tuple_->buildTypeList();

    if ( in.size() != 1 && out.size() != 1 )
    {
        errorf( s->loc(), "either the left-hand side or the right-hand side of an " 
                "assignment statement must have exactly one element");
        ctxt_->result_ = false;
        return;
    }

    if (out.size() != 1)
    {
        MemberFctCall* call = dynamic_cast<MemberFctCall*>(s->exprList_->expr_);

        if (!call)
        {
            errorf( s->loc(), "the right-hand side of an assignment statement with "
                    "more than one item on the left-hand side " 
                    "must be a member function call");
            ctxt_->result_ = false;
            return;
        }

        MemberFct* fct = call->memberFct_;
        if ( fct && !fct->sig_.checkOut(ctxt_->module_, out) )
        {
            errorf( s->loc(), "bier" );
            ctxt_->result_ = false;
            return;
        }
    }

    TypeNode* lhs = s->tuple_->typeNode_;
    std::string str, name;
    if ( typeid(*lhs) == typeid(Decl) )
    {
        str = "create";
        name = "contructor";
    }
    else
    {
        str = "=";
        name = "operator";
    }

    const BaseType* bt = lhs->getType()->isInner();
    Class* _class = bt->lookupClass(ctxt_->module_);
    MemberFct* fct = _class->lookupMemberFct(ctxt_->module_, &str, in);

    if (!fct)
    {
        errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                name.c_str(), str.c_str(), in.toString().c_str(), _class->cid() );
    }
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
