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

#include "me/phiimpl.h"

namespace me {

/*
 * constructor
 */

PhiImpl::PhiImpl(BBNode* prevNode, BBNode* nextNode)
    : prevNode_(prevNode)
    , nextNode_(nextNode)
    , prevBB_(prevNode->value_)
    , nextBB_(nextNode->value_)
{
    /* 
     * Because of the critical edge elimination prevNode's 
     * successors can't have phi functions when prevNode has more than
     * one successor.
     */
    swiftAssert( prevNode->succ_.size() == 1, "must have exactly one successor");
}

/*
 * further methods
 */

void PhiImpl::genPhiInstr()
{
    collectFreeRegs();
    buildDependencyGraph();
    removeChains();
    removeCycles();
    cleanUp();
}

void PhiImpl::collectFreeRegs()
{
    // let the child class fill the set of available colors
    fillColors(); 

    /* 
     * erase all non-spilled regs which are in the live-out 
     * of the last instruction of prevNode
     */
    VARSET_EACH(iter, prevBB_->end_->prev_->value_->liveOut_) 
    {
        Reg* reg = (*iter)->isReg(freeRegTypeMask_, false);
        if (!reg)
            continue;

        swiftAssert( free_.contains(reg->color_), "colors must be found here" );
        free_.erase( reg->color_ );
    }

    // let the child class erase eventually forbidden colors
    eraseColors(); 
}

void PhiImpl::buildDependencyGraph()
{
    typedef Map<int, RegGraph::Node*> Inserted;
    Inserted inserted;

    size_t phiIndex = nextNode_->whichPred(prevNode_);

    // for each phi function in nextBB
    for (InstrNode* iter = nextBB_->firstPhi_; iter != nextBB_->firstOrdinary_; iter = iter->next())
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
                "must be a PhiInstr here" );
        PhiInstr* phi = (PhiInstr*) iter->value_;

        // ignore phi functions of MemVars
        if ( typeid(*phi->res_[0].var_) != typeid(Reg) )
            continue;

        swiftAssert( typeid(*phi->result()) == typeid(Reg),
                "must be a Reg, too");

        // get regs
        Reg* srcReg = (Reg*) phi->arg_[phiIndex].op_;
        Reg* dstReg = (Reg*) phi->res_[0].var_;

        // get Type
        Op::Type type = dstReg->type_;
        swiftAssert(srcReg->type_ == type, "types must match");

        if ( !(type & typeMask_) )
            continue;

        // is this a pointless definition? (should be optimized away)
        if ( !phi->liveOut_.contains(dstReg) )
            continue;

        swiftAssert( !!srcReg->isSpilled() == !!dstReg->isSpilled(), 
                "must be spilled, too" );

        // precede only when either spilled or non spilled should be considered
        if ( !!srcReg->isSpilled() ^ isSpilled() )
            continue;

        // is this a move of the dummy value UNDEF?
        InstrNode* def = srcReg->def_.instrNode_;
        if ( typeid(*def->value_) == typeid(AssignInstr) )
        {
            AssignInstr* ai = (AssignInstr*) def->value_;
            if (ai->kind_ == '=')
            {
                if ( typeid(*ai->arg_[0].op_) == typeid(Undef) )
                    continue; // yes -> so ignore this phi function
            }
        }

        // get colors
        int srcColor = srcReg->color_;
        int dstColor = dstReg->color_;

        /* 
         * check whether these colors have already been inserted 
         * and insert if applicable
         */
        Inserted::iterator srcIter = inserted.find(srcColor);
        if ( srcIter == inserted.end() )
        {
            srcIter = inserted.insert( std::make_pair(
                        srcColor, rg_.insert(new int(srcColor))) ).first;
        }

        Inserted::iterator dstIter = inserted.find(dstColor);
        if ( dstIter == inserted.end() )
        {
            dstIter = inserted.insert( std::make_pair(
                        dstColor, rg_.insert(new int(dstColor))) ).first;
        }

        srcIter->second->link(dstIter->second);
        linkTypes_[srcColor][dstColor] = type;
    }
}

void PhiImpl::removeChains()
{
    /*
     * while there is an edge e = (r, s) with r != s 
     * where the outdegree of s equals 0 ...
     */
    while (true)
    {
        RegGraph::Relative* iter = rg_.nodes_.first();
        while ( iter != rg_.nodes_.sentinel() && !iter->value_->succ_.empty() )
            iter = iter->next();

        if ( iter != rg_.nodes_.sentinel() )
        {
            RegGraph::Node* n = iter->value_;

            // ... resolve this by a move p -> n
            swiftAssert( n->pred_.size() == 1, 
                    "must have exactly one predecessor" );
            RegGraph::Node* p = n->pred_.first()->value_;

            int p_color = *p->value_; // save color
            int n_color = *n->value_; // save color
            Op::Type type = linkTypes_[p_color][n_color];

            genMove(type, p_color, n_color);

            if ( isSpilled() )
            {
                // p is free now while n is used now
                free_.insert(p_color);
                free_.erase (n_color);
            }

            rg_.erase(n);

            // if p doesn't have a predecessor erase it, too
            if ( p->pred_.empty() )
                rg_.erase(p);
        }
        else
            break;
    }
}

void PhiImpl::removeCycles()
{
    // while there are still nodes left
    while ( !rg_.nodes_.empty() )
    {
        // take first node
        RegGraph::Relative* relative = rg_.nodes_.first();
        RegGraph::Node* node = relative->value_;

        /*
         * remove trivial self loops 
         * without generating any instructions
         */
        if (node->succ_.first()->value_ == node)
        {
            rg_.erase(node);
            continue;
        }
            
        /*
         * -> we have a non-trivial cycle:
         * R1 -> R2 -> R3 -> ... -> Rn -> R1
         *
         * -> generate these moves:
         *  mov r1, tmp
         *  mov rn, r1
         *  ...
         *  mov r2, r3
         *  mov r1, r2
         *  mov tmp, rn
         */

         // start with pred node
        Op::Type type = linkTypes_[*node->pred_.first()->value_->value_][*node->value_];

        //  mov r1, tmp
        genReg2Tmp(type, *node->value_);

        /*
         * iterate over the cycle
         */
        std::vector<RegGraph::Node*> toBeErased;

        RegGraph::Node* dst = node; // current node
        RegGraph::Node* src = node->pred_.first()->value_; // start with pred node
        while (src != node) // until we reach r1 again
        {
            // mov r_current, r_pred
            genMove(type, *src->value_, *dst->value_);

            // remember for erasion
            toBeErased.push_back(src);

            // go to pred node
            dst = src;
            src = src->pred_.first()->value_;
        }

        int dstColor = *dst->value_;

        // mov tmp, rn
        genTmp2Reg(type, dstColor);

        // append last node to be erased
        toBeErased.push_back(node);

        // remove all handled nodes
        for (size_t i = 0; i < toBeErased.size(); ++i)
            rg_.erase( toBeErased[i] );

    } // while there are still nodes left
}

}
