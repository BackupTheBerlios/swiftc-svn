#include "be/spiller.h"

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

void setFunction(me::Function* function)
{
    function_ = function;
    cfg_ = function_->cfg_;
}
