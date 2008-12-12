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
    DefUse(Reg* reg, InstrNode* instrNode, BBNode* bbNode)
        : reg_(reg)
        , instrNode_(instrNode)
        , bbNode_(bbNode)
    {}

    void set(Reg* reg, InstrNode* instrNode, BBNode* bbNode)
    {
        reg_        = reg;
        instrNode_  = instrNode;
        bbNode_     = bbNode;
    }

    Reg* reg_;              /// < The register which is referenced here.
    InstrNode* instrNode_;  ///< The instruction which is referenced here.
    BBNode* bbNode_;        ///< The basic block where \a instr_ can be found.
};

typedef List<DefUse> DefUseList;

#define DEFUSELIST_EACH(iter, defUseList) \
    for (me::DefUseList::Node* (iter) = (defUseList).first(); (iter) != (defUseList).sentinel(); (iter) = (iter)->next())

} // namespace me

#endif // ME_DEFUSE_H
