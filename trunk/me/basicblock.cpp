#include "basicblock.h"

#include <sstream>
#include <iostream>

#include "utils/assert.h"

namespace me {

/*
 * constructors
 */

BasicBlock::BasicBlock(InstrNode* begin, InstrNode* end, InstrNode* firstOrdinary)
    : begin_(begin)
    , firstOrdinary_( firstOrdinary ? firstOrdinary : end) // select end if firstOrdinary == 0
    , end_(end)
{
    firstPhi_ = firstOrdinary_;// assume that there are no phis at the beginning
}

/*
 * further methods
 */

std::string BasicBlock::name() const
{
    std::ostringstream oss;
    oss << begin_->value_->toString();

    return oss.str();
}

std::string BasicBlock::toString() const
{
    std::ostringstream oss;

    // print leading label central and with a horizontal line
    oss << '\t' << begin_->value_->toString() << "|\\" << std::endl; // print instruction

    // for all instructions in this basic block except the first and the last LabelInstr
    for (InstrNode* instrIter = begin_->next(); instrIter != end_; instrIter = instrIter->next())
        oss << '\t' << instrIter->value_->toString() << "\\l\\" << std::endl; // print instruction

    return oss.str();
}

} // namespace me
