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

#include "me/instrcoalescing.h"

#include <limits>

#include "me/cfg.h"
#include "me/functab.h"
#include "me/op.h"
#include "me/ssa.h"

/*
 * helpers
 */

#define NODES_EACH(iter, nodes) \
    for (me::InstrCoalescing::Nodes::iterator (iter) = (nodes).begin(); (iter) != (nodes).end(); ++(iter))
#define NODESET_EACH(iter, nodeSet) \
    for (me::InstrCoalescing::NodeSet::iterator (iter) = (nodeSet).begin(); (iter) != (nodeSet).end(); ++(iter))


namespace me {

/*
 * constructor
 */

InstrCoalescing::InstrCoalescing(Function* function, 
                       const Colors& reservoir, 
                       int typeMask)
    : CodePass(function)
    , reservoir_(reservoir)
    , typeMask_(typeMask)
{}

InstrCoalescing::InstrCoalescing(Function* function, 
                       int typeMask)
    : CodePass(function)
    , reservoir_(Colors())  // use an empty set
    , typeMask_(typeMask)
{}

/*
 * virtual methods
 */

void InstrCoalescing::process()
{
    // register each reg which can be principally used
    VARMAP_EACH(iter, function_->vars_)
    {
        Reg* reg = iter->second->isReg( typeMask_, isSpilled() );

        if ( !reg || reg->isUsedinPhiFunction() ) 
            continue;

        nodes_.insert( std::make_pair(reg, Node(reg)) );
    }

    NODES_EACH(iter, nodes_)
    {
        Node* n = &iter->second;
        int color = n->reg_->getPreferedColor();

        if (color == -1 || color == n->reg_->color_)
            continue;

        //-> try to recolor
        recolor(n, color);
    }
}

/*
 * further methods
 */

bool InstrCoalescing::isSpilled() const
{
    return reservoir_.empty();
}

void InstrCoalescing::recolor(Node* n, int color)
{
    if ( !n->reg_->isColorAdmissible(color) )
        return;

    NodeSet changed;
    setColor(n, color, changed);

    RegSet neighbors = cfg_->intNeighbors( n->reg_, typeMask_, isSpilled() );
    bool rollback = false;

    // for each neighbor
    REGSET_EACH(iter, neighbors)
    {
        Reg* neighbor = *iter;

        // does it also have color 'color'?
        if (neighbor->color_ == color)
        {
            Nodes::iterator nodeIter = nodes_.find(neighbor);

            if ( nodeIter == nodes_.end() || !avoidColor(&nodeIter->second, color, changed) )
            {
                // -> that did not work so rollback
                for (NodeSet::iterator changedIter = changed.begin(); changedIter != changed.end(); ++changedIter)
                {
                    rollback = true;
                    Node* current = *changedIter;
                    swiftAssert(current->oldColor_ != -1, "must be set");
                    current->reg_->color_ = current->oldColor_;
                }
            }
        }
    }

    NODESET_EACH(iter, changed)
    {
        Node* n = *iter;
        n->fixed_ = false;
        
        // TODO there are false positives
        if (!rollback)
            function_->usedColors_.insert(n->reg_->color_);
    }
}

void InstrCoalescing::setColor(Node* n, int color, NodeSet& changed)
{
    n->fixed_ = true;
    n->oldColor_ = n->reg_->color_;
    n->reg_->color_ = color;
    changed.insert(n);
}

bool InstrCoalescing::avoidColor(Node* n, int color, NodeSet& changed)
{
    if (n->reg_->color_ != color)
        return true;

    if (n->fixed_)
        return false;

    // find admissible colors for n
    std::vector<int> admissible;
    int prefered = n->reg_->getPreferedColor();
    bool preferedSet = false;

    if (prefered != -1 && n->reg_->color_ != prefered && n->reg_->isColorAdmissible(prefered) )
    {
        admissible.push_back(prefered);
        preferedSet = true;
    }

    COLORS_EACH(iter, reservoir_)
    {
        int admissibleColor = *iter;

        if ( admissibleColor == color || (preferedSet && prefered == admissibleColor) )
            continue;

        if ( n->reg_->isColorAdmissible(admissibleColor) )
            admissible.push_back(admissibleColor);
    }

    if ( admissible.empty() )
        return false;

    RegSet neighbors = cfg_->intNeighbors( n->reg_, typeMask_, isSpilled() );

    // select a color newColor in admissible which is used least in neighbors
    int min = std::numeric_limits<int>::max();
    int newColor;

    for (size_t i = 0; i < admissible.size(); ++i)
    {
        int currentColor = admissible[i];
        int counter = 0;

        REGSET_EACH(iter, neighbors)
        {
            Reg* neighbor = *iter;
            if (neighbor->color_ == currentColor)
                ++counter;
        }

        if (min > counter )
        {
            min = counter;
            newColor = currentColor;
        }
    }

    swiftAssert( min != std::numeric_limits<int>::max(), 
            "at least one color must have been found" );
    setColor(n, newColor, changed);

    REGSET_EACH(iter, neighbors)
    {
        Reg* neighbor = *iter;

        if (neighbor->color_ != newColor)
            continue;

        Nodes::iterator nodeIter = nodes_.find(neighbor);
        if ( nodeIter == nodes_.end() )
            return false;

        Node* neighborNode = &nodeIter->second;
        if ( !avoidColor(neighborNode, newColor, changed) )
            return false;
    }

    return true;
}

#undef NODES_EACH
#undef NODESET_EACH

} // namespace me

