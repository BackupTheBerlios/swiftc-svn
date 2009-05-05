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

#ifndef SWIFT_CALL_H
#define SWIFT_CALL_H

#include <vector>

/*
 * forward declarations
 */

namespace me {
    class Op;
    class Var;
}

namespace swift {

class ExprList;
class Signature;
class Tupel;
class Type;

class Call
{
public:

    /*
     * constructor
     */

    Call(ExprList* exprList, Tupel* tupel, Signature* sig);

    /*
     * further methods
     */

    bool emitCall();
    void emitStores();

    me::Var* getPrimaryPlace();
    Type* getPrimaryType();

private:

    /*
     * data
     */

    ExprList* exprList_; ///< Right hand side arguments.
    Tupel* tupel_; ///< Left hand side if present.
    Signature* sig_;

    std::vector<me::Op*> in_;
    std::vector<me::Var*> out_;
};

} // namespace swift

#endif // SWIFT_CALL_H
