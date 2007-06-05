#ifndef SWIFT_BASIC_BLOCK_H
#define SWIFT_BASIC_BLOCK_H

#include <string>

#include "utils/list.h"

#include "me/ssa.h"

struct BasicBlock;
typedef List<BasicBlock*> BBList;

struct BasicBlock
{
    BBList prev_;

    LabelInstr* begin_;
    LabelInstr* end_;

    BasicBlock(LabelInstr* begin, LabelInstr* end)
        : begin_(begin)
        , end_(end)
    {}

    std::string toString() const;
};

#endif // SWIFT_BASIC_BLOCK_H