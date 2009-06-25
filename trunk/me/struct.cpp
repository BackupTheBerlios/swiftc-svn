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

int Member::sizeOf() const
{
    swiftAssert( alreadyAnalyzed(), "not analyzed yet" );
    return size_;
}

int Member::getNr() const
{
    return nr_;
}

int Member::getOffset() const
{
    return offset_;
}

void Member::setOffset(int offset)
{
    offset_ = offset;
}

#ifdef SWIFT_DEBUG

std::string Member::getId() const
{
    return id_;
}

#endif // SWIFT_DEBUG

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
    size_ = Op::sizeOf(type_);
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
    size_ = Op::sizeOf(type_) * num_;
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
    swiftAssert( !memberMap_.contains(member->getNr()), "already inserted" );

    members_.push_back(member);
    memberMap_[ member->getNr() ] = member;
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

        member->setOffset( arch->calcAlignedOffset(size_, member->sizeOf()) );
        size_ = member->getOffset() + member->sizeOf();
    }
}

void Struct::destroyNonStructMembers()
{
    for (size_t i = 0; i < members_.size(); ++i)
    {
        if ( typeid(*members_[i]) != typeid(Struct) )
            delete members_[i];
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
        oss << '\t' << member->getId() << '\n';
        oss << "\t\tsize: " << member->sizeOf() << '\n';
        oss << "\t\toffset: " << member->getOffset() << '\n';
    }

    return oss.str();
}

#endif // SWIFT_DEBUG

} // namespace me
