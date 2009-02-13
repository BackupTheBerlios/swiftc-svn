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

#include "me/struct.h"

#include <typeinfo>

#include "me/arch.h"

/*
 * globals
 */

namespace {

int namecounter = 0;

int type2size(me::Op::Type type)
{
    switch (type)
    {
        case me::Op::R_BOOL: 
        case me::Op::R_INT8:
        case me::Op::R_UINT8:
        case me::Op::R_SAT8:
        case me::Op::R_USAT8:
            return 1;

        case me::Op::R_INT16:
        case me::Op::R_UINT16:
        case me::Op::R_SAT16:
        case me::Op::R_USAT16:
            return 2;

        case me::Op::R_INT32:
        case me::Op::R_UINT32:
        case me::Op::R_REAL32:
            return 4;

        case me::Op::R_INT64:
        case me::Op::R_UINT64:
        case me::Op::R_REAL64:
            return 8;

        case me::Op::R_PTR:
            return me::arch->getPtrSize();

        case me::Op::R_STACK:
        case me::Op::R_SPECIAL:
            swiftAssert(false, "illegal switch-case-value");
            return 0;
    }

    return 0;
}

/** 
 * @brief Calulates the aligned offset of a \a Member based on its unaligned
 * \p offset, \a Arch::alignOf and its \p size.
 * 
 * @param offset The unaligned offset.
 * @param size The size of the \a Member item.
 * 
 * @return The aligned offset.
 */
int calcAlignedOffset(int offset, int size)
{
    int newOffset = offset + size;
    int align = me::arch->alignOf(size);
    int mod = newOffset % align;

    // do we have to adjust the offset due to alignment?
    if (mod)
        newOffset += align - mod;

    return newOffset;
}

} // namespace

//------------------------------------------------------------------------------

namespace me {

/*
 * constructor and destructor
 */

#ifdef SWIFT_DEBUG

Member::Member(const std::string& id)
    : nr_(namecounter++)
    , size_(0)
    , offset_(0)
    , id_(id)
{}

#else // SWIFT_DEBUG

Member::Member()
    : nr_(namecounter++)
    , size_(0)
    , offset_(0)
{}

#endif // SWIFT_DEBUG

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

#ifdef SWIFT_DEBUG

SimpleType::SimpleType(Op::Type type, const std::string& id)
    : Member(id)
    , type_(type)
{}

#else // SWIFT_DEBUG

SimpleType::SimpleType(Op::Type type)
    , type_(type)
{}

#endif // SWIFT_DEBUG

/*
 * further methods
 */

void SimpleType::analyze()
{
}

//------------------------------------------------------------------------------

#ifdef SWIFT_DEBUG

FixedSizeArray::FixedSizeArray(Member* type, size_t num, const std::string& id)
    : Member(id)
    , type_(type)
    , num_(num)
{}

#else // SWIFT_DEBUG

FixedSizeArray::FixedSizeArray(Member* type, size_t num);
    , type_(type)
    , num_(num)
{}

#endif // SWIFT_DEBUG

/*
 * further methods
 */

//void FixedSizeArray::analyze()
//{
//}
    
//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

#ifdef SWIFT_DEBUG

Struct::Struct(const std::string& id)
    : Member(id)
{}

#else // SWIFT_DEBUG

Struct::Struct()
{}

#endif // SWIFT_DEBUG

Struct::~Struct()
{
    for (size_t i = 0; i < members_.size(); ++i)
    {
        if ( typeid(*members_[i]) != typeid(Struct) )
            delete members_[i];
    }
}

/*
 * further methods
 */

#ifdef SWIFT_DEBUG

Member* Struct::append(Op::Type type, const std::string& id)
{
    Member* member = new SimpleType(type, id);
    memberMap_[member->nr_] = member;
    members_.push_back(member);

    return member;
}

//Member* Struct::append(Struct* _struct, const std::string& id)
//{
    //Member* member = new Member(this, _struct, id);
    //members_[member->nr_] = member;

    //return member;
//}

#else // SWIFT_DEBUG

Member* Struct::append(Op::Type type)
{
    Member* member = new SimpleType(this, type);
    memberMap_[member->nr_] = member;
    members_.push_back(member);

    return member;
}

/*Member* Struct::append(Struct* _struct)*/
//{
    //Member* member = new Member(this, _struct);
    //members_[member->nr_] = member;

    //return member;
/*}*/

#endif // SWIFT_DEBUG

Member* Struct::lookup(int nr)
{
    MemberMap::iterator iter = memberMap_.find(nr);

    if ( iter == memberMap_.end() )
        return 0;
    else
        return iter->second;
}

//void Struct::analyze()
//{
    //if (size_ != NOT_ANALYZED)
        //return; // has already been analyzed

//}

} // namespace me
