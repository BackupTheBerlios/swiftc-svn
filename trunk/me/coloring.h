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

#ifndef ME_COLORING_H
#define ME_COLORING_H

#include "utils/set.h"

#include "me/codepass.h"
#include "me/forward.h"

namespace me {

typedef Set<int> Colors;

class Coloring : public CodePass
{
private: 

    int typeMask_;
    const Colors reservoir_;
    size_t stackPlace_;

public:

    /*
     * constructors
     */

    /// Use this one for coloring of memory locations.
    Coloring(Function* function, size_t stackPlace);
    Coloring(Function* function, int typeMask, const Colors& reservoir);

    /*
     * methods
     */

    virtual void process();

private:

    /*
     * memory location coloring
     */

    void colorRecursiveMem(BBNode* bbNode);
    int getFreeMemColor(Colors& colors);

    /*
     * register coloring
     */

    void colorRecursive(BBNode* bbNode);
    void colorConstraintedInstr(InstrNode* instrNode, VarSet& alreadyColored);
};

} // namespace me

#endif // ME_COLORING_H
