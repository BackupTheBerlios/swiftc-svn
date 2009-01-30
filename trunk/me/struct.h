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

#ifndef ME_STRUCT_H
#define ME_STRUCT_H

#include <map>
#include <stack>

#include "me/op.h"

namespace me {

struct Struct;

struct Member
{
    Struct* parent_;
    int nr_;
    int size_;
    int offset_;

    union
    {
        Op::Type type_;
        Struct* struct_;
    };

    bool simpleType_;

#ifdef SWIFT_DEBUG
    std::string id_;
#endif // SWIFT_DEBUG

    /*
    * constructors and destructor
    */

#ifdef SWIFT_DEBUG
    Member(Struct* parent, Struct* _struct, const std::string& id);
    Member(Struct* parent, Op::Type type, const std::string& id);
#else // SWIFT_DEBUG
    Member(Struct* parent, Struct* _struct);
    Member(Struct* parent, Op::Type type);
#endif // SWIFT_DEBUG
};

struct Struct
{
    static int nameCounter_;

    int nr_;
    int memberNameCounter_;
    int size_;

    enum
    {
        NOT_ANALYZED = -1
    };

#ifdef SWIFT_DEBUG
    std::string id_;
#endif // SWIFT_DEBUG

    typedef std::map<int, Member*> MemberMap;
    MemberMap members_;

    /*
     * constructor and destructor
     */

#ifdef SWIFT_DEBUG
    Struct(const std::string& id);
#else // SWIFT_DEBUG
    Struct();
#endif // SWIFT_DEBUG

    ~Struct();

    /*
     * further methods
     */

#ifdef SWIFT_DEBUG
    Member* append(Op::Type type, const std::string& id);
    Member* append(Struct* _struct, const std::string& id);
#else // SWIFT_DEBUG
    Member* append(Op::Type type);
    Member* append(Struct* _struct);
#endif // SWIFT_DEBUG

    Member* lookup(int nr);

    void analyze();
};

} // namespace me

#endif // ME_STRUCT_H
