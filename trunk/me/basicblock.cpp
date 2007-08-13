#include "basicblock.h"

#include <sstream>

#include "utils/assert.h"


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

size_t BasicBlock::whichPred(BasicBlock* bb) const
{
    size_t pos = 0;

    for (BBSet::const_iterator iter = pred_.begin(); iter != pred_.end(); ++iter)
    {
        if (*iter == bb)
            break;
        ++pos;
    }

    swiftAssert(pos < pred_.size(), "bb not found");

    return pos;
}

