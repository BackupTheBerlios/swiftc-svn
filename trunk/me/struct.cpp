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

#include "me/arch.h"

namespace me {

/*
 * globals
 */

int Struct::nameCounter_ = 0;

//------------------------------------------------------------------------------

#ifdef SWIFT_DEBUG

Member::Member(Struct* parent, Struct* _struct, const std::string& id)
    : parent_(parent)
    , nr_(parent->memberNameCounter_++)
    , struct_(_struct)
    , simpleType_(false)
    , id_(id)
{}

Member::Member(Struct* parent, Op::Type type, const std::string& id)
    : parent_(parent)
    , nr_(parent->memberNameCounter_++)
    , type_(type)
    , simpleType_(true)
    , id_(id)
{}

#else // SWIFT_DEBUG

Member::Member(Struct* parent, Struct* _struct)
    : parent_(parent)
    , nr_(parent->memberNameCounter_++)
    , struct_(_struct)
    , simpleType_(false)
{}

Member::Member(Struct* parent, Op::Type type)
    : parent_(parent)
    , nr_(parent->memberNameCounter_++)
    , type_(type)
    , simpleType_(true)
{}

#endif // SWIFT_DEBUG

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

#ifdef SWIFT_DEBUG

Struct::Struct(const std::string& id)
    : nr_(nameCounter_++)
    , memberNameCounter_(0)
    , size_(NOT_ANALYZED)
    , id_(id)
{}

#else // SWIFT_DEBUG

Struct::Struct()
    : nr_(nameCounter_++)
    , memberNameCounter_(0)
    , size_(NOT_ANALYZED)
{}

#endif // SWIFT_DEBUG

Struct::~Struct()
{
    for (MemberMap::iterator iter = members_.begin(); iter != members_.end(); ++iter)
        delete iter->second;
}

/*
 * further methods
 */

#ifdef SWIFT_DEBUG

Member* Struct::append(Op::Type type, const std::string& id)
{
    Member* member = new Member(this, type, id);
    members_[member->nr_] = member;

    return member;
}

Member* Struct::append(Struct* _struct, const std::string& id)
{
    Member* member = new Member(this, _struct, id);
    members_[member->nr_] = member;

    return member;
}

#else // SWIFT_DEBUG

Member* Struct::append(Op::Type type)
{
    Member* member = new Member(this, type);
    members_[member->nr_] = member;

    return member;
}

Member* Struct::append(Struct* _struct)
{
    Member* member = new Member(this, _struct);
    members_[member->nr_] = member;

    return member;
}

#endif // SWIFT_DEBUG

Member* Struct::lookup(int nr)
{
    MemberMap::iterator iter = members_.find(nr);

    if ( iter == members_.end() )
        return 0;
    else
        return iter->second;
}

void Struct::analyze()
{
    if (size_ != NOT_ANALYZED)
        return; // has already been analyzed

    // for each member
    for (MemberMap::iterator iter = members_.begin(); iter != members_.end(); ++iter)
    {
        Member* member = iter->second;

        if ( member->simpleType_ )
        {
            switch (member->type_)
            {
                case me::Op::R_BOOL: 
                case me::Op::R_INT8:
                case me::Op::R_UINT8:
                case me::Op::R_SAT8:
                case me::Op::R_USAT8:
                    member->size_ = 1;
                    break;

                case me::Op::R_INT16:
                case me::Op::R_UINT16:
                case me::Op::R_SAT16:
                case me::Op::R_USAT16:
                    member->size_ = 2;
                    break;

                case me::Op::R_INT32:
                case me::Op::R_UINT32:
                case me::Op::R_REAL32:
                    member->size_ = 4;
                    break;

                case me::Op::R_INT64:
                case me::Op::R_UINT64:
                case me::Op::R_REAL64:
                    member->size_ = 8;
                    break;

                case me::Op::R_PTR:
                    member->size_ = arch->getPtrSize();
                    break;

                case me::Op::R_STACK:
                case me::Op::R_SPECIAL:
                    swiftAssert(false, "illegal switch-case-value");
                    break;
            }
        }
        else
        {
            member->struct_->analyze();
            member->size_ = member->struct_->size_;
        }

        member->offset_ = size_;
        size_ += member->size_;
    }
}

} // namespace me
