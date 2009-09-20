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

#include "me/loopsfinder.h"

#include <iostream>
#include <vector>

#include "me/cfg.h"
#include "me/functab.h"

namespace me {

/*
 * constructor
 */

LoopsFinder::LoopsFinder(Function* function)
    : CodePass(function)
{}

/*
 * virtual methods
 */

void LoopsFinder::process()
{
    //std::cout << "-- " << *function_->id_ << " --" << std::endl;
    // for each node
    CFG_RELATIVES_EACH(fromIter, cfg_->nodes_)
    {
        BBNode* fromNode = fromIter->value_;
        //BasicBlock* from = fromNode->value_;

        // for each successor
        CFG_RELATIVES_EACH(toIter, fromNode->succ_)
        {
            BBNode* toNode = toIter->value_;
            BasicBlock* to = toNode->value_;

            /*
             * examine edge 'from' -> 'to':
             * does 'to' dominate 'from'?
             */

            if ( to->hasDomChild(fromNode) )
            {
                // yes, so 'from' -> 'to' is a back edge

                // find loop body
                //std::cout << from->name() << " -> " << to->name() << std::endl;
            }
        }
    }
}

/*
 * further methods
 */

} // namespace me
