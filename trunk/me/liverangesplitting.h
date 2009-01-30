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

#ifndef ME_LIVE_RANGE_SPLITTING_H
#define ME_LIVE_RANGE_SPLITTING_H

#include "me/codepass.h"
#include "me/functab.h"

namespace me {

class LiveRangeSplitting : public CodePass
{
private:

    RDUMap phis_;

public:

    /*
     * constructor and destructor
     */

    LiveRangeSplitting(Function* function);
    virtual ~LiveRangeSplitting();

    /*
     * further methods
     */

    virtual void process();

private:

    void liveRangeSplit(InstrNode* instrNode, BBNode* bbNode);
};

} // namespace me

#endif // ME_LIVE_RANGE_SPLITTING_H

