#include "me/coloring.h"

#include <algorithm>
#include <iostream>
#include <typeinfo>

#include "me/cfg.h"
#include "me/op.h"

namespace me {

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
    std::vector<int> freeColors( reservoir_.size() );

    std::vector<int>::iterator end = std::set_difference( 
            reservoir_.begin(), reservoir_.end(), 
            colors.begin(), colors.end(), 
            freeColors.begin() );

    freeColors.erase( end, freeColors.end() ); // truncate properly

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
            for (size_t i = 0; i < instr->rhs_.size(); ++i)
            {
                Reg* reg = dynamic_cast<Reg*>(instr->rhs_[i].op_);

                // only color regs and regs of proper type
                if ( !reg || !reg->colorReg(typeMask_) )
                    continue;

                if ( InstrBase::isLastUse(iter, reg) )
                {
                    // -> its the last use of reg
                    Colors::iterator colorIter = colors.find(reg->color_);
#ifdef SWIFT_DEBUG
                    // has this reg already been erased due to a double entry like a = b + b?
                    if ( erased.find(reg) != erased.end() )
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
            } // for each rhs var
        } // if no phi instr

        // for each var on the left hand side -> assign a color for result
        for (size_t i = 0; i < instr->lhs_.size(); ++i)
        {
            Reg* reg = instr->lhs_[i].reg_;

            // only color regs and regs of proper type
            if ( !reg->colorReg(typeMask_) )
                continue;

            reg->color_ = findFirstFreeColorAndAllocate(colors);

            // pointless definitions should be optimized away
            if (instr->liveOut_.find(reg) == instr->liveOut_.end())
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

void Coloring::colorConstraintedInstr(InstrNode* instrNode)
{
    InstrBase* instr = instrNode->value_;
    
    // for each constrained arg
    for (size_t i = 0; i < instr->rhs_.size(); ++i)
    {
        if ( typeid(*instr->rhs_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->rhs_[i].op_;

        if ( !reg->typeCheck(typeMask_) )
            continue;

        int constraint = instr->rhs_[i].constraint_;
        if (constraint == InstrBase::NO_CONSTRAINT)
            continue;

        // TODO
    }

    // for each constrained result
    for (size_t i = 0; i < instr->lhs_.size(); ++i)
    {
        Reg* reg = (Reg*) instr->lhs_[i].reg_;

        if ( !reg->typeCheck(typeMask_) )
            continue;

        int constraint = instr->rhs_[i].constraint_;
        if (constraint == InstrBase::NO_CONSTRAINT)
            continue;

        // TODO
    }

    // for each unconstrained arg
    for (size_t i = 0; i < instr->rhs_.size(); ++i)
    {
        if ( typeid(*instr->rhs_[i].op_) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) instr->rhs_[i].op_;

        if ( !reg->typeCheck(typeMask_) )
            continue;

        // TODO
    }

    // for each unconstrained result
    for (size_t i = 0; i < instr->lhs_.size(); ++i)
    {
        Reg* reg = (Reg*) instr->lhs_[i].reg_;

        if ( !reg->typeCheck(typeMask_) )
            continue;

        // TODO
    }
}

} // namespace me
