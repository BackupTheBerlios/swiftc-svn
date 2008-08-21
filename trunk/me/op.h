#ifndef ME_OP_H
#define ME_OP_H

#include <map>
#include <string>
#include <sstream>

#include "utils/list.h"
#include "utils/types.h"

#include "me/defuse.h"

// forward declarations

#ifdef SWIFT_DEBUG

namespace be {
    struct IVar;
    typedef Graph<IVar>::Node* VarNode;
}

#endif // SWIFT_DEBUG

namespace me {

struct Struct;
struct InstrBase;

//------------------------------------------------------------------------------

/**
 * Base for an Operant which is used by SSA instrucions
 */
struct Op
{
    /// Types of pseudo registers / constants
    enum Type
    {
        R_INDEX,
        R_INT,  R_INT8,  R_INT16,  R_INT32,  R_INT64,  R_SAT8,  R_SAT16,
        R_UINT, R_UINT8, R_UINT16, R_UINT32, R_UINT64, R_USAT8, R_USAT16,
        R_REAL, R_REAL32, R_REAL64,
        R_BOOL,
        R_STRUCT,

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
        VR_REAL64= -R_REAL64
    };

    Type type_;

/*
    constructor and destructor
*/
    Op(Type type)
        : type_(type)
    {}
    virtual ~Op() {}

/*
    further methods
*/

    virtual std::string toString() const = 0;
};

//------------------------------------------------------------------------------

/**
 * Represents the special value 'undef' (undefined). 
 *
 * Since SSA programs must be strict, assigning this value is one way to make
 * an SSA program strict.
 */
struct Undef : public Op
{
    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

/** 
 * @brief Represents a Literal.
 *
 * That is a constant which can be of different builtin types.
 */
struct Literal : public Op
{
    union Value
    {
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

        bool        bool_;

        void*       ptr_;
    };

    Value value_;

    /*
     * constructor
     */

    Literal(Type type)
        : Op(type)
    {}

    /*
     * further methods
     */

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

struct Reg : public Op
{
    enum
    {
        NOT_COLORED_YET,
        MEMORY_LOCATION
    };

    /**
     * varNr_ >= 0 a variable which is already defined only once <br>
     * varNr_ < 0  a variable which must be converted to SSA form <br>
     *
     * When in SSA-Form all varNr_ < 0 will be replaced by names > 0
     */
    int varNr_;

    /**
     * The color after coloring; <br>
     * -1 if it was not assigned already.
     * -2 if it is a memory location and thus needn't be colored
     */
    int color_;

    DefUse def_;   ///< knows where the var is defined
    UseList uses_; ///< knows all uses of this var

#ifdef SWIFT_DEBUG
    std::string id_; ///< This stores the name of the orignal var in the debug version.
    be::VarNode varNode_; ///< This stores the graph node of the IG in the debug version.
#endif // SWIFT_DEBUG

    /*
     * constructor
     */

#ifdef SWIFT_DEBUG
    Reg(Type type, int varNr, std::string* id = 0);

    static Reg* createMem(Type type, int varNr, std::string* id = 0);
#else // SWIFT_DEBUG
    Reg(Type type, int varNr);

    static Reg* createMem(Type type, int varNr);
#endif // SWIFT_DEBUG

    /*
     * further methods
     */

    /// Returns whether this Var is only defined once
    bool isSSA() const;

    /**
     * Checks via an swiftAssert whether this is not SSA and returns the var
     * number negated and casted to size_t in order to use this for array
     * accesses or so.
     */
    size_t var2Index() const;

    /**
     * Checks whether this Reg has already been colored. If this is actually a
     * MEMORY_LOCATION an assertion is thrown.
     */
    bool isColored() const;
    bool isMem() const;

    /**
     * Invoke this method if you have done something with this var such that
     * the SSA property is violated. This method will create a new negative
     * \a varNr_.
     */
    void removeSSAProperty();

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

/*
 * typedefs and defines for easy usage
 */

typedef std::map<int, Reg*> RegMap;
typedef std::set<Reg*> RegSet;
typedef List<Reg*> RegList;

/// Use this macro in order to easily visit all elements of a RegMap
#define REGMAP_EACH(iter, regMap) \
    for (me::RegMap::iterator (iter) = (regMap).begin(); (iter) != (regMap).end(); ++(iter))

/// Use this macro in order to easily visit all elements of a RegSet
#define REGSET_EACH(iter, regSet) \
    for (me::RegSet::iterator (iter) = (regSet).begin(); (iter) != (regSet).end(); ++(iter))

} // namespace me

#endif // ME_OP_H