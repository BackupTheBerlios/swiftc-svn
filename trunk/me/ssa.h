#ifndef ME_SSA_H
#define ME_SSA_H

#include <fstream>

#include "utils/graph.h"
#include "utils/list.h"

#include "me/forward.h"

namespace me {

//------------------------------------------------------------------------------

#define INSTRLIST_EACH(iter, instrList) \
    for (me::InstrNode* (iter) = (instrList).first(); (iter) != (instrList).sentinel(); (iter) = (iter)->next())

//------------------------------------------------------------------------------

/**
 * Base class for all instructions.
 */
struct InstrBase
{
    RegSet liveIn_; /// regs that are live-in  at this instruction.
    RegSet liveOut_;/// regs that are live-out at this instruction.

    /*
     * destructor
     */

    virtual ~InstrBase() {}

    /*
     * further methods
     */

    /**
     * Computes whether this \p instr ist the first instruction which does not
     * have \p var in the \a liveOut_.
     *
     * @param instr The instruction which should be tested. \p instr must have
     *      a predecessor and must contain a BaseAssignInstr.
     * @param var The Reg which should be tested.
     */
    static bool isLastUse(InstrNode* instrNode, Reg* var);

    virtual std::string toString() const = 0;
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
 * This is the base class for all instructions with in and out-going arguments.
 * TODO documentation for inout args.
 */
struct AssignmentBase : public InstrBase
{
    size_t  numLhs_;        ///< Number of left hand side args.
    Reg**   lhs_;           ///< Left hand side Regs.
    int*    lhsOldVarNr_;   ///< Left hand side old varNrs.

    size_t  numRhs_;        ///< Number of righthand side args.
    Op**    rhs_;           ///< Right hand side Ops.

    /*
     * constructor and destructor
     */

    AssignmentBase(size_t numLhs, size_t numRhs);
    ~AssignmentBase();

    /*
     * further methods
     */

    /**
     * When fully in SSA form the old var numbers are not needed anymore and
     * can be destroyed.
     */
    void destroyLhsOldVarNrs();

    /** 
     * @brief Finds out whether \p reg is used in this instruction. 
     *
     * I.e. this reg occurs on the right hand side.
     * 
     * @param reg The Reg which is looked for.
     * 
     * @return True - if this is used here, false otherwise. 
     */
    bool isRegUsed(Reg* reg);

    /** 
     * @brief Finds out whether \p reg is defined in this instruction. 
     *
     * I.e. this reg occurs on the left hand side.
     * 
     * @param reg The Reg which is looked for.
     * 
     * @return True - if this is defined here, false otherwise. 
     */
    bool isRegDefined(Reg* reg);
};

//------------------------------------------------------------------------------

/** 
 * @brief Implements phi functions.
 */
struct PhiInstr : public AssignmentBase
{
    BBNode** sourceBBs_; ///< predecessor basic block of each rhs-arg

    /*
     * constructor and destructor
     */

    PhiInstr(Reg* result, size_t numRhs);
    ~PhiInstr();

    /*
     * getters
     */

    Reg* result();
    const Reg* result() const;

    int oldResultNr() const;

    /*
     * further methods
     */

    std::string toString() const;
    void genCode(std::ofstream& /*ofs*/) {}
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
struct AssignInstr : public AssignmentBase
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

    AssignInstr(int kind, Reg* result, Op* op1, Op* op2 = 0);

    /*
     * further methods
     */

    std::string toString() const;
    void genCode(std::ofstream& ofs);

    std::string getOpString() const;
};

//------------------------------------------------------------------------------

struct JumpInstr : public AssignmentBase
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
    InstrNode* trueLabelNode_;
    InstrNode* falseLabelNode_;

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

    /*
     * further methods
     */

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

struct SpillInstr : public AssignmentBase
{
    /*
     * constructor
     */

    SpillInstr();

    /*
     * getters
     */

    Reg* arg();
    const Reg* arg() const;
};

struct Spill : public SpillInstr
{
    /*
     * constructor
     */

    Spill(Reg* result, Reg* arg);

    /*
     * further methods
     */

    void genCode(std::ofstream& ofs);
    std::string toString() const;
};

//------------------------------------------------------------------------------

struct Reload : public SpillInstr
{
    /*
     * constructor
     */

    Reload(Reg* result, Reg* arg);

    /*
     * further methods
     */

    void genCode(std::ofstream& ofs);
    std::string toString() const;
};

} // namespace me

#endif // ME_SSA_H
