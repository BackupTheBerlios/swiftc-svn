#include "pseudoreg.h"

#include <iostream>
#include <cmath> // for fmod


PseudoReg::~PseudoReg()
{
    delete id_;
}

std::string PseudoReg::toString() const
{
    if (id_)
        return *id_;
    // else
    std::ostringstream oss;

    switch (regType_)
    {
        case R_INT:     oss << value_.int_;                    break;
        case R_INT8:    oss << int(value_.int8_)    << "b";     break;
        case R_INT16:   oss << value_.int16_        << "w";     break;
        case R_INT32:   oss << value_.int32_        << "d";     break;
        case R_INT64:   oss << value_.int64_        << "q";     break;
        case R_SAT8:    oss << int(value_.sat8_)    << "sb";    break;
        case R_SAT16:   oss << value_.sat16_        << "sw";    break;

        case R_UINT:    oss << value_.uint_;                   break;
        case R_UINT8:   oss << int(value_.uint8_)   << "ub";    break;
        case R_UINT16:  oss << value_.uint16_       << "uw";    break;
        case R_UINT32:  oss << value_.uint32_       << "ud";    break;
        case R_UINT64:  oss << value_.uint64_       << "uq";    break;
        case R_USAT8:   oss << int(value_.usat8_)   << "usb";   break;
        case R_USAT16:  oss << value_.usat16_       << "usw";   break;

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
                std::cout << "fjkdlfjkdjfdl" << fmod(value_.real64_, 1.0) << std::endl;
            break;
        default:
            swiftAssert(false, "VR_* not implemented yet");
    }

    return oss.str();
}
