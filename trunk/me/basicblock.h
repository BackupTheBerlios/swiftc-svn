#ifndef SWIFT_BASIC_BLOCK_H
#define SWIFT_BASIC_BLOCK_H

#include <fstream>
#include <string>
#include <set>

#include "utils/list.h"

#include "me/ssa.h"

struct BasicBlock;

typedef List<BasicBlock*> BBList;
typedef std::set<BasicBlock*> BBSet;

struct BasicBlock
{
    InstrList::Node* begin_;
    InstrList::Node* end_;

    size_t index_;

    BBSet pred_; ///< predecessors of the control flow graph
    BBSet succ_; ///< successors of the control flow graph
    BBSet domFrontier_;

    /// keeps acount of the regs with var numbers which are assigned to in this basic block last
    RegMap varNr_;

    BasicBlock(InstrList::Node* begin, InstrList::Node* end)
        : begin_(begin)
        , end_(end)
        , index_(std::numeric_limits<size_t>::max()) // means: not reached yet
    {}

    void connectBB(BasicBlock* bb)
    {
        this->succ_.insert(bb);
        bb->pred_.insert(this);
    }
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
    bool isReached() const
    {
        return index_ != std::numeric_limits<size_t>::max();
    }

    /// returns the title string of this BasicBlock
    std::string toString() const;
};

#endif // SWIFT_BASIC_BLOCK_H
