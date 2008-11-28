#include "basicblock.h"

#include <iostream>
#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

#include "me/op.h"

namespace me {

/*
 * constructors
 */

BasicBlock::BasicBlock(InstrNode* begin, InstrNode* end, InstrNode* firstOrdinary /*= 0*/)
    : begin_(begin)
    , firstOrdinary_( firstOrdinary ? firstOrdinary : end) // select end if firstOrdinary == 0
    , end_(end)
{
    firstPhi_ = firstOrdinary_;// assume that there are no phis at the beginning
}

/*
 * further methods
 */

InstrNode* BasicBlock::getLastNonJump()
{
    InstrNode* result = end_->prev();

    if ( dynamic_cast<JumpInstr*>(result->value_) )
        result = result->prev(); // ... use last instruction before the JumpInstr

    return result;
}

void BasicBlock::fixPointers()
{
    // TODO this can be done smarter
    bool phi = false;
    bool ordinary = false;
    for (InstrNode* iter = begin_->next(); iter != end_; iter = iter->next())
    {
        if ( phi == false && typeid(*iter->value_) == typeid(PhiInstr) )
        {
            firstPhi_ = iter;
            phi = true;
        }
        if (ordinary == false && typeid(*iter->value_) != typeid(PhiInstr) )
        {
            firstOrdinary_ = iter;
            ordinary = true;
        }
    }
    if (ordinary == false)
        firstOrdinary_ = end_;
    if (phi == false)
        firstPhi_ = firstOrdinary_;
}

bool BasicBlock::hasPhiInstr() const
{
    return firstPhi_ != firstOrdinary_;
}

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

std::string BasicBlock::livenessString() const
{
    std::ostringstream oss;

    // print live in infos
    oss << "\tlive IN:" << std::endl;

    REGSET_EACH(iter, liveIn_)
        oss << "\t\t" << (*iter)->toString() << std::endl;

    // print live out infos
    oss << "\tlive OUT:" << std::endl;

    REGSET_EACH(iter, liveOut_)
        oss << "\t\t" << (*iter)->toString() << std::endl;

    return oss.str();
}

} // namespace me
