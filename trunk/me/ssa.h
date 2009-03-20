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

#ifndef ME_SSA_H
#define ME_SSA_H

#include <fstream>
#include <typeinfo>

#include "utils/graph.h"
#include "utils/list.h"

#include "me/forward.h"

namespace me {

struct Struct;
struct Memory;
struct Offset;

//------------------------------------------------------------------------------

#define INSTRLIST_EACH(iter, instrList) \
    for (me::InstrNode* (iter) = (instrList).first(); (iter) != (instrList).sentinel(); (iter) = (iter)->next())

//------------------------------------------------------------------------------

enum
{
    NO_CONSTRAINT = -1
};

//------------------------------------------------------------------------------

struct Res
{
    Var* var_;
    int  oldVarNr_;   ///< Left hand side old varNrs.
    int  constraint_;

    Res() {}
    Res(Var* var, int oldVarNr, int constraint = NO_CONSTRAINT)
        : var_(var)
        , oldVarNr_(oldVarNr)
        , constraint_(constraint)
    {}

    std::string toString() const;
};

//------------------------------------------------------------------------------

struct Arg
{
    Op* op_;
    int constraint_;

    Arg() {}
    Arg(Op* op, int constraint = NO_CONSTRAINT)
        : op_(op)
        , constraint_(constraint)
    {}

    std::string toString() const;
};

//------------------------------------------------------------------------------

/**
 * Base class for all instructions.
 */
struct InstrBase
{
    typedef std::vector<Res> LHS;
    typedef std::vector<Arg> RHS;

    LHS res_;
    RHS arg_;
    
    VarSet liveIn_; /// vars that are live-in  at this instruction.
    VarSet liveOut_;/// vars that are live-out at this instruction.

    bool constrained_;

    /*
     * destructor
     */

    InstrBase(size_t numLhs, size_t numRhs);
    virtual ~InstrBase();

    /*
     * further methods
     */

     /** 
      * @brief Finds out whether \p var is used in this instruction. 
      *
      * I.e. this var occurs on the right hand side.
      * 
      * @param var The Var which is looked for.
      * 
      * @return True - if this is used here, false otherwise. 
      */
     bool isVarUsed(Var* var);
 
    /** 
     * @brief Finds out whether \p var is defined in this instruction. 
     *
     * I.e. this var occurs on the left hand side.
     * 
     * @param var The Var which is looked for.
     * 
     * @return True - if this is defined here, false otherwise. 
     */
    bool isVarDefined(Var* var);

    /** 
     * @brief Finds occurance of \p var.
     * 
     * @param var The definition to be searched.
     * 
     * @return If found \p var is return, 0 otherwise.
     */
    Var* findResult(Var* var);

    /** 
     * @brief Finds \em first occurance of \p op.
     * 
     * @param op The operator to be searched.
     * 
     * @return If found \p op is return, 0 otherwise.
     */
    Op* findArg(Op* op);

    bool isConstrained() const;

    void constrain();

    void unconstrainIfPossible();

    bool livesThrough(Var* var) const;

    enum OpType
    {
        LITERAL, 
        VARIABLE,
        VARIABLE_DEAD
    };
    
    OpType getOpType(size_t i) const;

    /**
     * Computes whether this \p instr ist the first instruction which does not
     * have \p var in the \a liveOut_.
     *
     * @param instrNode The instruction which should be tested. \p instr must have
     *      a predecessor and must contain a BaseAssignInstr.
     * @param var The Var which should be tested.
     */
    static bool isLastUse(InstrNode* instrNode, Var* var);

    virtual std::string toString() const = 0;

    std::string livenessString() const;
};

//------------------------------------------------------------------------------

/**
 * @brief Instructions of type LabelInstr mark the bounds of a basic block. 
 *
 * So swizzling around other Instr won't invalidate pointers in basic blocks.
 */
struct LabelInstr : public InstrBase
{
    static int counter_;
    std::string label_;

    LabelInstr();

    virtual std::string toString() const
    {
        return label_;
    }

    std::string asmName() const;
};

//------------------------------------------------------------------------------

/** 
 * @brief Implements phi functions.
 */
struct PhiInstr : public InstrBase
{
    BBNode** sourceBBs_; ///< predecessor basic block of each arg

    /*
     * constructor and destructor
     */

    PhiInstr(Var* result, size_t numRhs);
    ~PhiInstr();

    /*
     * getters
     */

    Var* result();
    const Var* result() const;

    int oldResultNr() const;

    /*
     * further methods
     */

    std::string toString() const;
};

//------------------------------------------------------------------------------

/**
 * This Instruction represents either an unary, a binary Instruction or
 * just an assignment.
 *
 * Assignments have a op2_ value of 0. \a assignment_ is set to true.
 * Possible unary operations and the corresponding \a kind_ value: <br>
 *
 * result =  op1 -> '='<br>
 * result += op1 -> '+' <br>
 * result -= op1 -> '-' <br>
 * result *= op1 -> '*' <br>
 * result /= op1 -> '/' <br>
 * result %= op1 -> '%' <br>
 * <br>
 *
 * Unary instructions have a op2_ value of 0.
 * Possible unary operations and the corresponding \a kind_ value: <br>
 *
 * result = - op1 -> UNARY_MINUS <br>
 * result = ^ op1 -> '^' <br>
 * result = not op1 -> NOT <br>
 * <br>
 *
 * Possible binary operations and the corresponding \a kind_ value: <br>
 *
 * result = op1 + op2 -> '+'<br>
 * result = op1 - op2 -> '-' <br>
 * result = op1 * op2 -> '*' <br>
 * result = op1 / op2 -> '/' <br>
 * result = op1 % op2 -> '%' <br>
 * <br>
 * result = op1 AND op2 -> AND <br>
 * result = op1 OR  op2 -> OR <br>
 * result = op1 XOR op2 -> XOR <br>
 * <br>
 * result = op1 == op2 -> EQ <br>
 * result = op1 != op2 -> NQ <br>
 * result = op1 \< op2 -> '<' <br>
 * result = op1 >  op2 -> '>' <br>
 * result = op1 <= op2 -> LE <br>
 * result = op1 >= op2 -> GE <br>
*/
struct AssignInstr : public InstrBase
{
    enum
    {
        // be sure not to collide with ASCII codes
        EQ = 256, NE,
        LE, GE,
        AND, OR, XOR,
        NOT, UNARY_MINUS
    };

    union
    {
        int kind_;
        char c_;
    };

    /*
     * constructor
     */

    AssignInstr(int kind, Var* result, Op* op1, Op* op2 = 0);

    /*
     * further methods
     */

    Var* resVar();
    Reg* resReg();

    bool isArithmetic() const;
    bool isComparison() const;

    std::string toString() const;

    std::string getOpString() const;
};

/** 
 * @brief A no operation instruction
 *
 * This instruction can be used to artificially increase the live span of
 * the args.
 */
struct NOP : public InstrBase
{
    /*
     * constructors
     */

    /** 
     * @brief Constructor with on Var arg as use.
     * 
     * @param op The Var which should be used here. 
     */
    NOP(Op* op);

    /*
     * further methods
     */

    virtual std::string toString() const;
};
//------------------------------------------------------------------------------

/** 
 * @brief Base for jumps to other BBNode instances.
 *
 * IMPORTANT: currently it is assumed that the jump targets are distinct.
 */
struct JumpInstr : public InstrBase
{
    size_t numTargets_;
    InstrNode* instrTargets_[2];
    BBNode* bbTargets_[2];

    /*
     * constructor 
     */

    JumpInstr(size_t numLhs, size_t numRhs_, size_t numTargets);
};

//-------------------------------------------------------------------------------


/** 
 * @brief Performs a jump to a given Target.
 *
 * The target must be an InstrNode* to a LabelInstr.
 */
struct GotoInstr : public JumpInstr
{
    /*
     * constructor
     */

    GotoInstr(InstrNode* instrTarget);

    /*
     * getters
     */

    LabelInstr* label();
    const LabelInstr* label() const;

    /*
     * further methods
     */

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

struct BranchInstr : public JumpInstr
{
    enum
    {
        CC_NOT_SET = -1
    };

    /// Use this for some back-end specific stuff for condition code handling
    int cc_;

    BBNode* trueBB_;
    BBNode* falseBB_;

    /*
     * constructor
     */

    BranchInstr(Op* boolOp, InstrNode* trueLabel, InstrNode* falseLabel);

    /*
     * getters
     */

    LabelInstr* trueLabel();
    const LabelInstr* trueLabel() const;

    LabelInstr* falseLabel();
    const LabelInstr* falseLabel() const;

    Op* getOp();
    const Op* getOp() const;

    /*
     * further methods
     */

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

struct Spill : public InstrBase
{
    /*
     * constructor
     */

    Spill(Var* result, Var* arg);

    /*
     * further methods
     */

    Var* resVar();
    Reg* resReg();

    std::string toString() const;
};

//------------------------------------------------------------------------------

struct Reload : public InstrBase
{
    /*
     * constructor
     */

    Reload(Var* result, Var* arg);

    /*
     * further methods
     */

    Var* resVar();
    Reg* resReg();

    std::string toString() const;
};

//------------------------------------------------------------------------------

/** 
 * @brief Load from a stack location or an arbitrary memory location.
 *
 * A Load actually has two versions: <br>
 * - var = Load(stack_var, offset) <br>
 * - var = Load(ptr, offset) <br>
 * The former variant fetches an item from the stack. 
 * The latter one fetches an item from an arbitrary memory location. <br>
 * Note that the offset is not a real argument it is just a member of this
 * class although the notation might imply that.
 */
struct Load : public InstrBase
{
    Offset* offset_;

    /*
     * constructor and destructor
     */

    /** 
     * @brief Loads the the content of \p mem via \p ptr with an at 
     * compile time known \p offset into \p result.
     * 
     * @param result A proper pseudo-varister. Is an lvalue.
     *
     * @param location The memory location to load from. Must be either of 
     *      type \a me::Op::R_STACK or of type \a me::Op::R_PTR. 
     *      This is an rvalue.
     *
     * @param offset The offset to the base pointer / stack location.
     */
    Load(Var* result, Var* location, Offset* offset);
    ~Load();

    /*
     * further methods
     */

    int getOffset() const;
    Reg* resReg();
    std::string toString() const;
};

//------------------------------------------------------------------------------

/** 
 * @brief Store into a stack location or an arbitrary memory location.
 *
 * A Store actually has two versions: <br>
 * - stack_var = Store(arg, stack_var, offset) <br>
 * -             Store(arg,       ptr, offset) <br>
 * The former variant stores an item to the stack. 
 * The latter one stores an item to an arbitrary memory location. <br>
 * Note that the offset is not a real argument it is just a member of this
 * class although the notation might imply that.
 */
struct Store : public InstrBase
{
    Offset* offset_;

    /*
     * constructor and destructor
     */

    /** 
     * @brief Loads the the content of \p mem via \p ptr with an at 
     * compile time known \p offset into \p result.
     * 
     * @param location The memory location to store to. Must be either of 
     *      type \a me::Op::R_STACK or of type \a me::Op::R_PTR. 
     *      In the former case this is an lvalue in the latter one an rvalue.
     *
     * @param arg The rvalue to be stored.
     *
     * @param offset The offset to the base pointer / stack location.
     */
    Store(Var* location, Op* arg, Offset* offset);
    ~Store();

    /*
     * further methods
     */

    int getOffset() const;
    MemVar* resMemVar();
    std::string toString() const;
};

//------------------------------------------------------------------------------

struct SetParams : public InstrBase
{
    /*
     * constructor
     */

    SetParams(size_t numLhs);

    /*
     * further methods
     */

    std::string toString() const;
};

//------------------------------------------------------------------------------

struct SetResults : public InstrBase
{
    /*
     * constructor
     */

    SetResults(size_t numRhs);

    /*
     * further methods
     */

    std::string toString() const;
};

//------------------------------------------------------------------------------

/** 
 * @brief This represents a \a Function call.
 *
 * It is assumed that all arguments are \em NOT changed in the function.
 * If you want to model the change of a function's argument add another
 * definition: <br>
 * a, b, c = CallInstr(e, f, c) <br>
 * In this example c is changed. c get a new name during SSA construction
 * or an reconstructSSAForm pass.
 */
struct CallInstr : public InstrBase
{
    /*
     * constructor
     */

    /**
     * Creates a \a Function static call to a symbol.
     * 
     * @param numRes The number of results.
     * @param numArgs  The number of arguments.
     * @param symbol The symbol used for calling
     */
    CallInstr(size_t numRes, 
              size_t numArgs, 
              const std::string& symbol, 
              bool vararg = false);

    /*
     * further methods
     */

    bool isVarArg() const;
    virtual std::string toString() const;

    /*
     * data
     */

    std::string symbol_;
    bool vararg_;
};

} // namespace me

#endif // ME_SSA_H
