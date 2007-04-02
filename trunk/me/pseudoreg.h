#ifndef SWIFT_PSEUDOREG_H
#define SWIFT_PSEUDOREG_H

#include <string>
#include <sstream>

#include "utils/list.h"

// forward declaration
struct Scope;

// -----------------------------------------------------------------------------

struct PseudoReg
{
    /// Types of pseudo registers / constants
    enum RegType
    {
        R_INDEX,
        R_INT,  R_INT8,  R_INT16,  R_INT32,  R_INT64,  R_SAT8,  R_SAT16,

        R_UINT, R_UINT8, R_UINT16, R_UINT32, R_UINT64, R_USAT8, R_USAT16,

        R_REAL, R_REAL32, R_REAL64,

        // SIMD registers use the same value but negative
        VR_INDEX = -R_INDEX,
        VR_INT   = -R_INT,
        VR_INT8  = -R_INT8,
        VR_INT16 = -R_INT16,
        VR_INT32 = -R_INT32,
        VR_INT64 = -R_INT64,

        VR_SAT8  = -R_SAT8,
        VR_SAT16 = -R_SAT16,

        VR_UINT  = -R_UINT,
        VR_UINT8 = -R_UINT8,
        VR_UINT16= -R_UINT16,
        VR_UINT32= -R_UINT32,
        VR_UINT64= -R_UINT64,

        VR_USAT8 = -R_USAT8,
        VR_USAT16= -R_USAT16,

        VR_REAL  = -R_REAL,
        VR_REAL32= -R_REAL32,
        VR_REAL64= -R_REAL64,
    };

    enum State {
        /// location lies in an unexecutable edge with an as yet unknown constant value
        TOP,
        /// location lies in an executable edge with an known constant value
        BOTTOM
    };

    std::string* id_;
    RegType regType_;
    State state_;
    Scope* scope_;

    /// for constants
    union Value {
        size_t      index_;

        int         int_;
        int8_t      int8_;
        int16_t     int16_;
        int32_t     int32_;
        int64_t     int64_;
        int8_t      sat8_;
        int16_t     sat16_;

        uint        uint_;
        uint8_t     uint8_;
        uint16_t    uint16_;
        uint32_t    uint32_;
        uint64_t    uint64_;
        uint8_t     usat8_;
        uint16_t    usat16_;

        float       real_;
        float       real32_;
        double      real64_;

        void*       ptr_;
    };

    Value value_;

    PseudoReg(std::string* id, RegType regType)
        : id_(id)
        , regType_(regType)
        , state_(TOP) // TOP is assumed as initial state
        , scope_(0)
    {}

    std::string toString() const {
        if (id_)
            return *id_;
        // else
        std::ostringstream oss;

        // TODO
        return "";
    }
};

typedef List<PseudoReg*> RegList;

#endif // SWIFT_PSEUDOREG_H
