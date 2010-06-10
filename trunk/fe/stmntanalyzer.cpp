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
     * Here are three cases:
     *
     * 1. A constructor call:
     *  A a = b, c, ...             (AssignStmnt::CREATE)
     *
     * 2. An assignment call:       (AssignStmnt::ASSIGN)
     *  a = b, c, ...
     *
     * 3. A more complicated assignment:
     *  A a, b, C c, d, ... = ...   (AssignStmnt::MULTIPLE)
     *
     *  Here the number of rhs items (which equals the number of lhs return
     *  values) must match the number of return values on the rhs.  For each
     *  (lhs value, rhs value ) =: (l, r)  pair the following is done:
     *
     *      a) If l is a Decl and r is an init value, l is set to this value.
     *      No further calls are done.
     *
     *      b) If l is a Decl but r is not an init value, the copy constructor
     *      is called in order to initialize l.
     *
     *      c) Otherwise the copy assignment with r as argumet is called in
     *      order set l.
     */

    size_t numLhs = s->tuple_->numItems();

    swiftAssert( numLhs != 0 && s->exprList_->numItems() != 0,
            "there must be at least one item on the left- "
            "and one on the right-hand side" );

    // analyze args and tuple
    s->exprList_->accept( tna_.get() );
    s->tuple_->accept( tna_.get() );

    const TypeList& in = s->exprList_->typeList();
    const TypeList& out = s->tuple_->typeList();

    swiftAssert(numLhs == out.size(), 
            "the tuple may not introduce additional nodes");

    if ( in.empty() )
    {
        errorf( s->loc(), "right-hand side does not return anything" );
        ctxt_->result_ = false;
        return;
    }


    if ( numLhs == 1 && in.size() > 1 )
    {
        /*
         * do we have case 1 or 2?
         */

        TypeNode* lhs = s->tuple_->getTypeNode(0);
        std::string str, name;

        if ( dynamic_cast<Decl*>(lhs) )
        {
            str = "create";
            name = "contructor";
            s->kind_ = AssignStmnt::CREATE;
        }
        else
        {
            str = "=";
            name = "assignment";
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

            s->fcts_.push_back(fct);
        }
    }
    else
    {
        /*
         * we have case 3
         */

        if ( numLhs != in.size() )
        {
            errorf( s->loc(), "the number of left-hand side items must match "
                    "the number or returned values on the right-hand side here" );
            ctxt_->result_ = false;
            return;
        }

        if ( !in.check(ctxt_->module_, out) )
        {
            errorf( s->loc(), "the types of left-hand side (%s) must match "
                    "the types of the right-hand side (%s)",
                    in.toString().c_str(), out.toString().c_str() );
            ctxt_->result_ = false;
            return;
        }

        s->kind_ = AssignStmnt::MULTIPLE;

        // check each lhs/rhs pair
        for (size_t i = 0; i < numLhs; ++i)
        {
            TypeNode* tn = s->tuple_->getTypeNode(i);
            std::string str, name;

            if ( dynamic_cast<Decl*>(tn) )
            {
                str = "create";
                name = "contructor";

                if ( s->exprList_->isInit(i) )
                {
                    s->fcts_.push_back(0); // no call needed for this pair
                    continue;
                }
            }
            else
            {
                str = "=";
                name = "assignment";
            }

            TypeList tmp;
            tmp.push_back( in[i] );

            if ( const BaseType* bt = out[i]->cast<BaseType>() )
            {
                Class* _class = bt->lookupClass(ctxt_->module_);
                MemberFct* fct = _class->lookupMemberFct(ctxt_->module_, &str, tmp);

                if (!fct)
                {
                    errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                            name.c_str(), str.c_str(), in.toString().c_str(), _class->cid() );
                    ctxt_->result_ = false;
                }

                s->fcts_.push_back(fct);
            }
            else
            {
                s->fcts_.push_back(0); // TOOD
            }
        }

        swiftAssert( s->fcts_.size() == s->exprList_->numRetValues(), "sizes must match" );
    }
}

void StmntAnalyzer::visit(ExprStmnt* s)
{
    s->expr_->accept( tna_.get() );
}

} // namespace swift
