#ifndef SWIFT_BASIC_BLOCK_H
#define SWIFT_BASIC_BLOCK_H

#include <fstream>
#include <string>
#include <set>

#include "utils/graph.h"
#include "utils/list.h"

#include "me/ssa.h"


#define BBLIST_EACH(iter, bbList) \
    for (BBList::Node* (iter) = (bbList).first(); (iter) != (bbList).sentinel(); (iter) = (iter)->next())

struct BasicBlock
{
    InstrList::Node* begin_;
    InstrList::Node* end_;

    size_t index_;

    BBList domFrontier_;
    BBList domChildren_;

    /// keeps acount of the vars which are assigned to in this basic block last
    RegMap vars_;

    BasicBlock() {}

    BasicBlock(InstrList::Node* begin, InstrList::Node* end)
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

    /// returns the title string of this BasicBlock
    std::string name() const;

    /// returns a string holding the instructions of the BasicBlock for dot-files
    std::string toString() const;
};

#endif // SWIFT_BASIC_BLOCK_H
