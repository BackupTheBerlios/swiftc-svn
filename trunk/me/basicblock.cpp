#include "basicblock.h"

#include <sstream>

std::string BasicBlock::toString() const
{
    swiftAssert( begin_ || end_, "begin_ and end_ are not allowed to be zero simultanously");

    std::ostringstream oss;

    if (!begin_)
        oss << "ENTRY";
    else if (!end_)
        oss << "EXIT";
    else
        oss << begin_->value_->toString();

    return oss.str();
}
