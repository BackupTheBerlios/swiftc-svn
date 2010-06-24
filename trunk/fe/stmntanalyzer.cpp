#include "fe/stmntanalyzer.h"

#include <typeinfo>

#include "utils/cast.h"

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

void StmntAnalyzer::visit(RepeatUntilLoop* l)
{
    l->expr_->accept( tna_.get() );

    if ( l->expr_->size() != 1 || !l->expr_->getType()->isBool() )
    {
        errorf(l->expr_->loc(), 
                "the exit condition of a repeat-unitl statement must return a 'bool'");
        ctxt_->result_ = false;
    }

    l->scope_->accept(this, ctxt_);
}

void StmntAnalyzer::visit(WhileLoop* l)
{
    l->expr_->accept( tna_.get() );

    if ( l->expr_->size() != 1 || !l->expr_->getType()->isBool() )
    {
        errorf(l->expr_->loc(), 
                "the exit condition of a while statement must return a 'bool'");
        ctxt_->result_ = false;
    }

    l->scope_->accept(this, ctxt_);
}

void StmntAnalyzer::visit(SimdLoop* l)
{
    l->lExpr_->accept( tna_.get() );

    if ( l->lExpr_->size() != 1 || !l->lExpr_->getType()->isIndex() )
    {
        errorf(l->lExpr_->loc(), 
                "the lower bound of an simd loop header must return an 'index'");
        ctxt_->result_ = false;
    }

    l->rExpr_->accept( tna_.get() );

    if ( l->rExpr_->size() != 1 || !l->rExpr_->getType()->isIndex() )
    {
        errorf(l->rExpr_->loc(), 
                "the upper bound of an simd loop header must return an 'index'");
        ctxt_->result_ = false;
    }

    l->scope_->accept(this, ctxt_);
}


void StmntAnalyzer::visit(ScopeStmnt* s) 
{
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

    typedef AssignStmnt::Call ASCall;

    Module* module = ctxt_->module_;
    size_t numLhs = s->tuple_->numItems();

    swiftAssert( numLhs != 0 && s->exprList_->numItems() != 0,
            "there must be at least one item on the left- "
            "and one on the right-hand side" );

    // analyze args and tuple
    s->exprList_->accept( tna_.get() );
    s->tuple_->accept( tna_.get() );

    const TypeList& rhs = s->exprList_->typeList();
    const TypeList& lhs = s->tuple_->typeList();

    swiftAssert(numLhs == lhs.size(), 
            "the tuple may not introduce additional nodes");

    if ( rhs.empty() )
    {
        errorf( s->loc(), "right-hand side does not return anything" );
        ctxt_->result_ = false;
        return;
    }

    if ( numLhs == 1 && rhs.size() > 1 )
    {
        /*
         * do we have case 1 or 2?
         */

        TypeNode* left = s->tuple_->getTypeNode(0);
        std::string str, name;

        if ( dynamic<Decl>(left) )
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

        if ( const BaseType* bt = left->getType()->cast<BaseType>() )
        {
            Class* _class = bt->lookupClass(module);
            MemberFct* fct = _class->lookupMemberFct(module, &str, rhs);

            if (!fct)
            {
                errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                        name.c_str(), 
                        str.c_str(), 
                        rhs.toString().c_str(), 
                        _class->cid() );

                ctxt_->result_ = false;
                return;
            }

            s->calls_.push_back( ASCall(ASCall::USER, fct) );
        }
        else if ( const Ptr* ptr = left->getType()->cast<Ptr>() )
        {
            // ptr only suports a copy create and assign
            errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                    name.c_str(), 
                    str.c_str(), 
                    rhs.toString().c_str(), 
                    ptr->toString().c_str() );

            ctxt_->result_ = false;
            return;
        }
        else if ( const Container* c = left->getType()->cast<Container>() )
        {
            // array and simd only suport a copy create and assign, 
            // and an constructor taking an index value
            errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                    name.c_str(), 
                    str.c_str(), 
                    rhs.toString().c_str(), 
                    c->toString().c_str() );

            ctxt_->result_ = false;
            return;
        }
        else
            swiftAssert(false, "unreachable");
    }
    else
    {
        /*
         * we have case 3
         */

        if ( s->exprList_->numItems() > 1 )
        {
            errorf(s->loc(), "todo");
            ctxt_->result_ = false;
            return;
        }

        if ( numLhs != rhs.size() )
        {
            errorf( s->loc(), "the number of left-hand side items must match "
                    "the number of returned values on the right-hand side here" );
            ctxt_->result_ = false;
            return;
        }

        s->kind_ = AssignStmnt::PAIRWISE;

        TypeList tmp;
        tmp.resize(1);

        // check each lhs/rhs pair
        for (size_t i = 0; i < numLhs; ++i)
        {
            TypeNode* tn = s->tuple_->getTypeNode(i);
            std::string str, name;
            bool isCreate;

            if ( dynamic<Decl>(tn) )
            {
                // is this a copy create and the caller initializes this value?
                if ( s->exprList_->isInit(i) && lhs[i]->check(rhs[i], module) )
                {
                    s->calls_.push_back( ASCall(ASCall::GETS_INITIALIZED_BY_CALLER) );
                    continue;
                }

                str = "create";
                name = "contructor";
                isCreate = true;
            }
            else
            {
                str = "=";
                name = "assignment";
                isCreate = false;
            }

            tmp[0] = rhs[i];

            if ( const BaseType* bt = lhs[i]->cast<BaseType>() )
            {
                Class* _class = bt->lookupClass(module);
                MemberFct* fct = _class->lookupMemberFct(module, &str, tmp);

                if (!fct)
                {
                    errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                            name.c_str(), 
                            str.c_str(), 
                            rhs.toString().c_str(), 
                            _class->cid() );

                    ctxt_->result_ = false;
                    s->calls_.push_back( ASCall(ASCall::EMPTY) );
                    continue;
                }

                if ( dynamic<ScalarType>(bt) )
                    s->calls_.push_back( ASCall(ASCall::COPY) );
                else
                {
                    swiftAssert( dynamic<UserType>(bt), 
                            "must be castable to BaseType" );

                    if (isCreate)
                    {
                        Create* create = cast<Create>(fct);
                        if ( create->isAutoCopy() )
                            s->calls_.push_back( ASCall(ASCall::COPY) );
                        else
                            s->calls_.push_back( ASCall(ASCall::USER, fct) );
                    }
                    else
                    {
                        Assign* assign = cast<Assign>(fct);
                        if ( assign->isAutoCopy() )
                            s->calls_.push_back( ASCall(ASCall::COPY) );
                        else
                            s->calls_.push_back( ASCall(ASCall::USER, fct) );
                    }
                }
            }
            else if ( const Ptr* ptr = lhs[i]->cast<Ptr>() )
            {
                if ( !ptr->check(rhs[i], module) )
                {
                    // ptr only suports a copy create and assign
                    errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                            name.c_str(), 
                            str.c_str(), 
                            rhs[i]->toString().c_str(),
                            ptr->toString().c_str() );

                    ctxt_->result_ = false;
                }

                // use a simple copy
                s->calls_.push_back( ASCall(ASCall::COPY) );
            }
            else if ( const Container* c = lhs[i]->cast<Container>() )
            {
                if ( c->check(rhs[i], module) )
                    s->calls_.push_back( ASCall(ASCall::CONTAINER_COPY) );
                else if ( rhs[i]->isIndex() && isCreate )
                    s->calls_.push_back( ASCall(ASCall::CONTAINER_CREATE) );
                else
                {
                    // array and simd only suport a copy create and assign, 
                    // and an constructor taking an index value
                    errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                            name.c_str(), 
                            str.c_str(), 
                            rhs[i]->toString().c_str(),
                            c->toString().c_str() );

                    ctxt_->result_ = false;
                }
            }
            else
            {
                swiftAssert( lhs[i]->cast<ErrorType>(), "unreachable" );
            }
        } // for each lhs/rhs pair

        if (ctxt_->result_)
        {
            swiftAssert( s->calls_.size() == s->exprList_->numRetValues(), 
                    "sizes must match" );
        }
    }
}

void StmntAnalyzer::visit(ExprStmnt* s)
{
    s->expr_->accept( tna_.get() );
}

} // namespace swift
