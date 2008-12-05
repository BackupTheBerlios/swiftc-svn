#ifndef ME_DEFUSE_H
#define ME_DEFUSE_H

#include "utils/list.h"
#include "utils/graph.h"

#include "me/basicblock.h"
#include "me/forward.h"

namespace me {

/// This class can be used to reference def and use information
struct DefUse
{
    DefUse() {}
    DefUse(InstrNode* instrNode, BBNode* bbNode)
        : instrNode_(instrNode)
        , bbNode_(bbNode)
    {}

    void set(InstrNode* instrNode, BBNode* bbNode)
    {
        instrNode_ = instrNode;
        bbNode_ = bbNode;
    }

    InstrNode* instrNode_;  ///< The instruction which is referenced here
    BBNode* bbNode_; ///< The basic block where \a instr_ can be found
};

typedef List<DefUse> UseList;

#define USELIST_EACH(iter, useList) \
    for (me::UseList::Node* (iter) = (useList).first(); (iter) != (useList).sentinel(); (iter) = (iter)->next())

} // namespace me

#endif // ME_DEFUSE_H
