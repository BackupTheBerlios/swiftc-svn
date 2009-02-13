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

#include "me/op.h"

namespace me {

struct Struct;

//------------------------------------------------------------------------------

/**
 * @brief With Member you can build a complex layout of data types, 
 * which all have an at compile time known offset to the base pointer.
 *
 * Member instances are types for locations in member. This is a different thing
 * than spilled locals of type \a me::Op::Type: 
 * The middle-end assumes that there are infinit registers available. Spilling
 * is only a product of the fact, they are limited in reality.
 *
 * \a Member is the base class for all more complex data types. It is assumed
 * that in a first pass all \a Struct objects are registered as types. In a
 * second pass all member can be added. 
 *
 * Each Member is given a unique 'name': \a nr_. However this name is only
 * needed when it is used within a Struct.
 * 
 * Each \a Member class must implement \a analyze. This must be invoked on all
 * root data types. \a size_ and \a offset_ will be calculated there.
 *
 * \a parent_ points to the parent Struct type. \a size_ and \a offset_ are 
 * calculated relative to that. If it does not belong to a parent Struct
 * or this is a root Struct set it to 0.
 *
 * For better debugging you can use the \a id_ member.
 */
struct Member
{
    int nr_;        ///< Global name.
    int size_;      ///< Size in bytes of this \a Member.
    int offset_;    ///< Offest in bytes relative to its \a parent_.

#ifdef SWIFT_DEBUG
    std::string id_;///< Use this for additional debugging information.
#endif // SWIFT_DEBUG

    /*
     * constructor and destructor
     */

#ifdef SWIFT_DEBUG
    Member(const std::string& id);
#else // SWIFT_DEBUG
    Member();
#endif // SWIFT_DEBUG

    virtual ~Member() {}

    /*
     * further methods
     */

    //virtual void analyze() = 0;
};

//------------------------------------------------------------------------------

/** 
 * @brief This is a simple type.
 *
 * This means one of \a me::Op::Type.
 */
struct SimpleType : public Member
{
    Op::Type type_;

    /*
     * constructor
     */

#ifdef SWIFT_DEBUG
    SimpleType(Op::Type type, const std::string& id);
#else // SWIFT_DEBUG
    SimpleType(Op::Type type);
#endif // SWIFT_DEBUG

    /*
     * further methods
     */

    virtual void analyze();
};

//------------------------------------------------------------------------------

/** 
 * @brief This is an array of an Member type.
 *
 * The number of elemets \a size_ is fix and must be known at compile time.
 */
struct FixedSizeArray : public Member
{
    Member* type_;
    const size_t num_;

    /*
     * constructors and destructor
     */

#ifdef SWIFT_DEBUG
    FixedSizeArray(Member* type, size_t num, const std::string& id);
#else // SWIFT_DEBUG
    FixedSizeArray(Member* type, size_t num);
#endif // SWIFT_DEBUG
};

//------------------------------------------------------------------------------

/** 
 * @brief Represents struct or record types.
 *
 * The members are inserted in \a members_ and \memberMap_. The former one has 
 * chronological order, the latter one is sorted by the global number.
 * 
 */
struct Struct : public Member
{
    typedef std::vector<Member*> Members;
    /// All members in chronological order.
    Members members_;

    /// All members sorted by global number.
    typedef std::map<int, Member*> MemberMap;
    MemberMap memberMap_;

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
    //Member* append(Struct* _struct, const std::string& id);
#else // SWIFT_DEBUG
    Member* append(Op::Type type);
    //Member* append(Struct* _struct);
#endif // SWIFT_DEBUG

    Member* lookup(int nr);

    //void analyze();
};

} // namespace me

#endif // ME_STRUCT_H
