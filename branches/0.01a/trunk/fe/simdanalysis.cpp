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

#include "fe/simdanalysis.h"

#include <iostream>
#include <sstream>
#include <stack>

#include "utils/assert.h"

#include "fe/error.h"
#include "fe/module.h"
#include "fe/symtab.h"
#include "fe/type.h"

#include "me/arch.h"
#include "me/ssa.h"

namespace swift {

/*
 * constructor 
 */

SimdAnalysis::SimdAnalysis(location loc)
    : std::vector<SimdInfo>()
    , loc_(loc)
    , simdLength_(NOT_ANALYZED)
{}

/*
 * further methods
 */

int SimdAnalysis::checkAndGetSimdLength()
{
    if ( empty() )
    {
        simdLength_ = me::arch->getSimdWidth();
        return simdLength_;
    }

    int tmp = 0;

    for (size_t i = 0; i < size(); ++i)
    {
        int simdLength = (*this)[i].simdLength_;

        if (tmp == 0)
            tmp = simdLength;
        else if (tmp != simdLength)
        {
            errorf(loc_, "different simd lengths used in this simd statement");
            return -1;
        }
    }

    if (tmp == 0)
        simdLength_ = me::arch->getSimdWidth();
    else
        simdLength_ = tmp;

    return simdLength_;
}

int SimdAnalysis::getSimdLength() const
{
    swiftAssert(simdLength_ != NOT_ANALYZED, "has not been analyzed yet");
    return simdLength_;
}

} // namespace swift
