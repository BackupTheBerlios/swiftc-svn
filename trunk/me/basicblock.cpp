#include "basicblock.h"

#include <sstream>

std::string BasicBlock::toString() const
{
    std::ostringstream oss;
    oss << begin_->toString() << " -> " << end_->toString();

    return oss.str();
}
