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
    BBSet pred_;
    BBSet succ_;

    InstrList::Node* begin_;
    InstrList::Node* end_;

    BasicBlock(InstrList::Node* begin, InstrList::Node* end)
        : begin_(begin)
        , end_(end)
    {}

    /// returns the title string of this BasicBlock
    std::string toString() const;

    void connectBB(BasicBlock* bb)
    {
        this->succ_.insert(bb);
        bb->pred_.insert(this);
    }
};

#endif // SWIFT_BASIC_BLOCK_H
