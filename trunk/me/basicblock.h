#ifndef ME_BASIC_BLOCK_H
#define ME_BASIC_BLOCK_H

#include <fstream>
#include <string>
#include <set>

#include "utils/graph.h"
#include "utils/list.h"

#include "me/ssa.h"

namespace me {

#define BBLIST_EACH(iter, bbList) \
    for (me::BBList::Node* (iter) = (bbList).first(); (iter) != (bbList).sentinel(); (iter) = (iter)->next())

/** 
 * @brief This represents a basic block in the control flow graph (CFG).
 *
 * The first Instruction is always a LabelInstr. \a begin_ points to it. <br>
 *
 * Then there are zero or more PhiInstr instances. \a firstPhi_ points to the 
 * first PhiInstr. \a firstOrdinary_ points to the first Instruction which is 
 * \em NOT a PhiInstr. If there are zero PhiInstr instances, \a firstPhi_ = 
 * \a firstOrdinary_. <br>
 *
 * Then there are zero or more regular Instructions. This means \em NO LabelInstr
 * instances and \em NO PhiInstr instances. Furthermore if there is a GotoInstr
 * or a BranchInstr this will be the last instruction in this BasicBlock. <br>
 * 
 * \a end_ points to the LabelInstr of the next BasicBlock, which is the first
 * instruction which does not belong to this BasicBlock. If this LabelInstr
 * does not exist it points to the sentinel of the Function's instrList_.<br>
 * <br>
 *
 * So here are some examples how to iterate over groups of instructions: <br>
 *
 * All instructions: <br>
 * for (iter = bb->begin_; iter != bb->end_; ++iter) <br><br>
 * 
 * All instructions without the leading LabelInstr: <br>
 * for (iter = bb->firstPhi_; iter != bb->end_; ++iter) <br><br>
 * or: 
 *
 * All PhiInstr instances, if any: <br>
 * for (iter = bb->firstPhi_; iter != bb->firstOrdinary_; ++iter) <br><br>
 *
 * All ordinary instructions, if any: <br>
 * for (iter = bb->firstOrdinary_; iter != bb->end_; ++iter) <br><br>
 */
struct BasicBlock
{
    /// Points the leading LabelInstr of this BasicBlock.
    InstrNode* begin_;

    /** 
     * Points to the first PhiInstr. 
     * If this does not exist it points to \a firstOrdinary_.
     */
    union 
    {
        InstrNode* first_;
        InstrNode* firstPhi_;
    };

    /**
     * Points to the first ordinary instruction.
     * If this does not exist it points to \a end_.
     */
    InstrNode* firstOrdinary_;

    /**
     * Points to the leading LabelInstr of the next BasicBlock or
     * the ending LabelInstr respectively.
     * If this does not exist it points to the sentinel of this functions
     * instrList_.
     */
    InstrNode* end_;

    size_t index_;

    BBList domFrontier_;
    BBList domChildren_;

    /// Keeps acount of the vars which are assigned to in this BasicBlock last.
    RegMap vars_;
    /// Regs that live while
    RegSet liveIn_;
    RegSet liveOut_;

/*
    constructors
*/
    BasicBlock() {}
    BasicBlock(InstrNode* begin, InstrNode* end, InstrNode* firstOrdinary);

/*
    further methods
*/
    /// Returns the title string of this BasicBlock.
    std::string name() const;

    /// Returns a string holding the instructions of the BasicBlock for dot-files.
    std::string toString() const;
};

} // namespace me

#endif // ME_BASIC_BLOCK_H
