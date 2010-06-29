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

class Context;
class Var;
class Sig;
class Stmnt;
class StmntVisitorBase;

//------------------------------------------------------------------------------

class Scope
{
public:

    Scope(Scope* parent);
    ~Scope();

    Var* lookupVarOneLevelOnly(const std::string* id);
    Var* lookupVar(const std::string* id);

    void insert(Var* var);
    void appendStmnt(Stmnt* stmnt);
    void accept(StmntVisitorBase* s);
    bool isEmpty() const;

private:

    Scope* parent_;

    typedef std::map<const std::string*, Var*, StringPtrCmp> VarMap;
    VarMap vars_;

    typedef std::vector<Stmnt*> Stmnts;
    Stmnts stmnts_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_SCOPE
