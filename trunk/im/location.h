#ifndef SWIFT_PSEUDOREG_H
#define SWIFT_PSEUDOREG_H

// this should be the only dependency from ../fe/.*
#include "../fe/tokens.h" // be consistent with these tokens

// -----------------------------------------------------------------------------

struct PseudoReg
{
    /// Types of pseudo registers / constants
    enum Reg
    {
        R_INDEX = INDEX,
        R_INT   = INT,
        R_INT8  = INT8,
        R_INT16 = INT16,
        R_INT32 = INT32,
        R_INT64 = INT64,

        R_SAT8  = SAT8,
        R_SAT16 = SAT16,

        R_UINT  = UINT,
        R_UINT8 = UINT8,
        R_UINT16= UINT16,
        R_UINT32= UINT32,
        R_UINT64= UINT64,

        R_USAT8 = USAT8,
        R_USAT16= USTA16,

        R_REAL  = REAL,
        R_REAL32= REAL32,
        R_REAL64= REAL64,

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
        VR_USAT16= -R_USTA16,

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

    Reg reg_;
    State state_;
    Scope* scope_;

    /**
        for constants:
        (SIMD constant propagation not implemented. necessary?)
    */
    union {
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

    PseudoReg(Reg reg)
        : reg_(reg)
        : state_(TOP) // TOP is assumed as initial state
        , scope_(0)
    {}

    virtual ~PseudoReg() {}
};

#endif // SWIFT_PSEUDOREG_H
