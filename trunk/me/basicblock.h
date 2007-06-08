#ifndef SWIFT_BASIC_BLOCK_H
#define SWIFT_BASIC_BLOCK_H

#include <fstream>
#include <string>

#include "utils/list.h"

#include "me/ssa.h"

struct BasicBlock;
typedef List<BasicBlock*> BBList;

struct BasicBlock
{
    BBList pred_;
    BBList succ_;

    InstrList::Node* begin_;
    InstrList::Node* end_;

    BasicBlock(InstrList::Node* begin, InstrList::Node* end)
        : begin_(begin)
        , end_(end)
    {}

    std::string toString() const;
    void toDot(std::ofstream& ofs) const;
};

#endif // SWIFT_BASIC_BLOCK_H
