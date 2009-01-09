#include "me/coloring.h"

#include <algorithm>
#include <iostream>
#include <typeinfo>

#include "me/cfg.h"
#include "me/op.h"

namespace me {

typedef Colors::ResultVec IntVec;

/*
 * constructor
 */

Coloring::Coloring(Function* function, int typeMask, const Colors& reservoir)
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
    colorRecursive(cfg_->entry_);
}

void Coloring::colorRecursive(BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;
    Colors colors;

    InstrNode* start;

    if ( bb->hasConstrainedInstr() )
    {
        InstrNode* constrainedInstr = bb->firstOrdinary_;
        
        swiftAssert( bb->liveIn_.empty(), 
                "liveIn should be empty with a constrained instruction" );

        RegSet alreadyColored;

        colorConstraintedInstr(constrainedInstr, colors, alreadyColored);

        /*
         * color definitions of preceding phi instructions not in 
         * res(constrainedInstr) or arg(constrainedInstr)
         */

        // for each PhiInstr
        for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
        {
            swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), "must be a PhiInstr" );
            PhiInstr* phi = (PhiInstr*) iter->value_;
            Reg* phiRes = phi->result();

            if ( !phiRes->colorReg(typeMask_) )
                continue;

            if ( !alreadyColored.contains(phiRes) )
            {
                // -> assign a fresh color here

                IntVec freeColors = reservoir_.difference(colors);
                swiftAssert( !freeColors.empty(), "must not be empty" );

                colors.insert(freeColors[0]);
                phiRes->color_ = freeColors[0];

                // pointless definitions should be optimized away
                if ( !phi->liveOut_.contains(phiRes) )
                    colors.erase( colors.find(phiRes->color_) );
            }
        }

        // start with the first instr which is followed by the constrained one
        start = constrainedInstr->next();
    }
    else
    {
        // all vars in liveIn_ have already been colored
        REGSET_EACH(iter, bb->liveIn_)
        {
            Reg* reg = *iter;

            // do not color memory locations or regs with wrong types
            if ( !reg->colorReg(typeMask_) )
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
        RegSet erased;
#endif // SWIFT_DEBUG

        // for each var on the right hand side
        for (size_t i = 0; i < instr->arg_.size(); ++i)
        {
            Reg* reg = dynamic_cast<Reg*>(instr->arg_[i].op_);

            // only color regs of proper type, which are not memory locations
            if ( !reg || !reg->colorReg(typeMask_) )
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
            Reg* reg = instr->res_[i].reg_;

            // only color regs and regs of proper type
            if ( !reg->colorReg(typeMask_) )
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

void Coloring::colorConstraintedInstr(InstrNode* instrNode, Colors& colors, RegSet& alreadyColored)
{
    InstrBase* instr = instrNode->value_;
    
    RegSet liveThrough;
    RegSet dyingArgs;

    // init liveThrough and dyingArgs
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        if ( typeid(*instr->arg_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->arg_[i].op_;

        // do not color memory locations or regs with wrong types
        if ( !reg->colorReg(typeMask_) )
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
        Reg* reg = instr->res_[i].reg_;

        // do not color memory locations or regs with wrong types
        if ( !reg->colorReg(typeMask_) )
            continue;

        alreadyColored.insert(reg);
        unconstrainedDefs.insert(reg);
    }

    Colors colorsD, colorsA;
    Colors freeColors(reservoir_);

    // for each constrained arg
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        if ( typeid(*instr->arg_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->arg_[i].op_;

        // do not color memory locations or regs with wrong types
        if ( !reg->colorReg(typeMask_) )
            continue;

        int constraint = instr->arg_[i].constraint_;
        if (constraint == InstrBase::NO_CONSTRAINT)
            continue;

        // mark register as occupied
        colorsA.insert(constraint); 

        // mark arg as assigned
        dyingArgs.erase(reg);
        liveThrough.erase(reg);

        // set color
        reg->color_ = constraint;

        if ( liveThrough.contains(reg) )
        {
            swiftAssert( freeColors.contains(constraint), "must be found here" );
            freeColors.erase(constraint);

            swiftAssert( !colors.contains(constraint), "must not be found here" );
            colors.insert(constraint);
        }
    }

    // for each constrained result
    for (size_t i = 0; i < instr->res_.size(); ++i)
    {
        Reg* reg = (Reg*) instr->res_[i].reg_;

        // do not color memory locations or regs with wrong types
        if ( !reg->colorReg(typeMask_) )
            continue;

        int constraint = instr->res_[i].constraint_;
        if (constraint == InstrBase::NO_CONSTRAINT)
            continue;

        // mark register as occupied
        colorsD.insert(constraint); 

        // mark reg as assigned
        swiftAssert( unconstrainedDefs.contains(reg), "must be found here" );
        unconstrainedDefs.erase(reg);

        // set color
        reg->color_ = constraint;

        swiftAssert( !colors.contains(constraint), "must not be found here" );
        colors.insert(constraint);

        // pointless definitions should be optimized away
        if ( !instr->liveOut_.contains(reg) )
            colors.erase( colors.find(reg->color_) );
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
            int constraint = free_without_c_d[0];
            reg->color_ = constraint;

            swiftAssert( !colors.contains(constraint), "must not be found here" );
            colors.insert(constraint);

            // pointless definitions should be optimized away
            if ( !instr->liveOut_.contains(reg) )
                colors.erase( colors.find(reg->color_) );
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

        swiftAssert( !colors.contains(reg->color_), "color must be free" );
        colors.insert(reg->color_);
    }
}

} // namespace me
