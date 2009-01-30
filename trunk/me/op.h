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

#ifndef ME_OP_H
#define ME_OP_H

#include <map>
#include <string>
#include <sstream>

#include "utils/list.h"
#include "utils/types.h"

#include "me/defuse.h"

// forward declarations


namespace me {

struct Struct;
struct InstrBase;

#ifdef SWIFT_DEBUG

struct IVar;
typedef Graph<IVar>::Node VarNode;

#endif // SWIFT_DEBUG

//------------------------------------------------------------------------------

/**
 * Base for an Operant which is used by SSA instructions
 */
struct Op
{
    /// Types of pseudo registers / constants
    enum Type
    {
        R_BOOL      = 1 <<  0,

        // integers
        R_INT8      = 1 <<  1,
        R_INT16     = 1 <<  2,
        R_INT32     = 1 <<  3,
        R_INT64     = 1 <<  4,
        R_SAT8      = 1 <<  5,
        R_SAT16     = 1 <<  6,

        // unsigned integers
        R_UINT8     = 1 <<  7,
        R_UINT16    = 1 <<  8,
        R_UINT32    = 1 <<  9,
        R_UINT64    = 1 << 10,
        R_USAT8     = 1 << 11,
        R_USAT16    = 1 << 12,

        // floating points
        R_REAL32    = 1 << 13,
        R_REAL64    = 1 << 14,

        // pointers
        R_PTR       = 1 << 15, ///< A pointer to an arbitrary location.
        R_STACK     = 1 << 16, ///< A pointer to a known location on the stack.

        // Use this to do something special by hand
        R_SPECIAL   = 1 << 17
    };

    Type type_;

    /*
     * constructor and destructor
     */

    Op(Type type)
        : type_(type)
    {}
    virtual ~Op() {}

    /*
     * further methods
     */

    bool typeCheck(int typeMask) const;

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
    Undef(Type type)
        : Op(type)
    {}

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

/** 
 * @brief Represents a Const.
 *
 * That is a constant which can be of different builtin types.
 */
struct Const : public Op
{
    union Value
    {
        size_t      index_;

        int8_t      int8_;
        int16_t     int16_;
        int32_t     int32_;
        int64_t     int64_;
        int8_t      sat8_;
        int16_t     sat16_;

        uint8_t     uint8_;
        uint16_t    uint16_;
        uint32_t    uint32_;
        uint64_t    uint64_;
        uint8_t     usat8_;
        uint16_t    usat16_;

        float       real32_;
        double      real64_;

        bool        bool_;

        void*       ptr_;
    };

    Value value_;

    /*
     * constructor and destructor
     */

    Const(Type type);
    virtual ~Const();

    /*
     * further methods
     */

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

/** 
 * @brief This is a pseudo register.
 *
 * Pseudo registers will be mapped to real registers during code generation.
 */
struct Reg : public Op
{
    enum
    {
        NOT_COLORED_YET = -1,
    };

    /**
     * varNr_ >= 0 a variable which is already defined only once <br>
     * varNr_  < 0 a variable which must be converted to SSA form <br>
     *
     * When in SSA-Form all varNr_ < 0 will be replaced by names > 0
     */
    int varNr_;

    /**
     * The color after coloring: <br>
     * \a NOT_COLORED_YET if it was not assigned already. <br>
     * \a MEMORY_LOCATION if it is a memory location and thus needn't be colored
     */
    int color_;

    bool isMem_;

    DefUse def_;      ///< knows where the var is defined
    DefUseList uses_; ///< knows all uses of this var

#ifdef SWIFT_DEBUG
    std::string id_;    ///< This stores the name of the orignal var in the debug version.
    VarNode* varNode_;  ///< This stores the graph node of the IG in the debug version.
#endif // SWIFT_DEBUG

    /*
     * constructor
     */

#ifdef SWIFT_DEBUG
    Reg(Type type, int varNr, std::string* id = 0);
#else // SWIFT_DEBUG
    Reg(Type type, int varNr);
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
     * @brief Checks whether this Reg is actually a memory location.
     *
     * @return Is this Reg a memory location?
     */
    bool isMem() const;

    bool colorReg(int typeMask) const;

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

/*
 * typedefs and defines for easy usage
 */

/// Use this macro in order to easily visit all elements of a RegMap
#define REGMAP_EACH(iter, regMap) \
    for (me::RegMap::iterator (iter) = (regMap).begin(); (iter) != (regMap).end(); ++(iter))

/// Use this macro in order to easily visit all elements of a RegSet
#define REGSET_EACH(iter, regSet) \
    for (me::RegSet::iterator (iter) = (regSet).begin(); (iter) != (regSet).end(); ++(iter))

} // namespace me

#endif // ME_OP_H
