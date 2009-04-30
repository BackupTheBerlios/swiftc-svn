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

#ifndef SWIFT_DECL_H
#define SWIFT_DECL_H

#include <string>

#include "fe/syntaxtree.h"

namespace swift {

/*
 * forward declarations
 */

class Local;
class Type;

//------------------------------------------------------------------------------

class Decl : public TypeNode
{
public:

    /*
     * constructor and destructor
     */

    Decl(Type* type, std::string* id, int line);
    virtual ~Decl();

    /*
     * virtual methods
     */

    virtual bool analyze();

    /*
     * further methods
     */

    std::string toString() const;
    void setAsStandAlone();

private:

    /*
     * data
     */

    std::string* id_;
    Local* local_;
    bool standAlone_;
};

} // namespace swift

#endif // SWIFT_DECL_H
