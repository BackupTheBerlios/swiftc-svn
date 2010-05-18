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

#ifndef SWIFT_PROC
#define SWIFT_PROC

#include <map>
#include <string>
#include <vector>

#include "utils/stringhelper.h"

#include "fe/typelist.h"
#include "fe/var.h"

namespace swift {

class Context;
class InOut;

//------------------------------------------------------------------------------

class Sig
{
public:

    ~Sig();

    void setInList(Context* ctxt);
    void setOutList(Context* ctxt);

    bool checkIn(Module* module, const TypeList& inTypes) const;
    bool checkOut(Module* module, const TypeList& outTypes) const;
    
    InOut* lookupInOut(const std::string* id) const;
    void buildTypeLists();

    IOs in_;
    IOs out_;
    TypeList inTypes_;
    TypeList outTypes_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif //SWIFT_PROC
