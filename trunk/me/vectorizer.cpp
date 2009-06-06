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

#include "me/vectorizer.h"

#include "me/cfg.h"
#include "me/functab.h"

namespace me {

Vectorizer::Vectorizer(Function* function)
    : CodePass(function)
    , simdFunction_( me::functab->insertFunction(
                new std::string(*function->id_ + "simd"), false) )
{
}

void Vectorizer::process()
{
    // for each instruction
    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        //InstrBase* instr = iter->value_;

        //if ( typeid(*instr) == typeid(LabelInstr) )
        //{
            //currentBB = cfg_->labelNode2BBNode_[iter];
            //continue;
        //}

        //if ( instr->isConstrained() )
            //liveRangeSplit(iter, currentBB);
    }

    cfg_->constructSSAForm();
}

} // namespace me
