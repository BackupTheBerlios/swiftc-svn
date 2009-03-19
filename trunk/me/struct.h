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

#include "utils/map.h"

#include "me/op.h"

namespace me {

struct Struct;

//------------------------------------------------------------------------------

/**
 * @brief With Member you can build a complex layout of data types, 
 * which all have an at compile time known offset to the base.
 *
 * Member instances are types for locations in memory. This is a different thing
 * than spilled locals of type \a me::Op::Type: 
 * The middle-end assumes that there are infinit registers available. Spilling
 * is only a product of the fact, they are limited in reality.
 *
 * \a Member is the base class for all more complex data types. It is assumed
 * that in a first pass all \a Struct objects are registered as types. In a
 * second pass all members can be added. 
 *
 * Each \a Member instance has a unique \a nr_ which can be used for lookups.
 *
 * Each \a Member class must implement \a analyze. This must be invoked on all
 * root data types. \a size_ and \a offset_ will be calculated there.
 *
 * For better debugging you can use the \a id_ member.
 */
struct Member
{
    enum 
    {
        NOT_ANALYZED = -1
    };

    /// A Unique global name.
    int nr_;

    /// Size in bytes of this \a Member or \a NOT_ANALYZED if not yet analyzed.
    int size_; 

    ///< Offset in bytes of this \a Member relative to its direct root \a Struct.
    int offset_;

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

    virtual void analyze() = 0;

    bool alreadyAnalyzed() const;
};

//------------------------------------------------------------------------------

/** 
 * @brief This is a simple type.
 *
 * This means one of \a me::Op::Type.
 */
struct AtomicMember : public Member
{
    Op::Type type_;

    /*
     * constructor
     */

#ifdef SWIFT_DEBUG
    AtomicMember(Op::Type type, const std::string& id);
#else // SWIFT_DEBUG
    AtomicMember(Op::Type type);
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
struct ArrayMember : public Member
{
    Op::Type type_;
    const size_t num_;

    /*
     * constructors and destructor
     */

#ifdef SWIFT_DEBUG
    ArrayMember(Op::Type type, size_t num, const std::string& id);
#else // SWIFT_DEBUG
    ArrayMember(Op::Type type, size_t num);
#endif // SWIFT_DEBUG

    /*
     * further methods
     */

    virtual void analyze();
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

    /// All members sorted by global name number.
    typedef Map<int, Member*> MemberMap;
    MemberMap memberMap_;

    /*
     * constructor
     */

#ifdef SWIFT_DEBUG
    Struct(const std::string& id);
#else // SWIFT_DEBUG
    Struct();
#endif // SWIFT_DEBUG

    /*
     * further methods
     */

    void append(Member* member);
    Member* lookup(int nr);
    virtual void analyze();

#ifdef SWIFT_DEBUG
    std::string toString() const;
#endif // SWIFT_DEBUG
};

} // namespace me

#endif // ME_STRUCT_H
