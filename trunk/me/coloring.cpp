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

int Coloring::findFirstFreeColorAndAllocate(Colors& colors)
{
    // build vector which holds free colors
    IntVec freeColors = reservoir_.difference(colors);
    swiftAssert( !freeColors.empty(), "must not be empty" );

    colors.insert(freeColors[0]);

    return freeColors[0];
}

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

    // for each instruction 
    for (InstrNode* iter = bb->firstPhi_; iter != bb->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

#ifdef SWIFT_DEBUG
        /*
         * In the debug version this set knows vars which were already
         * removed. This allows more precise assertions (see below).
         */
        RegSet erased;
#endif // SWIFT_DEBUG

        if ( typeid(*instr) != typeid(PhiInstr) )
        {
            // for each var on the right hand side
            for (size_t i = 0; i < instr->arg_.size(); ++i)
            {
                Reg* reg = dynamic_cast<Reg*>(instr->arg_[i].op_);

                // only color regs and regs of proper type
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
        } // if no phi instr

        // for each var on the left hand side -> assign a color for result
        for (size_t i = 0; i < instr->res_.size(); ++i)
        {
            Reg* reg = instr->res_[i].reg_;

            // only color regs and regs of proper type
            if ( !reg->colorReg(typeMask_) )
                continue;

            reg->color_ = findFirstFreeColorAndAllocate(colors);

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

void Coloring::colorConstraintedInstr(InstrNode* instrNode, Colors& colors)
{
    InstrBase* instr = instrNode->value_;
    
    RegSet liveThrough;
    RegSet args;

    // for each constrained arg
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        if ( typeid(*instr->arg_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->arg_[i].op_;

        if ( !reg->typeCheck(typeMask_) )
            continue;

        if ( instr->livesThrough(reg) )
            liveThrough.insert(reg);
    }

    // a = arg(instr) \ liveThrough(instr)
    RegVec a = args.difference(liveThrough);

    // def = res(instr)
    RegSet def;
    for (size_t i = 0; i < instr->res_.size(); ++i)
        def.insert( instr->res_[i].reg_ );

    Colors colorsD, colorsA;
    Colors freeColors(reservoir_);

    // for each constrained arg
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        if ( typeid(*instr->arg_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->arg_[i].op_;

        if ( !reg->typeCheck(typeMask_) )
            continue;

        int constraint = instr->arg_[i].constraint_;
        if (constraint == InstrBase::NO_CONSTRAINT)
            continue;

        // mark reg as occupied
        colorsA.insert(reg->color_);

        if ( liveThrough.contains(reg) )
            freeColors.erase(reg->color_);
    }

    // for each constrained result
    for (size_t i = 0; i < instr->res_.size(); ++i)
    {
        Reg* reg = (Reg*) instr->res_[i].reg_;

        if ( !reg->typeCheck(typeMask_) )
            continue;

        int constraint = instr->arg_[i].constraint_;
        if (constraint == InstrBase::NO_CONSTRAINT)
            continue;

        colorsD.insert(reg->color_);
        def.erase(reg);
    }

    // for each unconstrained arg
    for (size_t i = 0; i < instr->arg_.size(); ++i)
    {
        if ( typeid(*instr->arg_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->arg_[i].op_;

        if ( !reg->typeCheck(typeMask_) )
            continue;

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
    for (size_t i = 0; i < instr->res_.size(); ++i)
    {
        Reg* reg = (Reg*) instr->res_[i].reg_;

        if ( !reg->typeCheck(typeMask_) )
            continue;

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
}

} // namespace me
