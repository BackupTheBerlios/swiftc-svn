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
#include <iostream>
#include <typeinfo>

#include "me/arch.h"

/*
 * TODO Cyclic dependencies between structs are not recognized yet. 
 * This results in an endless recursion.
 *
 * TODO empty structs are not handled correctly
 */

//------------------------------------------------------------------------------

namespace me {

/*
 * constructor 
 */

#ifdef SWIFT_DEBUG

Member::Member(Aggregate* aggregate, const std::string& id)
    : aggregate_(aggregate)
    , id_(id)
    , offset_(0)
{}

#else // SWIFT_DEBUG

Member::Member(Aggregate* aggregate)
    : aggregate_(aggregate)
    , offset_(0)
    , vectorized_(0)
{}

#endif // SWIFT_DEBUG

Member::~Member()
{
    delete aggregate_;
}

/*
 * further methods
 */

Member* Member::vectorize(int& simdLength)
{
#ifdef SWIFT_DEBUG
    vectorized_ = new Member( aggregate_->vectorize(simdLength), "simd_" + id_ );
#else //  SWIFT_DEBUG
    vectorized_ = new Member( aggregate_->vectorize(simdLength) );
#endif // SWIFT_DEBUG

    return vectorized_;
}

int Member::getOffset() const
{
    return offset_;
}

void Member::setOffset(int offset)
{
    offset_ = offset;
}

Aggregate* Member::aggregate()
{
    return aggregate_;
}

const Aggregate* Member::aggregate() const
{
    return aggregate_;
}

#ifdef SWIFT_DEBUG

std::string Member::getId() const
{
    return id_;
}

#endif // SWIFT_DEBUG

std::string Member::toString() const
{
    std::ostringstream oss;
#ifdef SWIFT_DEBUG
    oss << id_ << " - ";
#endif // SWIFT_DEBUG
    oss << offset_;

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor 
 */

Aggregate::Aggregate()
    : size_(NOT_ANALYZED)
    , simdLength_(NON_SIMD)
    , vectorized_(0)
{}

/*
 * further methods
 */

bool Aggregate::alreadyAnalyzed() const
{
    return size_ != NOT_ANALYZED;
}

int Aggregate::sizeOf() const
{
    swiftAssert( alreadyAnalyzed(), "not analyzed yet" );
    return size_;
}

int Aggregate::getSimdLength() const
{
    return simdLength_;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

AtomicAggregate::AtomicAggregate(Op::Type type)
    : type_(type)
{}

/*
 * virtual methods
 */

void AtomicAggregate::analyze()
{
    size_ = Op::sizeOf(type_);
}

AtomicAggregate* AtomicAggregate::vectorize(int& simdLength)
{
    AtomicAggregate* aggregate = new AtomicAggregate( Op::toSimd(type_) );
    aggregate->analyze();
    aggregate->simdLength_ = arch->getSimdWidth() / size_;
    simdLength = aggregate->simdLength_;
    vectorized_ = aggregate;

    return aggregate;
}

//------------------------------------------------------------------------------

ArrayAggregate::ArrayAggregate(Op::Type type, size_t num)
    : type_(type)
    , num_(num)
{}

/*
 * virtual methods
 */

void ArrayAggregate::analyze()
{
    size_ = Op::sizeOf(type_) * num_;
}

ArrayAggregate* ArrayAggregate::vectorize(int& simdLength)
{
    ArrayAggregate* aggregate = new ArrayAggregate( Op::toSimd(type_), num_);
    aggregate->analyze();
    aggregate->simdLength_ = arch->getSimdWidth() / size_;
    simdLength = aggregate->simdLength_;
    vectorized_ = aggregate;

    return aggregate;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

#ifdef SWIFT_DEBUG

Struct::Struct(const std::string& id)
    : id_(id)
{}

#else // SWIFT_DEBUG

Struct::Struct()
{}

#endif // SWIFT_DEBUG

/*
 * virtual methods
 */

void Struct::analyze()
{
    size_ = 0;

    // for each member
    for (size_t i = 0; i < members_.size(); ++i)
    {
        Member* member = members_[i];

        if ( !member->aggregate()->alreadyAnalyzed() )
            member->aggregate()->analyze();

        member->setOffset( arch->calcAlignedOffset(size_, member->aggregate()->sizeOf()) );
        size_ = member->getOffset() + member->aggregate()->sizeOf();
    }
}

Struct* Struct::vectorize(int& simdLength)
{
#ifdef SWIFT_DEBUG
    Struct* aggregate = new Struct( "simd_" + id_ );
#else // SWIFT_DEBUG
    Struct* aggregate = new Struct();
#endif // SWIFT_DEBUG

    bool simdLengthSet = false;
    bool vectorizeable = true;

    // for each member
    for (size_t i = 0; i < members_.size(); ++i)
    {
        int innerSimdLength;
        Member* member = members_[i]->vectorize(innerSimdLength);

        if (!simdLengthSet)
        {
            aggregate->simdLength_ = innerSimdLength;
            simdLengthSet = true;
        }
        else
        {
            if (aggregate->simdLength_ != innerSimdLength)
                vectorizeable = false;
        }

        aggregate->members_.push_back(member);
    }

    if (!vectorizeable)
    {
        aggregate->simdLength_ = NON_SIMD;
        simdLength  = NON_SIMD;
    }
    else
        simdLength = aggregate->simdLength_;

    vectorized_ = aggregate;

    return aggregate;
}

/*
 * further methods
 */

#ifdef SWIFT_DEBUG

Member* Struct::append(Aggregate* aggregate, const std::string& id)
{
    Member* member = new Member(aggregate, id);
    members_.push_back(member);

    return member;
}

#else // SWIFT_DEBUG

Member* Struct::append(Aggregate* aggregate)
{
    Member* member = new Member(aggregate);
    members_.push_back(member);

    return member;
}

#endif // SWIFT_DEBUG

void Struct::destroyNonStructMembers()
{
    for (size_t i = 0; i < members_.size(); ++i)
    {
        if ( typeid(*members_[i]->aggregate()) != typeid(Struct) )
            delete members_[i];
    }
}

#ifdef SWIFT_DEBUG

std::string Struct::dump() const
{
    std::ostringstream oss;

    oss << "STRUCT " << id_ << '\n'
        << "size: " << size_ << '\n';

    // for each member
    for (size_t i = 0; i < members_.size(); ++i)
    {
        Member* member = members_[i];
        oss << '\t' << member->getId() << '\n';
        oss << "\t\tsize: " << member->aggregate()->sizeOf() << '\n';
        oss << "\t\toffset: " << member->getOffset() << '\n';
    }

    return oss.str();
}

#endif // SWIFT_DEBUG

std::string Struct::toString() const
{
    std::ostringstream oss;

#ifdef SWIFT_DEBUG
    oss << id_;
#else // SWIFT_DEBUG
    oss << "STRUCT[" << members_.size() << ']';
#endif // SWIFT_DEBUG

    return oss.str();
}

} // namespace me
