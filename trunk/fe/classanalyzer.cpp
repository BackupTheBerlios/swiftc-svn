#include "fe/classanalyzer.h"

#include <algorithm>

#include "fe/context.h"
#include "fe/error.h"
#include "fe/scope.h"
#include "fe/stmnt.h"
#include "fe/stmntanalyzer.h"
#include "fe/type.h"

namespace swift {

ClassAnalyzer::ClassVisitor(Context* ctxt)
    : ClassVisitorBase(ctxt)
{}

ClassAnalyzer::~ClassVisitor() {}

//void ClassAnalyzer::visit(Class* c) {}

//void ClassAnalyzer::visit(Create* c)
//{
    //checkSig(c);
    //checkStmnts(c);
//}

//void ClassAnalyzer::visit(Reader* r)
//{
    //checkSig(r);
    //checkStmnts(r);
//}

//void ClassAnalyzer::visit(Writer* w)
//{
    //checkSig(w);
    //checkStmnts(w);
//}

//void ClassAnalyzer::visit(Routine* r)
//{
    //checkSig(r);
    //checkStmnts(r);
//}

void ClassAnalyzer::visit(MemberFct* m)
{
    checkSig(m);
    checkStmnts(m);
}

void ClassAnalyzer::visit(MemberVar* m)
{
}

namespace 
{
    static bool lessInOut(const InOut* io1, const InOut* io2)
    {
        return *io1->id() < *io2->id();
    }
}

void ClassAnalyzer::checkSig(MemberFct* m)
{
    Params& in = m->sig_.in_;
    RetVals& out = m->sig_.out_;
    
    /*
     * are there duplicates?
     */
    std::vector<InOut*> ios( in.size() + out.size() );
    std::copy( out.begin(), out.end(), 
            std::copy(in.begin(), in.end(), ios.begin()) );
    std::sort( ios.begin(), ios.end(), &lessInOut );

    if ( ios.empty() )
        return; // nothing to do

    // check all ios
    InOut* pre = ios[0];
    ctxt_->result_ &= pre->validate(ctxt_->module_);
    m->scope_->insert(pre);

    for (size_t i = 1; i < ios.size(); ++i)
    {
        InOut* io = ios[i];

        ctxt_->result_ &= io->validate(ctxt_->module_);

        if ( *pre->id() == *io->id() )
        {
            errorf( io->loc(), 
                    "there is already a %s named '%s' defined in this signature",
                    pre->kind(),
                    io->cid() );
            SWIFT_PREV_ERROR( pre->loc() );
            ctxt_->result_ = false;
        }
        else
            m->scope_->insert(io);

        pre = io;
    }
}

void ClassAnalyzer::checkStmnts(MemberFct* m)
{
    StmntAnalyzer sa(ctxt_);
    m->scope_->accept(&sa);
}

} // namespace swift
