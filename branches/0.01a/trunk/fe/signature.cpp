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

#include "fe/signature.h"

#include <sstream>

#include "utils/assert.h"

#include "fe/auto.h"
#include "fe/class.h"
#include "fe/memberfunction.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/type.h"
#include "fe/var.h"

#include "me/functab.h"

namespace swift {

//------------------------------------------------------------------------------

/*
 * destructor
 */

Signature::~Signature()
{
    for (size_t i = 0; i < inParams_.size(); ++i)
        delete inParams_[i];

    for (size_t i = 0; i < outParams_.size(); ++i)
        delete outParams_[i];
}

/*
 * further methods
 */

bool Signature::analyze() const
{
    bool result = true;

    // check each ingoing param
    for (size_t i = 0; i < inParams_.size(); ++i)
        result &= inParams_[i]->validateAndCreateVar();

    // check each outgoing param/result
    for (size_t i = 0; i < outParams_.size(); ++i)
        result &= outParams_[i]->validateAndCreateVar();

    return result;
}

bool Signature::checkIn(const TypeList& in) const
{
    // if the sizes do not match the Signature is obviously different
    if ( inTypes_.size() != in.size() )
        return false;

    // assume a true result in the beginning
    bool result = true;

    // check each param
    for (size_t i = 0; result && i < inTypes_.size(); ++i)
        result = inTypes_[i]->check( in[i] );

    return result;
}

bool Signature::checkOut(const TypeList& out) const
{
    // if the sizes do not match the Signature is obviously different
    if ( outTypes_.size() != out.size() )
        return false;

    // assume a true result in the beginning
    bool result = true;

    // check each result
    for (size_t i = 0; result && i < outTypes_.size(); ++i)
        result = outTypes_[i]->check( out[i] );

    return result;
}

bool Signature::check(const TypeList& in, const TypeList& out) const
{
    return checkIn(in) && checkOut(out);
}

bool Signature::check(const Signature* sig)
{
    return check( sig->getIn(), sig->getOut() );
}

// TODO remove copy&paste code

const Param* Signature::findParam(const std::string* id) const
{
    // is it an ingoing param?
    for (size_t i = 0; i < inParams_.size(); ++i)
    {
        if ( *inParams_[i]->id_ == *id)
            return inParams_[i];
    }

    // is it an outgoing param?
    for (size_t i = 0; i < outParams_.size(); ++i)
    {
        if ( *outParams_[i]->id_ == *id)
            return outParams_[i];
    }

    // -> not found, so return 0
    return 0;
}

Param* Signature::findParam(const std::string* id)
{
    // is it an ingoing param?
    for (size_t i = 0; i < inParams_.size(); ++i)
    {
        if ( *inParams_[i]->id_ == *id)
            return inParams_[i];
    }

    // is it an outgoing param?
    for (size_t i = 0; i < outParams_.size(); ++i)
    {
        if ( *outParams_[i]->id_ == *id)
            return outParams_[i];
    }

    // -> not found, so return 0
    return 0;
}

std::string Signature::toString() const
{
    std::ostringstream oss;
    oss << "TODO";

    return oss.str();
}

void Signature::appendInParam(InParam* param)
{
    inParams_.push_back(param);
    inTypes_.push_back( param->getType() );
}

void Signature::appendOutParam(OutParam* param)
{
    outParams_.push_back(param);
    outTypes_.push_back( param->getType() );
}

size_t Signature::getNumIn() const
{
    swiftAssert( inParams_.size() == inTypes_.size(), "sizes must match here" );
    return inTypes_.size();
}

size_t Signature::getNumOut() const
{
    swiftAssert( outParams_.size() == outTypes_.size(), "sizes must match here" );
    return outTypes_.size();
}

Param* Signature::getInParam(size_t i)
{
    swiftAssert(i < inParams_.size(), "index out ouf bounds" );
    return inParams_[i];
}

Param* Signature::getOutParam(size_t i)
{
    swiftAssert(i < outParams_.size(), "index out ouf bounds" );
    return outParams_[i];
}

const TypeList& Signature::getIn() const
{
    return inTypes_;
}

const TypeList& Signature::getOut() const
{
    return outTypes_;
}

std::string Signature::getMeId() const
{
    return meId_;
}

void Signature::setMeId(const std::string& meId)
{
    meId_ = meId;
}

} // namespace swift
