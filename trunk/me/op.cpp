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

#include "me/op.h"

#include <iostream>
#include <cmath> // for fmod

#include "utils/stringhelper.h"

#include "me/arch.h"

namespace me {

//------------------------------------------------------------------------------

bool Op::typeCheck(int typeMask) const
{
    return type_ & typeMask;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */
Const::Const(Type type)
    : Op(type)
{
    value_.uint64_ = 0; // clear all bits
}

/*
 * further methods
 */

std::string Const::toString() const
{
    std::ostringstream oss;

    switch (type_)
    {
        case R_INT8:    oss << int(value_.int8_)    << "b";     break;
        case R_INT16:   oss << value_.int16_        << "w";     break;
        case R_INT32:   oss << value_.int32_        << "d";     break;
        case R_INT64:   oss << value_.int64_        << "q";     break;
        case R_SAT8:    oss << int(value_.sat8_)    << "sb";    break;
        case R_SAT16:   oss << value_.sat16_        << "sw";    break;

        case R_UINT8:   oss << int(value_.uint8_)   << "ub";    break;
        case R_UINT16:  oss << value_.uint16_       << "uw";    break;
        case R_UINT32:  oss << value_.uint32_       << "ud";    break;
        case R_UINT64:  oss << value_.uint64_       << "uq";    break;
        case R_USAT8:   oss << int(value_.usat8_)   << "usb";   break;
        case R_USAT16:  oss << value_.usat16_       << "usw";   break;

        case R_BOOL:
            if (value_.bool_)
                oss << "true";
            else
                oss << "false";
            break;

        // -> hence it is real32 or real64

        case R_REAL32:
            oss << value_.real32_;
            if ( fmod(value_.real32_, 1.0) == 0.0 )
                oss << ".d";
            else
                oss << "d";
            break;
        case R_REAL64:
            oss << value_.real64_;
            if ( fmod(value_.real64_, 1.0) == 0.0 )
                oss << ".q";
            else
                oss << "q"; // FIXME
            break;
        default:
            swiftAssert(false, "VR_* not implemented yet");
    }

    return oss.str();
}

//------------------------------------------------------------------------------

std::string Undef::toString() const {
    return "UNDEF";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

#ifdef SWIFT_DEBUG

Reg::Reg(Type type, int varNr, const std::string* id /*= 0*/)
    : Op(type)
    , varNr_(varNr)
    , color_(NOT_COLORED_YET)
    , isSpilled_(false)
    , id_( id ? *id : "")
{}

Reg* Reg::clone() const
{
    // TODO make use of it
    Reg* reg = new Reg(type_, varNr_, &id_);
    reg->isSpilled_ = isSpilled_;
    return reg;
}

#else // SWIFT_DEBUG

Reg::Reg(Type type, int varNr)
    : Op(type)
    , varNr_(varNr)
    , color_(NOT_COLORED_YET)
    , isSpilled_(false)
{}

Reg* Reg::clone() const
{
    // TODO make use of it
    Reg* reg = new Reg(type_, varNr_);
    :cc
    return reg;
}

#endif // SWIFT_DEBUG

/*
 * further methods
 */

bool Reg::isSSA() const
{
    return varNr_ >= 0;
}

size_t Reg::var2Index() const
{
    swiftAssert(varNr_ < 0, "this is not a var");

    return size_t(-varNr_);
}

bool Reg::isSpilled() const
{
    return isSpilled_;
}

bool Reg::colorReg(int typeMask) const
{
    return ( !isSpilled() && typeCheck(typeMask) );
}

std::string Reg::toString() const
{
    std::ostringstream oss;

    if ( isSpilled() )
        oss << "@";

    if ( !isSSA() )
    {
#ifdef SWIFT_DEBUG
        oss << id_;
#else // SWIFT_DEBUG
        oss << "tmp";
#endif // SWIFT_DEBUG
        // _!_ indicates that this varNr_ is negative -> not SSA
        oss << "_!_" << number2String(-varNr_);
    }
    else
    {
        // -> it is SSA
#ifdef SWIFT_DEBUG
        if ( id_ == std::string("") )
            oss << "tmp";
        else
            oss << id_;
#else // SWIFT_DEBUG
        oss << "tmp";
#endif // SWIFT_DEBUG
        oss << number2String( varNr_);
    }

    if ( isSpilled() )
        oss << " (" << color_ << ')';
    else if ( color_ >= 0 )
        oss << " (" << me::arch->reg2String(this) << ')';

    return oss.str();
}

} // namespace me
