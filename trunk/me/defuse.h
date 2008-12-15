#ifndef ME_DEFUSE_H
#define ME_DEFUSE_H

#include <map>

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

//-------------------------------------------------------------------------------

typedef List<DefUse> DefUseList;

#define DEFUSELIST_EACH(iter, defUseList) \
    for (me::DefUseList::Node* (iter) = (defUseList).first(); (iter) != (defUseList).sentinel(); (iter) = (iter)->next())

//-------------------------------------------------------------------------------

struct RegDefUse
{
    DefUseList defs_;
    DefUseList uses_;
};

//------------------------------------------------------------------------------

/// Keeps registers with their definitions und uses.
typedef std::map<Reg*, RegDefUse*> RDUMap;

#define RDUMAP_EACH(iter, rdus) \
    for (RDUMap::iterator (iter) = (rdus).begin(); (iter) != (rdus).end(); ++(iter))

//------------------------------------------------------------------------------

} // namespace me

#endif // ME_DEFUSE_H
