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

#ifndef ME_CODE_PASS_H
#define ME_CODE_PASS_H

namespace me {

// forward declarations
class Function;
class CFG;

class CodePass
{
protected:

    Function* function_;
    CFG* cfg_;

public:

    /*
     * constructor
     */

    CodePass(Function* function);
    virtual ~CodePass() {}

    /*
     * methods
     */

    virtual void process() = 0;

    /*
     * further methods
     */

    CFG* cfg();
    const CFG* cfg() const;
};

} // namespace me

#endif // ME_CODE_PASS_H
