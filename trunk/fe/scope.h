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

#ifndef SWIFT_SCOPE
#define SWIFT_SCOPE

#include <map>
#include <string>
#include <vector>

#include "utils/stringhelper.h"

namespace swift {

class Local;
class Sig;
class Stmnt;
class StmntVisitor;

//------------------------------------------------------------------------------

class Scope
{
public:

    Scope(Scope* parent);
    ~Scope();

    Local* lookupLocalOneLevelOnly(const std::string* id);
    Local* lookupLocal(const std::string* id);

    bool insert(Local* local);
    void appendStmnt(Stmnt* stmnt);
    void accept(StmntVisitor* s);

private:

    Scope* parent_;

    typedef std::map<const std::string*, Local*, StringPtrCmp> LocalMap;
    LocalMap locals_;

    typedef std::vector<Stmnt*> Stmnts;
    Stmnts stmnts_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_SCOPE
