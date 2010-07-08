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

    typedef AssignStmnt::Call ASCall;

    Module* module = ctxt_->module_;

    TNList& lhs = *s->tuple_;
    TNList& rhs = *s->exprList_;

    // analyze args and tuple
    rhs.accept(tna_);
    lhs.accept(tna_);

    swiftAssert( lhs.numTypeNodes() != 0 && rhs.numTypeNodes() != 0,
            "there must be at least one item on the left- "
            "and one on the right-hand side" );
    swiftAssert( lhs.numTypeNodes() == lhs.numResults(),
            "the tuple may not introduce additional nodes");

    size_t numLhs = lhs.numTypeNodes();

    if ( rhs.numResults() == 0 )
    {
        errorf( s->loc(), "right-hand side does not return anything" );
        ctxt_->result_ = false;
        return;
    }

    if ( numLhs == 1 && rhs.numResults() > 1 )
    {
        /*
         * do we have case 1 or 2?
         */

        const TypeNode* left = lhs.getTypeNode(0);
        std::string str, name;

        if ( dynamic<Decl>(left) )
        {
            str = "create";
            name = "contructor";
            s->kind_ = AssignStmnt::CREATE;
        }
        else
        {
            str = *s->id_;
            name = "assignment";
            s->kind_ = AssignStmnt::ASSIGN;
        }

        if ( const BaseType* bt = left->get().type_->cast<BaseType>() )
        {
            Class* _class = bt->lookupClass(module);
            MemberFct* fct = _class->lookupMemberFct( module, &str, rhs.typeList() );

            if (!fct)
            {
                errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                        name.c_str(), 
                        str.c_str(), 
                        rhs.typeList().toString().c_str(), 
                        _class->cid() );

                ctxt_->result_ = false;
                return;
            }

            s->calls_.push_back( ASCall(ASCall::USER, fct) );
        }
        else if ( const Ptr* ptr = left->get().type_->cast<Ptr>() )
        {
            // ptr only suports a copy create and assign
            errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                    name.c_str(), 
                    str.c_str(), 
                    rhs.typeList().toString().c_str(), 
                    ptr->toString().c_str() );

            ctxt_->result_ = false;
            return;
        }
        else if ( const Container* c = left->get().type_->cast<Container>() )
        {
            // array and simd only suport a copy create and assign, 
            // and an constructor taking an index value
            errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                    name.c_str(), 
                    str.c_str(), 
                    rhs.typeList().toString().c_str(), 
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

        if ( rhs.numTypeNodes() > 1 )
        {
            errorf(s->loc(), "todo");
            ctxt_->result_ = false;
            return;
        }

        if ( numLhs != rhs.numResults() )
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
            const TypeNode* tn = lhs.getTypeNode(i);
            std::string str, name;
            bool isCreate;

            const Type* lType  = lhs.typeList()[i];
            const Type* rType  = rhs.typeList()[i];

            if ( dynamic<Decl>(tn) )
            {
                // is this a copy create and the caller initializes this value?
                if ( rhs.getResult(i).inits_ && lType->check(rType, module) )
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

            tmp[0] = rhs.typeList()[i];

            if ( const BaseType* bt = lType->cast<BaseType>() )
            {
                Class* _class = bt->lookupClass(module);
                Method* fct = cast<Method>( _class->lookupMemberFct(module, &str, tmp) );

                if (!fct)
                {
                    errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                            name.c_str(), 
                            str.c_str(), 
                            rType->toString().c_str(), 
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

                    if ( fct->isAutoGenerated() )
                        s->calls_.push_back( ASCall(ASCall::COPY) );
                    else
                        s->calls_.push_back( ASCall(ASCall::USER, fct) );
                }
            }
            else if ( const Ptr* ptr = lType->cast<Ptr>() )
            {
                if ( !ptr->check(rType, module) )
                {
                    // ptr only suports a copy create and assign
                    errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                            name.c_str(), 
                            str.c_str(), 
                            rType->toString().c_str(),
                            ptr->toString().c_str() );

                    ctxt_->result_ = false;
                }

                // use a simple copy
                s->calls_.push_back( ASCall(ASCall::COPY) );
            }
            else if ( const Container* c = lType->cast<Container>() )
            {
                if ( c->check(rType, module) )
                    s->calls_.push_back( ASCall(ASCall::CONTAINER_COPY) );
                else if ( rType->isIndex() /*&& isCreate*/ )
                    s->calls_.push_back( ASCall(ASCall::CONTAINER_CREATE) );
                else
                {
                    // array and simd only suport a copy create and assign, 
                    // and an constructor taking an index value
                    errorf( s->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                            name.c_str(), 
                            str.c_str(), 
                            rType->toString().c_str(),
                            c->toString().c_str() );

                    ctxt_->result_ = false;
                }
            }
            else
            {
                swiftAssert( lType->cast<ErrorType>(), "unreachable" );
            }
        } // for each lhs/rhs pair

        if (ctxt_->result_)
        {
            swiftAssert( s->calls_.size() == rhs.numResults(), 
                    "sizes must match" );
        }
    }
}

void StmntAnalyzer::visit(ExprStmnt* s)
{
    s->expr_->accept(tna_);
}

} // namespace swift
