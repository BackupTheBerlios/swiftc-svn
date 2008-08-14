#include "me/op.h"

#include <iostream>
#include <cmath> // for fmod

#include "utils/stringhelper.h"

namespace me {

//------------------------------------------------------------------------------

std::string Literal::toString() const
{
    std::ostringstream oss;

    switch (type_)
    {
        case R_INDEX:   oss << value_.index_        << "x";     break;

        case R_INT:     oss << value_.int_;                     break;
        case R_INT8:    oss << int(value_.int8_)    << "b";     break;
        case R_INT16:   oss << value_.int16_        << "w";     break;
        case R_INT32:   oss << value_.int32_        << "d";     break;
        case R_INT64:   oss << value_.int64_        << "q";     break;
        case R_SAT8:    oss << int(value_.sat8_)    << "sb";    break;
        case R_SAT16:   oss << value_.sat16_        << "sw";    break;

        case R_UINT:    oss << value_.uint_;                    break;
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

        // hence it is real, real32 or real64

        case R_REAL:
            oss << value_.real_;
        break;
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
                std::cout << "TODO" << fmod(value_.real64_, 1.0) << std::endl;
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

Reg::Reg(Type type, int varNr, std::string* id /*= 0*/)
    : Op(type)
    , varNr_(varNr)
    , color_(NOT_COLORED_YET)
    , id_( id ? *id : "")
{}

Reg* Reg::createMem(Type type, int varNr, std::string* id /*= 0*/)
{
    Reg* reg = new Reg(type, varNr, id);
    reg->color_ = MEMORY_LOCATION;

    return reg;
}

#else // SWIFT_DEBUG

Reg::Reg(Type type, int varNr)
    : Op(type)
    , varNr_(varNr)
    , color_(NOT_COLORED_YET)
{}

// TODO is this used???
Reg* Reg::createMem(Type type, int varNr)
{
    Reg* reg = new Reg(type, varNr);
    reg->color_ = MEMORY_LOCATION;

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

bool Reg::isColored() const
{
    swiftAssert(color_ != MEMORY_LOCATION, "this Reg is a mem");
    return color_ == NOT_COLORED_YET;
}

bool Reg::isMem() const
{
    return color_ == MEMORY_LOCATION;
}

std::string Reg::toString() const
{
    std::ostringstream oss;

    if ( !isSSA() )
    {
#ifdef SWIFT_DEBUG
        oss << id_;
#else
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
#else
        oss << "tmp";
#endif // SWIFT_DEBUG
        oss << number2String( varNr_);
    }

    return oss.str();
}

} // namespace me
