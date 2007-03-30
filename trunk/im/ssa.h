#ifndef SWIFT_SSA_H
#define SWIFT_SSA_H

#include <fstream>

#include "utils/list.h"
#include "pseudoreg.h"

// forward declarations
struct PseudoReg;

//------------------------------------------------------------------------------

struct InstrBase
{
    virtual ~InstrBase() {}

    virtual std::string toString() const = 0;
};

//------------------------------------------------------------------------------
//PseudoRegInstr---------------------------------------------------------------------
//------------------------------------------------------------------------------

struct PseudoRegInstr : public InstrBase
{
    virtual void genCode(std::ofstream& ofs) = 0;
};

//------------------------------------------------------------------------------

struct AssignInstr : public PseudoRegInstr
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

    std::string toString() const;
    void genCode(std::ofstream& ofs);
};

//------------------------------------------------------------------------------

/**
 *
 */
struct UnInstr : public PseudoRegInstr
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
 *  result = op1 l_and op2 <br>
 *  result = op1 l_or  op2 <br>
 *  result = op1 l_xor op2 <br>
 *  <br>
 *  result = op1 == op2 <br>
 *  result = op1 != op2 <br>
 *  result = op1 \< op2 <br>
 *  result = op1 >  op2 <br>
 *  result = op1 <= op2 <br>
 *  result = op1 >= op2 <br>
*/
struct BinInstr : public PseudoRegInstr
{
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

    std::string toString() const;
    void genCode(std::ofstream& ofs);
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//BranchInstr-------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * if a goto b
*/

// struct GotoInstr
// {
// };

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
