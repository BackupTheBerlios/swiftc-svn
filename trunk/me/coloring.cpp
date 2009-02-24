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

#include "me/coloring.h"

#include <algorithm>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"
#include "me/op.h"

namespace me {

typedef Colors::ResultVec IntVec;

/*
 * constructors
 */

Coloring::Coloring(Function* function, size_t stackPlace)
    : CodePass(function)
    , typeMask_(-1)         // not used in this case
    , reservoir_(Colors())  // use an empty set
    , stackPlace_(stackPlace)
{}

Coloring::Coloring(Function* function, int typeMask,const Colors& reservoir)
    : CodePass(function)
    , typeMask_(typeMask)
    , reservoir_(reservoir)
{}

/*
 * methods
 */

void Coloring::process()
{
    /*
     * start with the first true basic block
     * and perform a pre-order walk of the dominator tree
     */

    if ( reservoir_.empty() )
        colorRecursiveMem(cfg_->entry_);
    else
        colorRecursive(cfg_->entry_);
}

/*
 * memory location coloring
 */

int Coloring::getFreeMemColor(Colors& colors)
{
    int color = 0;

    for (Colors::iterator iter = colors.begin(); iter != colors.end(); ++iter)
    {
        if ( color != *iter )
            break;

        ++color;
    }

    colors.insert(color);

    // update the spill slots used
    function_->stackLayout_.insertColor(stackPlace_, color);

    return color;
}

void Coloring::colorRecursiveMem(BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;
    Colors colors; // colors already used go here

    // all vars in liveIn_ have already been colored
    VARSET_EACH(iter, bb->liveIn_)
    {
        Reg* reg = (*iter)->isSpilled();
        if (!reg)
            continue;

        int color = reg->color_;
        swiftAssert(color >= 0, "color must be assigned here");

        // mark as occupied
        colors.insert(color);
    }

    // for each instruction 
    for (InstrNode* iter = bb->firstPhi_; iter != bb->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

#ifdef SWIFT_DEBUG
        /*
         * In the debug version this set knows vars which were already
         * removed. This allows more precise assertions (see below).
         */
        VarSet erased;
#endif // SWIFT_DEBUG

        // for each var on the right hand side
        for (size_t i = 0; i < instr->arg_.size(); ++i)
        {
            Reg* reg = instr->arg_[i].op_->isSpilled();
            if (!reg)
                continue;

            if ( InstrBase::isLastUse(iter, reg) )
            {
                // -> its the last use of reg
                Colors::iterator colorIter = colors.find(reg->color_);
#ifdef SWIFT_DEBUG
                // has this reg already been erased due to a double entry like a = b + b?
                if ( erased.contains(reg) )
                    continue;

                swiftAssert( colorIter != colors.end(), "color must be found here" );
                colors.erase(colorIter); // last use of reg
                erased.insert(reg);
#else // SWIFT_DEBUG
                if ( colorIter != colors.end() )
                    colors.erase(colorIter); // last use of reg
                /*
                 * else -> the reg must already been removed which must
                 *      be caused by a double entry like a = b + b
                 */
#endif // SWIFT_DEBUG
            } // if last use
        } // for each arg var

        // for each var on the left hand side -> assign a color for result
        for (size_t i = 0; i < instr->res_.size(); ++i)
        {
            Reg* reg = instr->res_[i].var_->isSpilled();
            if (!reg)
                continue;

            reg->color_ = getFreeMemColor(colors);

            // pointless definitions should be optimized away
            if ( !instr->liveOut_.contains(reg) )
                colors.erase( colors.find(reg->color_) );
        }
    } // for each instruction

    // for each child of bb in the dominator tree
    BBLIST_EACH(bbIter, bb->domChildren_)
    {
        BBNode* domChild = bbIter->value_;
        colorRecursiveMem(domChild);
    }
}

/*
 * register coloring
 */

void Coloring::colorRecursive(BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;
    Colors colors; // colors already used go here
    InstrNode* start;

    if ( bb->hasConstrainedInstr() )
    {
        InstrNode* constrainedInstrNode = bb->firstOrdinary_;
        InstrBase* constrainedInstr = constrainedInstrNode->value_;
        
        swiftAssert( constrainedInstr->isConstrained(), "must be constrained");
        swiftAssert( bb->liveIn_.empty(), 
                "liveIn should be empty with a constrained instruction" );

        VarSet alreadyColored;
        colorConstraintedInstr(constrainedInstrNode, alreadyColored);

        /*
         * colors which must not be used above in the phi functions:
         * res(constrainedInstr) or arg(constrainedInstr)
         */
        Colors occupied; 

        for (size_t i = 0; i < constrainedInstr->arg_.size(); ++i)
        {
            Reg* reg = constrainedInstr->arg_[i].op_->colorReg(typeMask_);
            if (!reg)
                continue;

            occupied.insert(reg->color_);

            if ( constrainedInstr->livesThrough(reg) )
                colors.insert(reg->color_); // this one must not be used afterwards
        }

        for (size_t i = 0; i < constrainedInstr->res_.size(); ++i)
        {
            Reg* reg = constrainedInstr->res_[i].var_->colorReg(typeMask_);
            if (!reg)
                continue;

            occupied.insert(reg->color_);

            if ( constrainedInstr->liveOut_.contains(reg) )
                colors.insert(reg->color_); // this one must not be used afterwards
        }

        // for each PhiInstr
        for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
        {
            swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), "must be a PhiInstr" );
            PhiInstr* phi = (PhiInstr*) iter->value_;
            Reg* phiRes = phi->result()->colorReg(typeMask_);
            if (!phiRes)
                continue;

            if ( !alreadyColored.contains(phiRes) )
            {
                // -> assign a fresh color here
                IntVec freeColors = reservoir_.difference(occupied);
                swiftAssert( !freeColors.empty(), "must not be empty" );
                phiRes->color_ = freeColors[0];

                // pointless definitions should be optimized away
                if ( phi->liveOut_.contains(phiRes) )
                {
                    occupied.insert(phiRes->color_);

                    /*
                     * insert it only in colors 
                     * if it is in the liveOut of constrainedInstr
                     */
                    if ( constrainedInstr->liveOut_.contains(phiRes) )
                        colors.insert(phiRes->color_);
                }
            }
        }

        // start with the first instr which is followed by the constrained one
        start = constrainedInstrNode->next();
    }
    else
    {
        // all vars in liveIn_ have already been colored
        VARSET_EACH(iter, bb->liveIn_)
        {
            Reg* reg = (*iter)->colorReg(typeMask_);
            if (!reg)
                continue;

            int color = reg->color_;
            swiftAssert(color >= 0, "color must be assigned here");

            // mark as occupied
            colors.insert(color);
        }

        // start with first instruction
        start = bb->firstPhi_;
    }

    // for each instruction 
    for (InstrNode* iter = start; iter != bb->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        std::vector<int> freeHere;

#ifdef SWIFT_DEBUG
        /*
         * In the debug version this set knows vars which were already
         * removed. This allows more precise assertions (see below).
         */
        VarSet erased;
#endif // SWIFT_DEBUG

        // for each var on the right hand side
        for (size_t i = 0; i < instr->arg_.size(); ++i)
        {
            Reg* reg = instr->arg_[i].op_->colorReg(typeMask_);
            if (!reg)
                continue;

            if ( InstrBase::isLastUse(iter, reg) )
            {
                // -> its the last use of reg
                Colors::iterator colorIter = colors.find(reg->color_);
#ifdef SWIFT_DEBUG
                // has this reg already been erased due to a double entry like a = b + b?
                if ( erased.contains(reg) )
                    continue;

                swiftAssert( colorIter != colors.end(), "color must be found here" );
                freeHere.push_back(*colorIter);
                colors.erase(colorIter); // last use of reg
                erased.insert(reg);
#else // SWIFT_DEBUG
                if ( colorIter != colors.end() )
                {
                    freeHere.push_back(*colorIter);
                    colors.erase(colorIter); // last use of reg
                }
                /*
                 * else -> the reg must already been removed which must
                 *      be caused by a double entry like a = b + b
                 */
#endif // SWIFT_DEBUG
            } // if last use
        } // for each arg var

        // for each var on the left hand side -> assign a color for result
        for (size_t i = 0; i < instr->res_.size(); ++i)
        {
            Reg* reg = instr->res_[i].var_->colorReg(typeMask_);
            if (!reg)
                continue;

            // try to use a color which has just been freed here
            if ( !freeHere.empty() )
            {
                colors.insert(freeHere[0]);
                reg->color_ = freeHere[0];
            }
            else
            {
                IntVec freeColors = reservoir_.difference(colors);
                swiftAssert( !freeColors.empty(), "must not be empty" );

                colors.insert(freeColors[0]);
                reg->color_ = freeColors[0];
            }

            // pointless definitions should be optimized away
            if ( !instr->liveOut_.contains(reg) )
                colors.erase( colors.find(reg->color_) );
        }
    } // for each instruction

    // for each child of bb in the dominator tree
    BBLIST_EACH(bbIter, bb->domChildren_)
    {
        BBNode* domChild = bbIter->value_;
        colorRecursive(domChild);
    }
}

void Coloring::colorConstraintedInstr(InstrNode* instrNode, VarSet& alreadyColored)
{
    InstrBase* instr = instrNode->value_;
    
    RegSet liveThrough;
    RegSet dyingArgs;

    // init liveThrough and dyingArgs
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        Reg* reg = instr->arg_[i].op_->colorReg(typeMask_);
        if (!reg)
            continue;

        alreadyColored.insert(reg);

        if ( instr->livesThrough(reg) )
            liveThrough.insert(reg);
        else
            dyingArgs.insert(reg);
    }
    // -> liveThrough = {r | r in arg(instr) and r lives through instr}
    // -> dyingArgs = arg(instr) \ liveThrough

    // unconstrainedDefs = res(instr)
    RegSet unconstrainedDefs;
    for (size_t i = 0; i < instr->res_.size(); ++i)
    {
        Reg* reg = instr->res_[i].var_->colorReg(typeMask_);
        if (!reg)
            continue;

        unconstrainedDefs.insert(reg);
    }

    Colors colorsD, colorsA;
    Colors freeColors(reservoir_);

    // for each constrained arg
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        Reg* reg = (Reg*) instr->arg_[i].op_->colorReg(typeMask_);
        if (!reg)
            continue;

        int constraint = instr->arg_[i].constraint_;
        if (constraint == NO_CONSTRAINT)
            continue;

        // mark register as occupied
        colorsA.insert(constraint); 

        // set color
        reg->color_ = constraint;

        if ( liveThrough.contains(reg) )
        {
            swiftAssert( freeColors.contains(constraint), "must be found here" );
            freeColors.erase(constraint);
        }

        // mark arg as assigned
        dyingArgs.erase(reg);
        liveThrough.erase(reg);
    }

    // for each constrained result
    for (size_t i = 0; i < instr->res_.size(); ++i)
    {
        Reg* reg = instr->res_[i].var_->colorReg(typeMask_);
        if (!reg)
            continue;

        int constraint = instr->res_[i].constraint_;
        if (constraint == NO_CONSTRAINT)
            continue;

        // mark register as occupied
        colorsD.insert(constraint); 

        // mark reg as assigned
        swiftAssert( unconstrainedDefs.contains(reg), "must be found here" );
        unconstrainedDefs.erase(reg);

        // set color
        reg->color_ = constraint;
    }

    // for each unconstrained arg which does not live through
    REGSET_EACH(iter, dyingArgs)
    {
        Reg* reg = *iter;

        // try to use a color of the results
        IntVec c_d_without_c_a = colorsD.difference(colorsA);

        if ( !c_d_without_c_a.empty() )
            reg->color_ = c_d_without_c_a[0];
        else
        {
            // -> there is none -> use a fresh color
            IntVec free_without_c_a = freeColors.difference(colorsA);
            swiftAssert( !free_without_c_a.empty(), "must not be empty" );
            reg->color_ = free_without_c_a[0];
        }
    }

    // for each unconstrained result
    REGSET_EACH(iter, unconstrainedDefs)
    {
        Reg* reg = *iter;

        IntVec c_a_without_c_d = colorsA.difference(colorsD);

        if ( !c_a_without_c_d.empty() )
            reg->color_ = c_a_without_c_d[0];
        else
        {
            // -> there is none -> use a fresh color
            IntVec free_without_c_d = freeColors.difference(colorsA);
            swiftAssert( !free_without_c_d.empty(), "must not be empty" );
            reg->color_ = free_without_c_d[0];
        }
    }

    /*
     * assign registers to liveThrough from the set 
     * freeColors \ (colorsD U colorsA)
     */

    REGSET_EACH(iter, liveThrough)
    {
        Reg* reg = *iter;

        IntVec tmp = colorsD.unite(colorsA);
        Colors c_d_union_c_a;
        for (size_t i = 0; i < tmp.size(); ++i)
            c_d_union_c_a.insert( tmp[i] );

        IntVec free = freeColors.difference(c_d_union_c_a);
        swiftAssert( !free.empty(), "must not be empty" );
        reg->color_ = free[0];
    }
}

} // namespace me
