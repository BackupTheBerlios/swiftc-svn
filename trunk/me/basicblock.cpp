#include "basicblock.h"

#include <sstream>


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
