#include "fe/classanalyzer.h"

#include "fe/error.h"
#include "fe/scope.h"
#include "fe/stmnt.h"
#include "fe/stmntanalyzer.h"
#include "fe/type.h"

namespace swift {

ClassAnalyzer::ClassAnalyzer(Context& ctxt)
    : ClassVisitor(ctxt)
{}

void ClassAnalyzer::visit(Class* c) {}

void ClassAnalyzer::visit(Create* c)
{
    checkSig(c);
    checkStmnts(c);
}

void ClassAnalyzer::visit(Reader* r)
{
    checkSig(r);
    checkStmnts(r);
}

void ClassAnalyzer::visit(Writer* w)
{
    checkSig(w);
    checkStmnts(w);
}

void ClassAnalyzer::visit(Assign* a)
{
    checkSig(a);
    TypeList& in = a->sig_.inTypes_;

    if ( in.empty() )
    {
        errorf( a->loc(), "an assignment must at least have one parameter" );
        ctxt_.result_ = false;
    }
    else
    {
        const BaseType* bt = in[0]->isInner();
        if (bt && bt->lookupClass(ctxt_.module_) != ctxt_.class_ )
        {
            errorf(a->loc(), "first parameter of an assignment must be of class '%s'", 
                    ctxt_.class_->cid());
            ctxt_.result_ = false;
        }
    }

    checkStmnts(a);
}

void ClassAnalyzer::visit(Operator* o)
{
    checkSig(o);
    TypeList& in = o->sig_.inTypes_;

    Operator::NumIns numIns = o->getNumIns();

    switch (numIns)
    {
        case Operator::ONE:
            if (in.size() != 1)
            {
                errorf(o->loc(), "'operator %s' must exactly have one parameter", 
                        o->cid());
                ctxt_.result_ = false;
                goto check_stmnts;
            }
            break;
            
        case Operator::TWO:
            if (in.size() != 2)
            {
                errorf(o->loc(), "'operator %s' must exactly have two parameters", 
                        o->cid());
                ctxt_.result_ = false;
                goto check_stmnts;
            }
            break;
            
        case Operator::ONE_OR_TWO:
            if ( in.size() != 1 && in.size() != 2 )
            {
                errorf(o->loc(), "'operator %s' must exactly have one or two parameters", 
                        o->cid());
                ctxt_.result_ = false;
                goto check_stmnts;
            }
            break;
    }

    {
        const BaseType* bt = in[0]->isInner();
        if (bt && bt->lookupClass(ctxt_.module_) != ctxt_.class_ )
        {
            errorf(o->loc(), "first parameter of 'operator %s' must be of class '%s'", 
                    o->cid(), ctxt_.class_->cid());
            ctxt_.result_ = false;
        }
    }

check_stmnts:
    checkStmnts(o);
}

void ClassAnalyzer::visit(Routine* r)
{
    checkSig(r);
    checkStmnts(r);
}

void ClassAnalyzer::visit(MemberVar* m)
{
}

void ClassAnalyzer::checkSig(MemberFct* m)
{
    // check each ingoing param
    for (size_t i = 0; i < m->sig_.in_.size(); ++i)
        ctxt_.result_ &= m->sig_.in_[i]->validate(ctxt_.module_);

    // check each outgoing param/result
    for (size_t i = 0; i < m->sig_.out_.size(); ++i)
        ctxt_.result_ &= m->sig_.out_[i]->validate(ctxt_.module_);
}

void ClassAnalyzer::checkStmnts(MemberFct* m)
{
    StmntAnalyzer stmntAnalyzer(ctxt_);
    m->scope_->accept(&stmntAnalyzer);
}

} // namespace swift
