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

#ifndef SWIFT_TYPE_LIST_H
#define SWIFT_TYPE_LIST_H

#include <string>
#include <vector>

namespace llvm {
    class Type;
    class Value;
}

typedef std::vector<llvm::Value*> Values;
typedef std::vector<const llvm::Type*> LLVMTypes;

namespace swift {

class Module;
class Type;

//------------------------------------------------------------------------------

class TypeList : public std::vector<const Type*>
{
public:

    std::string toString() const;
    bool check(Module* m, const TypeList& t) const;
}; 

//------------------------------------------------------------------------------

typedef std::vector<bool> BoolVec;

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPE_LIST_H
