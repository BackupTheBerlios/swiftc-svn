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
#include "me/vectorizer.h"

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
    return type_ & SIMD_TYPES;
}

bool Op::isSimd(Type type) 
{
    return type & SIMD_TYPES;
}

Reg* Op::isReg(int typeMask)
{
    return 0;
}

Reg* Op::isReg(int typeMask, bool spilled)
{
    return 0;
}

Reg* Op::isNotSpilled(int /*typeMask*/)
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
            swiftAssert(type & SIMD_TYPES, "must a an simd type here");
            return arch->getSimdWidth();
    }

    swiftAssert(false, "unreachable code");
    return -1;
}

Op::Type Op::toSimd(Type type)
{
    if (type == R_PTR)
        return R_PTR;

    swiftAssert(type & VECTORIZABLE, "type not vectorizable");
    return (Type) (type << SIMD_OFFSET);
}

//------------------------------------------------------------------------------

Undef* Undef::toSimd(Vectorizer* vectorizer) const
{
    return vectorizer->simdFunction_->newUndef( Op::toSimd(type_) );
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Const::Const(Type type, size_t numBoxElems /*= 1*/)
    : Op(type)
    , numBoxElems_(numBoxElems)
{
    for (size_t i = 0; i < numBoxElems_; ++i)
        boxes_[i].uint64_ = 0; // clear all bits
}

/*
 * virtual methods
 */

Const* Const::toSimd(Vectorizer* vectorizer) const
{
    swiftAssert( !isSimd(), "must not be an simd type" );
    int simdLength = vectorizer->getSimdLength();
    Const* cst = vectorizer->simdFunction_->newConst( Op::toSimd(type_), simdLength );

    // broadcast original constant
    cst->broadcast(boxes_[0]);

    return cst;
}

std::string Const::toString() const
{
    std::ostringstream oss;

    for (size_t i = 0; i < numBoxElems_; ++i)
    {
        switch (type_)
        {
            case S_INT8:
            case R_INT8:   oss << int(boxes_[i].int8_)  << "b";   break;
            case S_INT16:
            case R_INT16:  oss << boxes_[i].int16_      << "w";   break;
            case S_INT32:
            case R_INT32:  oss << boxes_[i].int32_      << "d";   break;
            case S_INT64:
            case R_INT64:  oss << boxes_[i].int64_      << "q";   break;
            case S_SAT8:
            case R_SAT8:   oss << int(boxes_[i].int8_)  << "sb";  break;
            case S_SAT16:
            case R_SAT16:  oss << boxes_[i].int16_      << "sw";  break;

            case S_UINT8:
            case R_UINT8:  oss << int(boxes_[i].uint8_) << "ub";  break;
            case S_UINT16:
            case R_UINT16: oss << boxes_[i].uint16_     << "uw";  break;
            case S_UINT32:
            case R_UINT32: oss << boxes_[i].uint32_     << "ud";  break;
            case S_UINT64:
            case R_UINT64: oss << boxes_[i].uint64_     << "uq";  break;
            case S_USAT8:
            case R_USAT8:  oss << int(boxes_[i].uint8_) << "usb"; break;
            case S_USAT16:
            case R_USAT16: oss << boxes_[i].uint16_     << "usw"; break;
            case R_BOOL:
                if (boxes_[i].bool_)
                    oss << "true";
                else
                    oss << "false";
                break;
            case S_REAL32:
            case R_REAL32:
                oss << boxes_[i].float_;
                if ( fmod(boxes_[i].float_, 1.0) == 0.0 )
                    oss << ".d";
                else
                    oss << "d";
                break;
            case S_REAL64:
            case R_REAL64:
                oss << boxes_[i].double_;
                if ( fmod(boxes_[i].double_, 1.0) == 0.0 )
                    oss << ".q";
                else
                    oss << "q"; // FIXME
                break;
            default:
                swiftAssert(false, "unreachable code");
        }

        if (i != numBoxElems_ - 1)
            oss << '|';
    }

    return oss.str();
}

/*
 * further methods
 */

Box& Const::box()
{
    return boxes_[0];
}

const Box& Const::box() const
{
    return boxes_[0];
}

void Const::broadcast(const Box& box)
{
    for (size_t i = 0; i < numBoxElems_; ++i)
        boxes_[i] = box;
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

Var* Var::toSimd(Vectorizer* vectorizer) const
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

Reg* Reg::isReg(int typeMask, bool spilled)
{
    return ( !(spilled ^ isSpilled_) && typeCheck(typeMask) ) ? this : 0;
}

Reg* Reg::isSpilled()
{
    return isSpilled_ ? this : 0;
}

Reg* Reg::isSpilled(int typeMask)
{
    return ( isSpilled_ && typeCheck(typeMask) ) ? this : 0;
}

Reg* Reg::isNotSpilled(int typeMask)
{
    return ( !isSpilled_ && typeCheck(typeMask) ) ? this : 0;
}

Reg* Reg::toSimd(Vectorizer* vectorizer) const
{
    // is it already created?
    Vectorizer::Nr2Nr::iterator iter = vectorizer->src2dstNr_.find(varNr_);

    if ( iter != vectorizer->src2dstNr_.end() )
    {
        // yes -> return the already created one
        swiftAssert( vectorizer->simdFunction_->vars_.contains(iter->second),
                "must be found here" );
        return (Reg*) vectorizer->simdFunction_->vars_[iter->second];
    }
    // else

#ifdef SWIFT_DEBUG
    std::string simdStr = "s_" + id_;
    Reg* reg = vectorizer->simdFunction_->newSSAReg( Op::toSimd(type_), &simdStr );
#else // SWIFT_DEBUG
    Reg* reg = vectorizer->simdFunction_->newSSAReg( Op::toSimd(type_) );
#endif // SWIFT_DEBUG

    vectorizer->src2dstNr_[varNr_] = reg->varNr_;

    return reg;
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

/*
 * further methods
 */

bool Reg::isColorAdmissible(int color)
{
    // find definition
    InstrBase* def = def_.instrNode_->value_;
    for (size_t i = 0; i < def->res_.size(); ++i)
    {
        Res& res = def->res_[i];
        if (res.var_ == this)
        {
            // -> found
            if (res.constraint_ != NO_CONSTRAINT && res.constraint_ != color)
                return false;
            else
            break;
        }
    }

    DEFUSELIST_EACH(iter, uses_)
    {
        InstrBase* use = iter->value_.instrNode_->value_;
        for (size_t i = 0; i < use->arg_.size(); ++i)
        {
            Arg& arg = use->arg_[i];
            if (arg.op_ == this)
            {
                // -> found
                if (arg.constraint_ != NO_CONSTRAINT && arg.constraint_ != color)
                    return false;
                else
                    goto outer_loop;
            }
        }

outer_loop:
        continue;
    }

    return true;
}

int Reg::getPreferedColor() const
{
    AssignInstr* ai = dynamic_cast<AssignInstr*>(def_.instrNode_->value_);
    if (ai)
    {
        Reg* arg = dynamic_cast<Reg*>( ai->arg_[0].op_ );
        if (arg)
            return arg->color_;
    }

    return color_;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

#ifdef SWIFT_DEBUG

MemVar::MemVar(Aggregate* aggregate, int varNr, const std::string* id /*= 0*/)
    : Var(R_MEM, varNr, id)
    , aggregate_(aggregate)
{}

MemVar* MemVar::clone(int varNr) const
{
    return new MemVar(aggregate_, varNr, &id_);
}

#else // SWIFT_DEBUG

MemVar::MemVar(Aggregate* aggregate, int varNr)
    : Var(R_MEM, varNr)
    , aggregate_(aggregate)
{}

MemVar* MemVar::clone(int varNr) const
{
    return new MemVar(aggregate_, varNr);
}

#endif // SWIFT_DEBUG

MemVar* MemVar::toSimd(Vectorizer* vectorizer) const
{
    // is it already created?
    Vectorizer::Nr2Nr::iterator iter = vectorizer->src2dstNr_.find(varNr_);

    if ( iter != vectorizer->src2dstNr_.end() )
    {
        // yes -> return the already created one
        swiftAssert( vectorizer->simdFunction_->vars_.contains(iter->second),
                "must be found here" );
        return (MemVar*) vectorizer->simdFunction_->vars_[iter->second];
    }
    // else

#ifdef SWIFT_DEBUG
    std::string simdStr = "s_" + id_;
    //MemVar* memVar = vectorizer->simdFunction_->newSSAMemVar( aggregate->toSimd(), &simdStr );
#else // SWIFT_DEBUG
    //MemVar* memVar = vectorizer->simdFunction_->newSSAMemVar( aggregate->toSimd() );
#endif // SWIFT_DEBUG
    MemVar* memVar = 0;
    swiftAssert(false, "TODO");

    vectorizer->src2dstNr_[varNr_] = memVar->varNr_;

    return memVar;
}

std::string MemVar::toString() const
{
    std::ostringstream oss;
    oss << Var::toString();
    oss << " (" << color_ << ')';

    return oss.str();
}

} // namespace me
