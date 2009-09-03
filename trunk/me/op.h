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

#include "utils/box.h"
#include "utils/list.h"
#include "utils/types.h"

#include "me/defuse.h"

namespace me {

/*
 * forward declarations
 */

struct Aggregate;
struct InstrBase;
struct Reg;
struct Struct;

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
    enum
    {
        SIMD_OFFSET = 14
    };

    /// Types of pseudo registers / constants
    enum Type
    {
        R_BOOL   = 1 << 0,

        // pointers
        R_PTR    = 1 << 1, ///< A pointer to an arbitrary location.

        /** 
         * @brief A variable on the local stack frame. 
         *
         * This is not a spilled local. See \a Memory for details.
         */
        R_MEM    = 1 <<  2, 

        // integers
        R_INT8   = 1 <<  3,
        R_INT16  = 1 <<  4,
        R_INT32  = 1 <<  5,
        R_INT64  = 1 <<  6,
        R_SAT8   = 1 <<  7,
        R_SAT16  = 1 <<  8,

        // unsigned integers
        R_UINT8  = 1 <<  9 ,
        R_UINT16 = 1 << 10,
        R_UINT32 = 1 << 11,
        R_UINT64 = 1 << 12,
        R_USAT8  = 1 << 13,
        R_USAT16 = 1 << 14,

        // floating points
        R_REAL32 = 1 << 15,
        R_REAL64 = 1 << 16,

        /*
         * simd types
         */

        // signed integers
        S_INT8   = R_INT8  << SIMD_OFFSET,
        S_INT16  = R_INT16 << SIMD_OFFSET,
        S_INT32  = R_INT32 << SIMD_OFFSET,
        S_INT64  = R_INT64 << SIMD_OFFSET,
        S_SAT8   = R_SAT8  << SIMD_OFFSET,
        S_SAT16  = R_SAT16 << SIMD_OFFSET,

        // unsigned integers
        S_UINT8   = R_UINT8  << SIMD_OFFSET,
        S_UINT16  = R_UINT16 << SIMD_OFFSET,
        S_UINT32  = R_UINT32 << SIMD_OFFSET,
        S_UINT64  = R_UINT64 << SIMD_OFFSET,
        S_USAT8   = R_USAT8  << SIMD_OFFSET,
        S_USAT16  = R_USAT16 << SIMD_OFFSET,

        // floating points
        S_REAL32  = R_REAL32 << SIMD_OFFSET,
        S_REAL64  = R_REAL64 << SIMD_OFFSET,
    };

    enum
    {
        SIMD_TYPES 
            =  S_INT8 | S_INT16  |  S_INT32 |  S_INT64
            | S_UINT8 | S_UINT16 | S_UINT32 | S_UINT64
            | S_SAT8  | S_SAT16
            | S_USAT8 | S_USAT16
            | S_REAL32| S_REAL64,

        VECTORIZABLE
            =  R_INT8 | R_INT16  |  R_INT32 |  R_INT64
            | R_UINT8 | R_UINT16 | R_UINT32 | R_UINT64
            | R_SAT8  | R_SAT16
            | R_USAT8 | R_USAT16
            | R_REAL32| R_REAL64,
    };

    Type type_;

    /*
     * constructor and destructor
     */

    Op(Type type);
    virtual ~Op();

    /*
     * virtual methods
     */

    virtual bool typeCheck(int typeMask) const;

    virtual Reg* isReg(int typeMask);
    virtual Reg* isReg(int typeMask, bool spilled);

    virtual Reg* isNotSpilled(int typeMask);

    /** 
     * @brief Checks whether this Reg is actually in a spilled aggregate location.
     *
     * If the type is not Reg or is not spilled 0 is return.
     *
     * @return \a this if it is a spilled Reg, 0 otherwise.
     */
    virtual Reg* isSpilled();

    virtual Reg* isSpilled(int typeMask);

    virtual std::string toString() const = 0;

    virtual Op* toSimd(Vectorizer* vectorizer) const = 0;

    /*
     * further methods
     */

    bool isReal() const;
    bool isSimd() const;

    /*
     * static methods
     */

    static bool isReal(Type type);
    static bool isSimd(Type type);
    static int sizeOf(Type type);
    static Type toSimd(Type type);
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

    /*
     * constructor
     */

    Undef(Type type)
        : Op(type)
    {}

    /*
     * virtual methods
     */

    virtual Undef* toSimd(Vectorizer* vectorizer) const;
    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

#define NUM_MAX_SIMD_WIDTH 16

/** 
 * @brief Represents a Const.
 *
 * That is a constant which can be of different builtin types.
 */
struct Const : public Op
{
    Box boxes_[NUM_MAX_SIMD_WIDTH];
    size_t numBoxElems_;

    /*
     * constructor
     */

    Const(Type type, size_t numBoxElems = 1);

    /*
     * virtual methods
     */

    virtual Const* toSimd(Vectorizer* vectorizer) const;
    virtual std::string toString() const;

    /*
     * further methods
     */

    /// Returns the default first element of the \p box array.
    Box& box();
    /// Returns the default first element of the \p box array.
    const Box& box() const;

    void broadcast(const Box& box);
};

//------------------------------------------------------------------------------

struct Var : public Op
{
    /**
     * @brief Name of this \a Reg in the current \a Function.
     *
     * varNr_ >= 0 a variable which is already defined only once <br>
     * varNr_  < 0 a variable which must be converted to SSA form <br>
     *
     * When in SSA-Form all varNr_ < 0 will be replaced by names > 0
     *
     * This is only important when invoking \a Cfg::constructSSAForm. If you
     * use \a Cfg::reconstructSSAForm \a varNr_ doesn't matter. There you
     * have to manually take care of proper \a DefUse chains.
     */
    int varNr_;

    enum
    {
        /// Reg has not been colored so far.
        NOT_COLORED_YET = -1, 
        /// Use this if you don't want to color this var at all for some reason.
        DONT_COLOR      = -2 
    };

    /**
     * The color after coloring: <br>
     * \a NOT_COLORED_YET if a color has not already been assigned. <br>
     * \a DONT_COLOR if it should not be considered during coloring.
     */
    int color_;

    DefUse def_;      ///< knows where the var is defined
    DefUseList uses_; ///< knows all uses of this var

#ifdef SWIFT_DEBUG
    std::string id_;    ///< This stores the name of the original var in the debug version.
    VarNode* varNode_;  ///< This stores the graph node of the IG in the debug version.
#endif // SWIFT_DEBUG

    /*
     * constructors
     */

#ifdef SWIFT_DEBUG

    Var(Type type, int varNr, const std::string* id = 0);
    static Var* create(Type type, int varNr, const std::string* id = 0);

#else // SWIFT_DEBUG

    Var(Type type, int varNr);
    static Var* create(Type type, int varNr);

#endif // SWIFT_DEBUG

    /*
     * virtual methods
     */

    virtual Var* clone(int varNr) const = 0;
    virtual bool typeCheck(int typeMask) const;
    virtual Var* toSimd(Vectorizer* vectorizer) const;
    virtual std::string toString() const;

    /*
     * further methods
     */

    /// Returns whether this Var is only defined once
    bool isSSA() const;

    bool dontColor() const;

    /**
     * Checks via an swiftAssert whether this is not SSA and returns the var
     * number negated and casted to size_t in order to use this for array
     * accesses or so.
     */
    size_t var2Index() const;
};

//------------------------------------------------------------------------------

/** 
 * @brief This is a pseudo register.
 *
 * Pseudo registers will be mapped to real registers during code generation.
 */
struct Reg : public Var
{
    /// Is this currently in a spilled aggregate loaction?
    bool isSpilled_;

    /*
     * constructor
     */

#ifdef SWIFT_DEBUG
    Reg(Type type, int varNr, const std::string* id = 0);
#else // SWIFT_DEBUG
    Reg(Type type, int varNr);
#endif // SWIFT_DEBUG

    /*
     * virtual methods
     */

    virtual Reg* clone(int varNr) const;
    virtual Reg* isReg(int typeMask);
    virtual Reg* isReg(int typeMask, bool spilled);
    virtual Reg* isSpilled();
    virtual Reg* isSpilled(int typeMask);
    virtual Reg* isNotSpilled(int typeMask);
    virtual Reg* toSimd(Vectorizer* vectorizer) const;
    virtual std::string toString() const;

    /*
     * further methods
     */

    bool isColorAdmissible(int color);
    int getPreferedColor() const;
};

//------------------------------------------------------------------------------

struct MemVar : public Var
{
    Aggregate* aggregate_;

    /*
     * constructor
     */

#ifdef SWIFT_DEBUG
    MemVar(Aggregate* aggregate, int varNr, const std::string* id = 0);
#else // SWIFT_DEBUG
    MemVar(Aggregate* aggregate, int varNr);
#endif // SWIFT_DEBUG

    /*
     * further methods
     */

    virtual MemVar* clone(int varNr) const;
    virtual MemVar* toSimd(Vectorizer* vectorizer) const;
    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

/*
 * typedefs and defines for easy usage
 */


#define REGSET_EACH(iter, regSet) \
    for (me::RegSet::iterator (iter) = (regSet).begin(); (iter) != (regSet).end(); ++(iter))

#define REGMAP_EACH(iter, regMap) \
    for (me::RegMap::iterator (iter) = (regMap).begin(); (iter) != (regMap).end(); ++(iter))

/// Use this macro in order to easily visit all elements of a VarMap
#define VARMAP_EACH(iter, varMap) \
    for (me::VarMap::iterator (iter) = (varMap).begin(); (iter) != (varMap).end(); ++(iter))

/// Use this macro in order to easily visit all elements of a VarSet
#define VARSET_EACH(iter, varSet) \
    for (me::VarSet::iterator (iter) = (varSet).begin(); (iter) != (varSet).end(); ++(iter))

} // namespace me

#endif // ME_OP_H
