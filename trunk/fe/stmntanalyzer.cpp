#include "fe/stmntanalyzer.h"

#include <typeinfo>

#include "utils/cast.h"

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/fct.h"
#include "fe/tnlist.h"
#include "fe/scope.h"
#include "fe/type.h"
#include "fe/typenodeanalyzer.h"

namespace swift {

StmntAnalyzer::StmntVisitor(Context* ctxt)
    : StmntVisitorBase(ctxt)
    , tna_( new TypeNodeAnalyzer(ctxt) )
{}

StmntAnalyzer::~StmntVisitor()
{
    delete tna_;
}

void StmntAnalyzer::visit(ErrorStmnt* s) {}

void StmntAnalyzer::visit(CFStmnt* s) 
{
    // TODO
}

void StmntAnalyzer::visit(DeclStmnt* s)
{
    s->decl_->accept(tna_);
}

void StmntAnalyzer::visit(IfElStmnt* s)
{
    s->expr_->accept(tna_);

    if ( s->expr_->numResults() != 1 || !s->expr_->get().type_->isBool() )
    {
        errorf(s->expr_->loc(), 
                "the check condition of an if-clause must return a 'bool'");
        ctxt_->result_ = false;
    }

    s->ifScope_->accept(this);

    if (s->elScope_)
        s->elScope_->accept(this);
}

void StmntAnalyzer::visit(RepeatUntilLoop* l)
{
    l->expr_->accept(tna_);

    if ( l->expr_->numResults() != 1 || !l->expr_->get().type_->isBool() )
    {
        errorf(l->expr_->loc(), 
                "the exit condition of a repeat-unitl statement must return a 'bool'");
        ctxt_->result_ = false;
    }

    l->scope_->accept(this);
}

void StmntAnalyzer::visit(WhileLoop* l)
{
    l->expr_->accept(tna_);

    if ( l->expr_->numResults() != 1 || !l->expr_->get().type_->isBool() )
    {
        errorf(l->expr_->loc(), 
                "the exit condition of a while statement must return a 'bool'");
        ctxt_->result_ = false;
    }

    l->scope_->accept(this);
}

void StmntAnalyzer::visit(SimdLoop* l)
{
    l->lExpr_->accept(tna_);

    if ( l->lExpr_->numResults() != 1 || !l->lExpr_->get().type_->isIndex() )
    {
        errorf(l->lExpr_->loc(), 
                "the lower bound of an simd loop header must return an 'index'");
        ctxt_->result_ = false;
    }

    l->rExpr_->accept(tna_);

    if ( l->rExpr_->numResults() != 1 || !l->rExpr_->get().type_->isIndex() )
    {
        errorf(l->rExpr_->loc(), 
                "the upper bound of an simd loop header must return an 'index'");
        ctxt_->result_ = false;
    }


    // register counter if applicable
    if (l->id_)
    {
        l->index_ = new Local(
                l->loc(), 
                new ScalarType(l->loc(), Token::CONST, new std::string("index")),
                new std::string(*l->id_) );

        l->scope_->insert(l->index_);
    }

    // mark context as "within simd loop"
    ctxt_->simdIndex_ = (llvm::Value*) 1;
    l->scope_->accept(this);
    ctxt_->simdIndex_ = 0;
}

void StmntAnalyzer::visit(ScopeStmnt* s) 
{
    s->scope_->accept(this);
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
     *  A a, b, C c, d, ... = f   (AssignStmnt::PAIRWISE)
     *
     *  Here the number of lhs items (which equals the number of lhs return
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

    TNList* lhs = s->tuple_;
    TNList* rhs = s->exprList_;

    // analyze args and tuple
    rhs->accept(tna_);
    lhs->accept(tna_);

    swiftAssert( lhs->numTypeNodes() != 0 && rhs->numTypeNodes() != 0,
            "there must be at least one item on the left- "
            "and one on the right-hand side" );
    swiftAssert( lhs->numTypeNodes() == lhs->numResults(),
            "the tuple may not introduce additional nodes");

    size_t numLhs = lhs->numTypeNodes();

    if ( rhs->numResults() == 0 )
    {
        errorf( s->loc(), "right-hand side does not return anything" );
        ctxt_->result_ = false;
        return;
    }

    if ( numLhs == 1 && rhs->numResults() > 1 )
    {
        // -> case 1 or 2
        s->kind_ = AssignStmnt::SINGLE;

        TypeNode* left = lhs->getTypeNode(0);
        s->acs_.resize(1);
        s->acs_[0] = AssignCreate( ctxt_, s->loc(), s->id_, left, rhs );
    }
    else
    {
        // -> case 3

        if ( rhs->numTypeNodes() > 1 )
        {
            errorf(s->loc(), "todo");
            ctxt_->result_ = false;
            return;
        }

        if ( numLhs != rhs->numResults() )
        {
            errorf( s->loc(), "the number of left-hand side items must match "
                    "the number of returned values on the right-hand side here" );
            ctxt_->result_ = false;
            return;
        }

        // -> case 3
        s->kind_ = AssignStmnt::PAIRWISE;

        // fill for each lhs/rhs pair s->acs_
        for (size_t i = 0; i < numLhs; ++i)
        {
            TypeNode* left = lhs->getTypeNode(i);
            s->acs_.push_back( AssignCreate(ctxt_, s->loc(), s->id_, left, rhs, i, i+1) );

        } // for each lhs/rhs pair

        if (ctxt_->result_)
            swiftAssert( s->acs_.size() == rhs->numResults(), "sizes must match" );

    }

    // finally check each constructor/assign call
    for (size_t i = 0; i < s->acs_.size(); ++i)
        s->acs_[i].check();
}

void StmntAnalyzer::visit(ExprStmnt* s)
{
    s->expr_->accept(tna_);
}

} // namespace swift
