#include "basicblock.h"

#include <sstream>

bool BasicBlock::reachedValue_ = true;

std::string BasicBlock::toString() const
{
    std::ostringstream oss;

    if ( isEntry() )
        oss << "ENTRY";
    else if ( isExit() )
        oss << "EXIT";
    else
        oss << begin_->value_->toString();

    return oss.str();
}

BBSet BasicBlock::intersect(BasicBlock* b1, BasicBlock* b2)
{
    BasicBlock* finger1 = b1;
    BasicBlock* finger2 = b2;

    while (finger1->postOrderNr_ != finger2->postOrderNr_)
    {
        while (finger1->postOrderNr < finger2->postOrderNr)
            finger1
    }
}
