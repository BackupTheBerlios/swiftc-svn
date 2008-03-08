#ifndef ME_BASIC_BLOCK_H
#define ME_BASIC_BLOCK_H

#include <fstream>
#include <string>
#include <set>

#include "utils/graph.h"
#include "utils/list.h"

#include "me/ssa.h"

namespace me {

#define BBLIST_EACH(iter, bbList) \
    for (me::BBList::Node* (iter) = (bbList).first(); (iter) != (bbList).sentinel(); (iter) = (iter)->next())

struct BasicBlock
{
    InstrNode begin_; ///< Points the leading LabelInstr of this BasicBlock.
    InstrNode end_;   ///< Points to the leading LabelInstr of the next BasicBlock.

    size_t index_;

    BBList domFrontier_;
    BBList domChildren_;

    /// Keeps acount of the vars which are assigned to in this BasicBlock last.
    RegMap vars_;
    /// Regs that live while
    RegSet liveIn_;
    RegSet liveOut_;

    BasicBlock() {}

    BasicBlock(InstrNode begin, InstrNode end)
        : begin_(begin)
        , end_(end)
    {}

    bool isEntry() const
    {
        swiftAssert( begin_ || end_, "begin_ and end_ are not allowed to be zero simultanously");
        return !begin_;
    }

    bool isExit() const
    {
        swiftAssert( begin_ || end_, "begin_ and end_ are not allowed to be zero simultanously");
        return !end_;
    }

    /// Returns the title string of this BasicBlock.
    std::string name() const;

    /// Returns a string holding the instructions of the BasicBlock for dot-files.
    std::string toString() const;
};

} // namespace me

#endif // ME_BASIC_BLOCK_H
