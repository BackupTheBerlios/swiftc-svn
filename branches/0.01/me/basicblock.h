/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef ME_BASIC_BLOCK_H
#define ME_BASIC_BLOCK_H

#include <fstream>
#include <string>
#include <set>

#include "utils/graph.h"
#include "utils/list.h"

#include "me/ssa.h"

namespace me {

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
 * Then there are zero or more regular instructions. This means \em NO
 * LabelInstr instances and \em NO PhiInstr instances. Furthermore if there is a
 * GotoInstr or a BranchInstr this will be the last instruction in this
 * BasicBlock. Thus the last instruction in a basic block is either a regular
 * Instruction or a JumpInstr<br>
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
 * All PhiInstr instances if any. Note that the loop isn't executed at all when
 * there is no PhiInstr at all: <br>
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
     *
     * If this does not exist it points to \a firstOrdinary_.
     */
    InstrNode* firstPhi_;

    /**
     * Points to the first ordinary instruction.
     *
     * If this does not exist it points to \a end_.
     */
    InstrNode* firstOrdinary_;

    /**
     * Points to the leading LabelInstr of the next BasicBlock or
     * the ending LabelInstr respectively.
     *
     * If this does not exist it points to the sentinel of this functions
     * instrList_.
     */
    InstrNode* end_;

    BBList domFrontier_;
    BBList domChildren_;

    /// Keep account of all vars which are not in SSA form and defined in this basic block.
    VarMap vars_; // TODO kill this
    /// Vars that live.
    VarSet liveIn_;
    VarSet liveOut_;

    /*
     * constructors
     */

    BasicBlock() {}
    BasicBlock(InstrNode* begin, InstrNode* end, InstrNode* firstOrdinary = 0);

    /*
     * further methods
     */

    InstrNode* getLastNonJump();
    InstrNode* getSpillLocation();
    InstrNode* getReloadLocation();
    InstrNode* getBackSpillLocation();
    InstrNode* getBackReloadLocation();

    void fixPointers();

    bool hasPhiInstr() const;

    bool hasConstrainedInstr() const;

    bool hasDomChild(const BBNode* bbNode) const;

    /// Returns the title string of this BasicBlock.
    std::string name() const;

    /// Returns a string holding the instructions of the BasicBlock for dot-files.
    std::string toString() const;

    /// Returns a string holding the liveness infos for this BasicBlock.
    std::string livenessString() const;
};

//------------------------------------------------------------------------------

#define BBLIST_EACH(iter, bbList) \
    for (me::BBList::Node* (iter) = (bbList).first(); (iter) != (bbList).sentinel(); (iter) = (iter)->next())

#define BBLIST_CONST_EACH(iter, bbList) \
    for (const me::BBList::Node* (iter) = (bbList).first(); (iter) != (bbList).sentinel(); (iter) = (iter)->next())

#define BBSET_EACH(iter, bbSet) \
    for (me::BBSet::iterator (iter) = (bbSet).begin(); (iter) != (bbSet).end(); ++(iter))

} // namespace me

#endif // ME_BASIC_BLOCK_H
