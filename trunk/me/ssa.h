#ifndef SWIFT_SSA_H
#define SWIFT_SSA_H

#include <fstream>

#include "utils/list.h"
#include "pseudoreg.h"

// forward declarations
struct PseudoReg;

//------------------------------------------------------------------------------

/**
 * @brief Base class for all instructions
 */
struct InstrBase
{
    virtual ~InstrBase() {}

    virtual std::string toString() const = 0;
};

typedef List<InstrBase*> InstrList;

//------------------------------------------------------------------------------

/**
 * Instructions of type LabelInstr mark the bounds of a basic block. So swizzling
 * around other Instr won't invalidate pointers in basic blocks.
 */
struct LabelInstr : public InstrBase
{
    static int counter_;
    std::string label_;

    LabelInstr();
    LabelInstr(const std::string& label)
        : label_(label)
    {}

    virtual std::string toString() const
    {
        return label_;
    }
};

//------------------------------------------------------------------------------

/**
 * GenCodeInstr can -- as the name says -- generate code.
 * Thus genCode is abstract here.
 */
struct GenCodeInstr : public InstrBase
{
    virtual void genCode(std::ofstream& ofs) = 0;
};

//------------------------------------------------------------------------------
//CalcInstr---------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * CalcInstr are instructions which calcultate something
 */
struct CalcInstr : public GenCodeInstr
{
};

//------------------------------------------------------------------------------

/**
 * @brief NOP = No Operation
 * These instructions can artificially increase the life time of a PseudoReg
 */
struct NOPInstr : public CalcInstr
{
    PseudoReg* reg_;

    NOPInstr(PseudoReg* reg)
        : reg_(reg)
    {}
    virtual std::string toString() const = 0;
    /// dummy implementation, NOP does nothing
    void genCode(std::ofstream& /*ofs*/) {}
};

//------------------------------------------------------------------------------

/**
 * @brief implements phi functions
 * These instructions can artificially increase the life time of PseudoReg
 */
struct PhiInstr : public CalcInstr
{
    PseudoReg* result_;
//     RegList args_;

    PhiInstr(PseudoReg* result)
        : result_(result)
    {}

    virtual std::string toString() const = 0;
    /// dummy implementation, NOP does nothing
    void genCode(std::ofstream& /*ofs*/) {}
};

//------------------------------------------------------------------------------

struct AssignInstr : public CalcInstr
{
    union
    {
        int kind_;
        char c_;
    };

    PseudoReg* result_;
    PseudoReg* reg_;

    AssignInstr(int kind, PseudoReg* result, PseudoReg* reg)
        : kind_(kind)
        , result_(result)
        , reg_(reg)
    {}
    ~AssignInstr()
    {
        swiftAssert( result_->regNr_ != PseudoReg::LITERAL, "this can't be a constant" );
    }

    std::string toString() const;
    void genCode(std::ofstream& ofs);
};

//------------------------------------------------------------------------------

/**
 *
 */
struct UnInstr : public CalcInstr
{
    union
    {
        int kind_;
        char c_;
    };

    PseudoReg* result_;
    PseudoReg* op_;

    UnInstr(int kind, PseudoReg* result, PseudoReg* op)
        : kind_(kind)
        , result_(result)
        , op_(op)
    {}
    ~UnInstr()
    {
        swiftAssert( result_->regNr_ != PseudoReg::LITERAL, "this can't be a constant" );
    }

    std::string toString() const;
    void genCode(std::ofstream& ofs);
};

//------------------------------------------------------------------------------

/**
 *  result = op1 + op2 <br>
 *  result = op1 - op2 <br>
 *  result = op1 * op2 <br>
 *  result = op1 / op2 <br>
 *  result = op1 % op2 <br>
 *  <br>
 *  result = op1 and op2 <br>
 *  result = op1 or  op2 <br>
 *  result = op1 xor op2 <br>
 *  <br>
 *  result = op1 == op2 <br>
 *  result = op1 != op2 <br>
 *  result = op1 \< op2 <br>
 *  result = op1 >  op2 <br>
 *  result = op1 <= op2 <br>
 *  result = op1 >= op2 <br>
*/
struct BinInstr : public CalcInstr
{
    enum
    {
        // be sure not to collide with ascii codes
        EQ = 256,
        NE,
        LE,
        GE
    };

    union
    {
        int kind_;
        char c_;
    };

    PseudoReg* result_;
    PseudoReg* op1_;
    PseudoReg* op2_;

    BinInstr(int kind, PseudoReg* result, PseudoReg* op1, PseudoReg* op2)
        : kind_(kind)
        , result_(result)
        , op1_(op1)
        , op2_(op2)
    {}
    ~BinInstr()
    {
        swiftAssert( result_->regNr_ != PseudoReg::LITERAL, "this can't be a constant" );
    }

    std::string toString() const;
    void genCode(std::ofstream& ofs);
};

//------------------------------------------------------------------------------
//BranchInstr-------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * BranchInstr do not calculate something. They influence the control flow.
 */
struct GotoInstr : public InstrBase
{
    LabelInstr* label_;

    GotoInstr(LabelInstr* label)
        : label_(label)
    {}

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

struct BranchInstr : public InstrBase
{
    PseudoReg* boolReg_;
    LabelInstr* trueLabel_;
    LabelInstr* falseLabel_;

    BranchInstr(PseudoReg* boolReg, LabelInstr* trueLabel, LabelInstr* falseLabel)
        : boolReg_(boolReg)
        , trueLabel_(trueLabel)
        , falseLabel_(falseLabel)
    {
        swiftAssert(boolReg->regType_ == PseudoReg::R_BOOL, "this is not a boolean pseudo reg");
    }

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

/**
 *  result =  PseudoReg <br>
 *  result <- PseudoReg <br>
 *  result <-> PseudoReg <br>
 *  <br>
 *  result += PseudoReg <br>
 *  result -= PseudoReg <br>
 *  result *= PseudoReg <br>
 *  result /= PseudoReg <br>
 *  result %= PseudoReg <br>
 *
 *  result and= PseudoReg <br>
 *  result or=  PseudoReg <br>
 *  result xor= PseudoReg <br>
*/


#endif // SWIFT_SSA_H
