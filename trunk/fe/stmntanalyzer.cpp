#include "fe/stmntanalyzer.h"

#include <typeinfo>

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/tnlist.h"
#include "fe/scope.h"
#include "fe/type.h"
#include "fe/typenodeanalyzer.h"

namespace swift {

StmntAnalyzer::StmntVisitor(Context* ctxt)
    : StmntVisitorBase(ctxt)
    , tna_( new TypeNodeAnalyzer(ctxt) )
{}

void StmntAnalyzer::visit(ErrorStmnt* s) {}

void StmntAnalyzer::visit(CFStmnt* s) 
{
    // todo
}

void StmntAnalyzer::visit(DeclStmnt* s)
{
    s->decl_->accept( tna_.get() );
}

void StmntAnalyzer::visit(IfElStmnt* s)
{
    s->expr_->accept( tna_.get() );

    if ( s->expr_->size() != 1 || !s->expr_->getType()->isBool() )
    {
        errorf(s->expr_->loc(), 
                "the check condition of an if-clause must return a 'bool'");
        ctxt_->result_ = false;
    }

    s->ifScope_->accept(this, ctxt_);

    if (s->elScope_)
        s->elScope_->accept(this, ctxt_);
}

void StmntAnalyzer::visit(RepeatUntilStmnt* s)
{
    s->expr_->accept( tna_.get() );

    if ( s->expr_->size() != 1 || !s->expr_->getType()->isBool() )
    {
        errorf(s->expr_->loc(), 
                "the exit condition of a repeat-unitl statement must return a 'bool'");
        ctxt_->result_ = false;
    }

    s->scope_->accept(this, ctxt_);
}

void StmntAnalyzer::visit(ScopeStmnt* s) 
{
    s->scope_->accept(this, ctxt_);
}

void StmntAnalyzer::visit(WhileStmnt* s)
{
    s->expr_->accept( tna_.get() );

    if ( s->expr_->size() != 1 || !s->expr_->getType()->isBool() )
    {
        errorf(s->expr_->loc(), 
                "the exit condition of a while statement must return a 'bool'");
        ctxt_->result_ = false;
    }

    s->scope_->accept(this, ctxt_);
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
     *
     *      a = f()
     */

    size_t lSize = s->tuple_->size();
    size_t rSize = s->exprList_->size();

    swiftAssert( rSize != 0 && lSize != 0,
            "there must be at least one item on the left- "
            "and one on the right-hand side" );

    if (rSize == 1)
    {
        if ( MemberFctCall* call = 
                dynamic_cast<MemberFctCall*>(s->exprList_->getTypeNode(0)) )
        {
            // mark stmnt as call stmnt
            s->kind_ = AssignStmnt::CALL;

            // analyze args
            s->tuple_->accept( tna_.get() );

            // analyze call
            call->setTuple(s->tuple_);
            call->accept( tna_.get() );

            return;
        }
        else if (lSize != 1)
        {
            errorf( s->loc(), "the right-hand side of an assignment statement with "
                    "more than one item on the left-hand side " 
                    "must be a member function call");
            ctxt_->result_ = false;

            return;
        }
    }

    // analyze args and tuple
    s->tuple_->accept( tna_.get() );
    s->exprList_->accept( tna_.get() );

    const TypeList& in = s->exprList_->typeList();
    //const TypeList& out = s->tuple_->typeList();

    if ( rSize != 1 && lSize != 1 )
    {
        errorf( s->loc(), "either the left-hand side or the right-hand side of an " 
                "assignment statement must have exactly one element");
        ctxt_->result_ = false;
        return;
    }

    TypeNode* lhs = s->tuple_->getTypeNode(0);
    swiftAssert(lhs->size() == 1, "must return one item");

    std::string str, name;
    if ( typeid(*lhs) == typeid(Decl) )
    {
        str = "create";
        name = "contructor";
        s->kind_ = AssignStmnt::CREATE;
    }
    else
    {
        str = "=";
        name = "operator";
        s->kind_ = AssignStmnt::ASSIGN;
    }

    if ( const BaseType* bt = lhs->getType()->cast<BaseType>() )
    {
        Class* _class = bt->lookupClass(ctxt_->module_);
        MemberFct* fct = _class->lookupMemberFct(ctxt_->module_, &str, in);

        if (!fct)
        {
            errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                    name.c_str(), str.c_str(), in.toString().c_str(), _class->cid() );
            ctxt_->result_ = false;
            return;
        }
    }
}

void StmntAnalyzer::visit(ExprStmnt* s)
{
    if (s->expr_)
        s->expr_->accept( tna_.get() );
}

} // namespace swift
