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
#include <cmath>

#include "utils/stringhelper.h"

#include "me/arch.h"
#include "me/functab.h"

namespace me {

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Op::Op(Type type)
    : type_(type)
{}

Op::~Op() 
{}

/*
 * further methods
 */

bool Op::typeCheck(int typeMask) const
{
    return type_ & typeMask;
}

bool Op::isReal(Type type)
{
    return (type == me::Op::R_REAL32 || type == me::Op::R_REAL64);
}

bool Op::isReal() const
{
    return isReal(type_);
}

bool Op::isSimd() const
{
    return type_ & SIMD;
}

Reg* Op::isReg(int typeMask)
{
    return 0;
}

Reg* Op::colorReg(int /*typeMask*/)
{
    return 0;
}

Reg* Op::isSpilled()
{
    return 0;
}

Reg* Op::isSpilled(int /*typeMask*/)
{
    return 0;
}

/*
 * static methods
 */

int Op::sizeOf(Type type)
{
    switch (type)
    {
        case R_BOOL: // TODO really?
        case R_INT8:
        case R_SAT8:
        case R_UINT8:
        case R_USAT8:
            return 1;

        case R_INT16:
        case R_SAT16:
        case R_UINT16:
        case R_USAT16:
            return 2;

        case R_INT32:
        case R_UINT32:
        case R_REAL32:
            return 4;

        case R_INT64:
        case R_UINT64:
        case R_REAL64:
            return 8;

        case R_PTR:
            return arch->getPtrSize();

        case R_MEM:
            swiftAssert(false, "unreachable code");
            return -1;

        default:
            swiftAssert(type & SIMD, "must a an simd type here");
            return arch->getSimdWidth();
    }

    swiftAssert(false, "unreachable code");
    return -1;
}

Op::Type Op::toSimd(Type type)
{
    swiftAssert(type & VECTORIZABLE, "type not vectorizable");
    return (Type) (type << SIMD_OFFSET);
}

//------------------------------------------------------------------------------

Undef* Undef::toSimd() const
{
    return new Undef( Op::toSimd(type_) );
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */
Const::Const(Type type)
    : Op(type)
{
    box_.uint64_ = 0; // clear all bits
}

/*
 * virtual methods
 */

Const* Const::toSimd() const
{
    Const* cst = new Const( Op::toSimd(type_) );
    cst->box_ = box_;

    return cst;
}

std::string Const::toString() const
{
    std::ostringstream oss;

    switch (type_)
    {
        case R_INT8:   oss << int(box_.int8_)  << "b";   break;
        case R_INT16:  oss << box_.int16_      << "w";   break;
        case R_INT32:  oss << box_.int32_      << "d";   break;
        case R_INT64:  oss << box_.int64_      << "q";   break;
        case R_SAT8:   oss << int(box_.int8_)  << "sb";  break;
        case R_SAT16:  oss << box_.int16_      << "sw";  break;

        case R_UINT8:  oss << int(box_.uint8_) << "ub";  break;
        case R_UINT16: oss << box_.uint16_     << "uw";  break;
        case R_UINT32: oss << box_.uint32_     << "ud";  break;
        case R_UINT64: oss << box_.uint64_     << "uq";  break;
        case R_USAT8:  oss << int(box_.uint8_) << "usb"; break;
        case R_USAT16: oss << box_.uint16_     << "usw"; break;
        case R_BOOL:
            if (box_.bool_)
                oss << "true";
            else
                oss << "false";
            break;
        case R_REAL32:
            oss << box_.float_;
            if ( fmod(box_.float_, 1.0) == 0.0 )
                oss << ".d";
            else
                oss << "d";
            break;
        case R_REAL64:
            oss << box_.double_;
            if ( fmod(box_.double_, 1.0) == 0.0 )
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

Var::Var(Type type, int varNr, const std::string* id /*= 0*/)
    : Op(type)
    , varNr_(varNr)
    , color_(NOT_COLORED_YET)
    , id_( id ? *id : "")
{}

#else // SWIFT_DEBUG

Var::Var(Type type, int varNr)
    : Op(type)
    , varNr_(varNr)
    , color_(NOT_COLORED_YET)
{}

#endif // SWIFT_DEBUG

/*
 * further methods
 */

bool Var::isSSA() const
{
    return varNr_ >= 0;
}

bool Var::dontColor() const
{
    return color_ == DONT_COLOR;
}

size_t Var::var2Index() const
{
    swiftAssert(varNr_ < 0, "this is not a var");

    return size_t(-varNr_);
}

bool Var::typeCheck(int typeMask) const
{
    return !dontColor() && Op::typeCheck(typeMask);
}

Var* Var::toSimd() const
{
    swiftAssert(false, "unreachable code");
    return 0;
}

std::string Var::toString() const
{
    std::ostringstream oss;

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

    return oss.str();
}


//------------------------------------------------------------------------------

/*
 * constructor
 */

#ifdef SWIFT_DEBUG

Reg::Reg(Type type, int varNr, const std::string* id /*= 0*/)
    : Var(type, varNr, id)
    , isSpilled_(false)
{
    swiftAssert(type_ != R_MEM, "Use a MemVar for this type");
}

Reg* Reg::clone(int varNr) const
{
    Reg* reg = new Reg(type_, varNr, &id_);
    reg->isSpilled_ = isSpilled_;
    return reg;
}

#else // SWIFT_DEBUG

Reg::Reg(Type type, int varNr)
    : Var(type, varNr)
    , isSpilled_(false)
{}

Reg* Reg::clone(int varNr) const
{
    Reg* reg = new Reg(type_, varNr);
    reg->isSpilled_ = isSpilled_;
    return reg;
}

#endif // SWIFT_DEBUG

/*
 * further methods
 */

Reg* Reg::isReg(int typeMask)
{
    return typeCheck(typeMask) ? this : 0;
}

Reg* Reg::isSpilled()
{
    return isSpilled_ ? this : 0;
}

Reg* Reg::isSpilled(int typeMask)
{
    return ( isSpilled_ && typeCheck(typeMask) ) ? this : 0;
}

Reg* Reg::colorReg(int typeMask)
{
    return ( !isSpilled() && typeCheck(typeMask) ) ? this : 0;
}

Reg* Reg::toSimd() const
{
#ifdef SWIFT_DEBUG
    return functab->newSSAReg( Op::toSimd(type_), new std::string("s_" + id_) );
#else // SWIFT_DEBUG
    return functab->newSSAReg( Op::toSimd(type_) );
#endif // SWIFT_DEBUG
}

std::string Reg::toString() const
{
    std::ostringstream oss;
    
    if ( isSpilled_ )
        oss << '@';

    oss << Var::toString();

    if ( isSpilled_ )
        oss << " (" << color_ << ')';
    else if ( color_ >= 0 )
        oss << " (" << me::arch->reg2String(this) << ')';

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

#ifdef SWIFT_DEBUG

MemVar::MemVar(Member* memory, int varNr, const std::string* id /*= 0*/)
    : Var(R_MEM, varNr, id)
    , memory_(memory)
{}

MemVar* MemVar::clone(int varNr) const
{
    return new MemVar(memory_, varNr, &id_);
}

#else // SWIFT_DEBUG

MemVar::MemVar(Member* memory, int varNr)
    : Var(R_MEM, varNr)
    , memory_(memory)
{}

MemVar* MemVar::clone(int varNr) const
{
    return new MemVar(memory_, varNr);
}

#endif // SWIFT_DEBUG

MemVar* MemVar::toSimd() const
{
#ifdef SWIFT_DEBUG
    //return functab->newSSAMemVar( memory->toSimd(), new std::string("s_" + id_) );
#else // SWIFT_DEBUG
    //return functab->newSSAMemVar( memory->toSimd() );
#endif // SWIFT_DEBUG
    return 0;
}

std::string MemVar::toString() const
{
    std::ostringstream oss;
    oss << Var::toString();
    oss << " (" << color_ << ')';

    return oss.str();
}

} // namespace me
