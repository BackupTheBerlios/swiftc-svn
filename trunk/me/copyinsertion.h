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

#ifndef ME_COPY_INSERTION_H
#define ME_COPY_INSERTION_H

#include "me/codepass.h"
#include "me/functab.h"

namespace me {

/** 
 * @brief Insert copies of regs if necessary.
 *
 * This pass ensures that a live-through arg which is constrained to the same
 * color as a result is copied.
 */
class CopyInsertion : public CodePass
{
private:

    RDUMap phis_;

public:

    /*
     * constructor and destructor
     */

    CopyInsertion(Function* function);

    /*
     * further methods
     */

    virtual void process();

private:

    void insertIfNecessary(InstrNode* instrNode);

    void insertCopy(size_t regIdx, InstrNode* instrNode);
};

} // namespace me

#endif // ME_COPY_INSERTION_H


