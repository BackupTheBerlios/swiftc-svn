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

#ifndef ME_DEF_USE_CALC_H
#define ME_DEF_USE_CALC_H

#include <fstream>

#include "me/codepass.h"
#include "me/functab.h"

namespace me {


class DefUseCalc : public CodePass
{
public:

    /*
     * constructor
     */

    DefUseCalc(Function* function);

    /*
     * methods
     */

    virtual void process();

private:

    /**
     * @brief Compiles for all vars their defining instruction. 
     *
     * The left hand side of \a PhiInstr instances counts as definition, too.
     */
    void calcDef();

    /** 
     * @brief Compiles for all v in vars the instructinos which make use of v.
     *
     * The right hand side of \a PhiInstr instances count as a use, too. 
     */
    void calcUse();

    /** 
     * @brief Used internally by \a CalcUse.
     * 
     * @param var Current var. 
     * @param bb Current basic block.
     */
    void calcUse(Reg* var, BBNode* bb);
};

} // namespace me

#endif // ME_DEF_USE_CALC_H
