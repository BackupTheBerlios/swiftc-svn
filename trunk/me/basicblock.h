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

    int postOrderNr_;
    bool reached_;

    BBSet pred_; ///< predecessors of the control flow graph
    BBSet succ_; ///< successors of the control flow graph

    /// keeps acount of the regs with var numbers which are assigned to in this basic block last
    RegMap varNr_;

    BasicBlock(InstrList::Node* begin, InstrList::Node* end)
        : begin_(begin)
        , end_(end)
        , postOrderNr_(-1)
        , reached_(false)
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

    /// returns the title string of this BasicBlock
    std::string toString() const;
};

#endif // SWIFT_BASIC_BLOCK_H
