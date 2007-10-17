#ifndef SWIFT_DEFUSE_H
#define SWIFT_DEFUSE_H

#include "utils/list.h"

// forward declarations
struct InstrBase;
struct BBNode;


/// This class can be used to reference def and use information
struct DefUse
{
    DefUse() {}
    DefUse(InstrBase* instr, BBNode* bbNode)
        : instr_(instr)
        , bbNode_(bbNode)
    {}

    void set(InstrBase* instr, BBNode* bbNode)
    {
        instr_ = instr;
        bbNode_ = bbNode;
    }

    InstrBase* instr_;  ///< The instruction which is referenced here
    BBNode*    bbNode_; ///< The basic block where \a instr_ can be found
};

typedef List<DefUse> UseList;

#define USELIST_EACH(iter, useList) \
    for (DefUse::Node* (iter) = (useList).first(); (iter) != (useList).sentinel(); (iter) = (iter)->next())


#endif // SWIFT_DEFUSE_H
