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

#ifndef SWIFT_SIMD_PREFIX_H
#define SWIFT_SIMD_PREFIX_H

#include "fe/syntaxtree.h"

#include "me/forward.h"

namespace swift {

/*
 * forward declarations
 */

class Expr;

class SimdPrefix : public Node
{
public:

    /*
     * constructor and destructor
     */

    SimdPrefix(Expr* leftExpr_, Expr* rightExpr, int line);
    virtual ~SimdPrefix();

    /*
     * virtual methods
     */

    virtual std::string toString() const;

    /*
     * further methods
     */

    void genSSA();
    bool analyze();

private:

    /*
     * index expresions
     */

    Expr* leftExpr_;
    Expr* rightExpr_;

    /*
     * regs
     */

    me::Reg* counter_;
    me::Reg* check_;
    
    /*
     * labels
     */

    me::InstrNode* simdLabelNode_;
    me::InstrNode* nextLabelNode_;
};

} // namespace swift

#endif // SWIFT_SIMD_PREFIX_H
