#include "fe/fctvectorizer.h"

#include <llvm/Function.h>

#include "fe/class.h"
#include "fe/context.h"
#include "fe/node.h"
#include "fe/type.h"

#include "Packetizer/api.h"

namespace swift {

FctVectorizer::FctVectorizer(Context* ctxt)
    : ctxt_(ctxt)
    , packetizer_( Packetizer::getPacketizer(true) )
{
    typedef Module::ClassMap::const_iterator CIter;
    const Module::ClassMap& classes = ctxt_->module_->classes();

    // for each class
    for (CIter iter = classes.begin(); iter != classes.end(); ++iter)
    {
        Class* c = iter->second;

        // skip builtin types
        if ( ScalarType::isScalar(c->id()) )
            continue;

        // for each member fct
        for (size_t i = 0; i < c->memberFcts().size(); ++i)
        {
            MemberFct* m = c->memberFcts()[i];

            if ( m->isSimd() )
                process(c, m);
        }
    }

    Packetizer::runPacketizer( packetizer_, ctxt_->lmodule() );
}

void FctVectorizer::process(Class* c, MemberFct* m)
{
    Packetizer::addFunctionToPacketizer(packetizer_, m->llvmFct_->getNameStr(), m->simdFct_->getNameStr(), 4);
}

} // namespace swift
