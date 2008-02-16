#include "be/spiller.h"

#include "me/functab.h"

namespace be {

/*
    constructor and destructor
*/

Spiller::Spiller()
    : function_(0)
    , cfg_(0)
{}

/*
    further methods
*/

void Spiller::setFunction(me::Function* function)
{
    function_ = function;
    cfg_ = &function_->cfg_;
}

} // namespace be
