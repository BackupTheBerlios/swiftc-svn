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

#include "fe/class.h"
#include "fe/method.h"
#include "fe/parser.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/var.h"

#include "me/functab.h"

namespace swift {

//------------------------------------------------------------------------------

/*
 * destructor
 */

Signature::~Signature()
{
    for (size_t i = 0; i < in_.size(); ++i)
        delete in_[i];

    for (size_t i = 0; i < out_.size(); ++i)
        delete out_[i];
}

/*
 * further methods
 */

bool Signature::analyze() const
{
    bool result = true;

    // check each ingoing param
    for (size_t i = 0; i < in_.size(); ++i)
        result &= in_[i]->validateAndCreateVar();

    // check each outgoing param/result
    for (size_t i = 0; i < out_.size(); ++i)
        result &= out_[i]->validateAndCreateVar();

    return result;
}

bool Signature::checkIngoing(const Signature* sig) const
{
    // if the sizes do not match the Signature is obviously different
    if ( in_.size() != sig->in_.size() )
        return false;

    // assume a true result in the beginning
    bool result = true;

    // check each param
    for (size_t i = 0; result && i < in_.size(); ++i)
        result = Param::check(in_[i], sig->in_[i]);

    return result;
}

bool Signature::check(const Signature* sig)
{
    bool result = checkIngoing(sig);

    if (!result)
        return false;

    /*
     * now check the outgoing part
     */

    // if the sizes do not match the Signature is obviously different
    if ( out_.size() != sig->out_.size() )
        return false;

    // check each result
    for (size_t i = 0; result && i < out_.size(); ++i)
        result = Param::check(out_[i], sig->out_[i]);

    return result;
}

// TODO remove copy&paste code

const Param* Signature::findParam(const std::string* id) const
{
    // is it an ingoing param?
    for (size_t i = 0; i < in_.size(); ++i)
    {
        if ( *in_[i]->id_ == *id)
            return in_[i];
    }

    // is it an outgoing param?
    for (size_t i = 0; i < out_.size(); ++i)
    {
        if ( *out_[i]->id_ == *id)
            return out_[i];
    }

    // -> not found, so return 0
    return 0;
}

Param* Signature::findParam(const std::string* id)
{
    // is it an ingoing param?
    for (size_t i = 0; i < in_.size(); ++i)
    {
        if ( *in_[i]->id_ == *id)
            return in_[i];
    }

    // is it an outgoing param?
    for (size_t i = 0; i < out_.size(); ++i)
    {
        if ( *out_[i]->id_ == *id)
            return out_[i];
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

void Signature::appendInParam(Param* param)
{
    swiftAssert(param->getKind() == Param::ARG || param->getKind() == Param::ARG_INOUT,
            "must be marked as ARG or ARG_INOUT");
    in_.push_back(param);
}

void Signature::appendOutParam(Param* param)
{
    swiftAssert(param->getKind() == Param::RES, "must be marked as ARG or ARG_INOUT");
    out_.push_back(param);
}

size_t Signature::getNumIn() const
{
    return in_.size();
}

size_t Signature::getNumOut() const
{
    return out_.size();
}

Param* Signature::getIn(size_t i)
{
    swiftAssert(i < in_.size(), "index out ouf bounds");
    return in_[i];
}

Param* Signature::getOut(size_t i)
{
    swiftAssert(i < out_.size(), "index out ouf bounds");
    return out_[i];
}

} // namespace swift
