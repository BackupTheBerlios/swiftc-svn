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
        AssignmentBase* ab = dynamic_cast<AssignmentBase*>(iter->value_);

        if (ab)
        {
#ifdef SWIFT_DEBUG
            /*
             * In the debug version this set knows vars which were already
             * removed. This allows more precise assertions (see below).
             */
            RegSet erased;
#endif // SWIFT_DEBUG

            if ( typeid(*ab) != typeid(PhiInstr) )
            {
                // for each var on the right hand side
                for (size_t i = 0; i < ab->numRhs_; ++i)
                {
                    Reg* reg = dynamic_cast<Reg*>(ab->rhs_[i]);

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
            for (size_t i = 0; i < ab->numLhs_; ++i)
            {
                Reg* reg = ab->lhs_[i];

                // only color regs and regs of proper type
                if ( !reg->colorReg(typeMask_) )
                    continue;

                reg->color_ = findFirstFreeColorAndAllocate(colors);

                // pointless definitions should be optimized away
                if (ab->liveOut_.find(reg) == ab->liveOut_.end())
                    colors.erase( colors.find(reg->color_) );
            }
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
    swiftAssert( dynamic_cast<AssignmentBase*>(instrNode->value_),
            "must be an AssignmentBase here");

    AssignmentBase* ab = (AssignmentBase*) instrNode->value_;

    // for each constrained arg
    for (size_t i = 0; i < ab->numRhs_; ++i)
    {
        if ( typeid(*ab->rhs_[i]) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) ab->rhs_[i];
    }

    // for each constrained result
    for (size_t i = 0; i < ab->numLhs_; ++i)
    {
        Reg* reg = (Reg*) ab->lhs_[i];
    }

    // for each unconstrained arg
    for (size_t i = 0; i < ab->numRhs_; ++i)
    {
        if ( typeid(*ab->rhs_[i]) != typeid(Reg) )
            continue;

        Reg* reg = (Reg*) ab->rhs_[i];
    }

    // for each unconstrained result
    for (size_t i = 0; i < ab->numLhs_; ++i)
    {
        Reg* reg = (Reg*) ab->lhs_[i];
    }
}

} // namespace me
