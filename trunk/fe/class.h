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

#ifndef SWIFT_CLASS_H
#define SWIFT_CLASS_H

#include <map>
#include <set>

#include "utils/list.h"

#include "fe/module.h"

/*
 * forward declarations
 */

namespace me {
    struct Member;
    struct Struct;
}

namespace swift {

struct BaseType;
struct ClassMember;
struct MemberFunction;
struct Ptr;
struct Type;

//------------------------------------------------------------------------------

/**
 * This is the representation of a Class in Swift.
 * It knows of its class members and methods.
 */
struct Class : public Definition
{
    /**
     * Knows whether the given class defines a constructor,
     * if this is not the case a default constructor must be created artifically
     */
    bool hasCreate_;

    enum DefaultCreate
    {
        DEFAULT_NONE,
        DEFAULT_TRIVIAL,
        DEFAULT_USER,
    };

    DefaultCreate defaultCreate_;

    enum CopyCreate
    {
        COPY_NONE,
        COPY_AUTO,
        COPY_USER
    };

    CopyCreate copyCreate_;

    ClassMember* classMember_; ///< Linked list of class members.

    typedef std::multimap<const std::string*, MemberFunction*, StringPtrCmp> MemberFunctionMap;
    typedef std::map<const std::string*, MemberVar*, StringPtrCmp> MemberVarMap;

    MemberFunctionMap memberFunctions_; ///< Methods defined in this class.
    MemberVarMap memberVars_;           ///< MemberVars defined in this class.

    me::Struct* meStruct_;

    /*
     * constructor and destructor
     */

    Class(std::string* id, Symbol* parent, int line);
    virtual ~Class();

    /*
     * virtual methods
     */

    virtual bool analyze();

    /*
     * further methods
     */

    void autoGenMethods();
    void prependMember(ClassMember* newMember);
    void addDefaultCreate();
    void addCopyCreate();
    void addAssignOperators();
    BaseType* createType(int modifier) const;
};

//------------------------------------------------------------------------------

/**
 * This class represents either a MemberVar or a Method.
 */
struct ClassMember : public Symbol
{
    ClassMember* next_; ///< Linked List of class members.

    /*
     * constructor and destructor
     */

    ClassMember(std::string* id, Symbol* parent, int line = NO_LINE);
    virtual ~ClassMember();

    /*
     * further Methods
     */

    virtual bool analyze() = 0;
};

//------------------------------------------------------------------------------

/**
 * This class represents a member variable of a Class.
 */
struct MemberVar : public ClassMember
{
    Type* type_;
    me::Member* meMember_;

    /*
     * constructor and destructor
     */

    MemberVar(Type* type, std::string* id, Symbol* parent = 0, int line = NO_LINE);
    virtual ~MemberVar();

    /*
     * further methods
     */

    virtual bool analyze();

    bool registerMeMember();
};

} // namespace swift

#endif // SWIFT_CLASS_H
