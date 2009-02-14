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

#include <sstream>
#include <typeinfo>

#include "me/arch.h"

/*
 * TODO Cyclic dependencies between structs are not recognized yet. 
 * This results in an endless recursion.
 *
 * TODO empty structs are not handled correctly
 */

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
    swiftAssert(size != 0, "size is zero");
    int result = offset;
    int align = me::arch->alignOf(size);
    int mod = result % align;

    // do we have to adjust the offset due to alignment?
    if (mod != 0)
        result += align - mod;

    return result;
}

} // namespace

//------------------------------------------------------------------------------

namespace me {

/*
 * constructor 
 */

#ifdef SWIFT_DEBUG

Member::Member(const std::string& id)
    : nr_(namecounter++)
    , size_(NOT_ANALYZED)
    , offset_(0)
    , id_(id)
{}

#else // SWIFT_DEBUG

Member::Member()
    : nr_(namecounter++)
    , size_(NOT_ANALYZED)
    , offset_(0)
{}

#endif // SWIFT_DEBUG

/*
 * further methods
 */

bool Member::alreadyAnalyzed() const
{
    return size_ != NOT_ANALYZED;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

#ifdef SWIFT_DEBUG

AtomicMember::AtomicMember(Op::Type type, const std::string& id)
    : Member(id)
    , type_(type)
{}

#else // SWIFT_DEBUG

AtomicMember::AtomicMember(Op::Type type)
    : type_(type)
{}

#endif // SWIFT_DEBUG

/*
 * further methods
 */

void AtomicMember::analyze()
{
    size_ = type2size(type_);
}

//------------------------------------------------------------------------------

#ifdef SWIFT_DEBUG

ArrayMember::ArrayMember(Op::Type type, size_t num, const std::string& id)
    : Member(id)
    , type_(type)
    , num_(num)
{}

#else // SWIFT_DEBUG

ArrayMember::ArrayMember(Op::Type type, size_t num)
    : type_(type)
    , num_(num)
{}

#endif // SWIFT_DEBUG

/*
 * further methods
 */

void ArrayMember::analyze()
{
    size_ = type2size(type_) * num_;
}
    
//------------------------------------------------------------------------------

/*
 * constructor
 */

#ifdef SWIFT_DEBUG

Struct::Struct(const std::string& id)
    : Member(id)
{}

#else // SWIFT_DEBUG

Struct::Struct()
{}

#endif // SWIFT_DEBUG

/*
 * further methods
 */

void Struct::append(Member* member)
{
    swiftAssert( memberMap_.find(member->nr_) == memberMap_.end(),
            "already inserted" );

    members_.push_back(member);
    memberMap_[member->nr_] = member;
}

Member* Struct::lookup(int nr)
{
    MemberMap::iterator iter = memberMap_.find(nr);

    if ( iter == memberMap_.end() )
        return 0;
    else
        return iter->second;
}

void Struct::analyze()
{
    size_ = 0;

    // for each member
    for (size_t i = 0; i < members_.size(); ++i)
    {
        Member* member = members_[i];

        if ( !member->alreadyAnalyzed() )
            member->analyze();

        member->offset_ = calcAlignedOffset(size_, member->size_);
        size_ = member->offset_ + member->size_;
    }
}

#ifdef SWIFT_DEBUG

std::string Struct::toString() const
{
    std::ostringstream oss;

    oss << "STRUCT " << id_ << '\n'
        << "size: " << size_ << '\n';

    // for each member
    for (size_t i = 0; i < members_.size(); ++i)
    {
        Member* member = members_[i];
        oss << '\t' << member->id_ << '\n';
        oss << "\t\tsize: " << member->size_ << '\n';
        oss << "\t\toffset: " << member->offset_ << '\n';
    }

    return oss.str();
}

#endif // SWIFT_DEBUG

} // namespace me
