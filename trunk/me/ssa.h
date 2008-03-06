#ifndef ME_SSA_H
#define ME_SSA_H

#include <fstream>

#include "utils/graph.h"
#include "utils/list.h"

#include "me/forward.h"

namespace me {

//------------------------------------------------------------------------------

#define INSTRLIST_EACH(iter, instrList) \
    for (me::InstrNode (iter) = (instrList).first(); (iter) != (instrList).sentinel(); (iter) = (iter)->next())

//------------------------------------------------------------------------------

/**
 * Base class for all instructions.
 */
struct InstrBase
{
    RegSet liveIn_; /// regs that are live-in  at this instruction.
    RegSet liveOut_;/// regs that are live-out at this instruction.

/*
    destructor
*/

    virtual ~InstrBase() {}

/*
    further methods
*/

    /**
     * Computes whether this \p instr ist the first instruction which does not
     * have \p var in the \a liveOut_.
     *
     * @param instr The instruction which should be tested. \p instr must have
     *      a predecessor and must contain a BaseAssignInstr.
     * @param var The Reg which should be tested.
     */
    static bool isLastUse(InstrNode instrNode, Reg* var);

    virtual std::string toString() const = 0;
};

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
 * GotoInstr does not calculate something. It influences the control flow.
 */
struct GotoInstr : public InstrBase
{
    InstrNode labelNode_;
    BBNode succBB_;

    GotoInstr(InstrNode labelNode)
        : labelNode_(labelNode)
    {
        swiftAssert( typeid(*labelNode->value_) == typeid(LabelInstr),
            "labelNode must be a node to a LabelInstr");
    }

    LabelInstr* label()
    {
        return (LabelInstr*) labelNode_->value_;
    }
    const LabelInstr* label() const
    {
        return (LabelInstr*) labelNode_->value_;
    }

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

/**
 * This is the base class for all instructions with in and out-going arguments.
 * TODO documentation for inout args.
 */
struct AssignmentBase : public InstrBase
{
    Reg**   lhs_;           ///< Left hand side Regs.
    int*    lhsOldVarNr_;   ///< Left hand side old varNrs.
    size_t  numLhs_;        ///< Number of left hand side args.

    Op**    rhs_;           ///< Right hand side Ops.
    size_t  numRhs_;        ///< Number of righthand side args.

/*
    constructor and destructor
*/

    AssignmentBase(size_t numLhs, size_t numRhs);
    ~AssignmentBase();

/*
    further methods
*/

    /**
     * When fully in SSA form the old var numbers are not needed anymore and
     * can be destroyed.
    */
    void destroyLhsOldVarNrs();
};

//------------------------------------------------------------------------------

/**
 * Implements phi functions.
 */
struct PhiInstr : public AssignmentBase
{
    BBNode* sourceBBs_; ///< predecessor basic block of each rhs-arg

/*
    constructor and destructor
*/

    PhiInstr(Reg* result, size_t numRhs);
    ~PhiInstr();

/*
    getters and setters
*/

//     Reg* result();
//     const Reg* result() const;
//     int resultOldVarNr() const;

/*
    further methods
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
    constructor
*/

    AssignInstr(int kind, Reg* result, Op* op1, Op* op2 = 0);

/*
    further methods
*/

    std::string toString() const;
    void genCode(std::ofstream& ofs);

    std::string getOpString() const;
};

//------------------------------------------------------------------------------

struct BranchInstr : public AssignmentBase
{
    InstrNode trueLabelNode_;
    InstrNode falseLabelNode_;

    BBNode trueBB_;
    BBNode falseBB_;

/*
    constructor
*/
    BranchInstr(Op* boolOp, InstrNode trueLabelNode, InstrNode falseLabelNode);

/*
    further methods
*/
    LabelInstr* trueLabel()
    {
        return (LabelInstr*) trueLabelNode_->value_;
    }
    LabelInstr* falseLabel()
    {
        return (LabelInstr*) falseLabelNode_->value_;
    }
    const LabelInstr* trueLabel() const
    {
        return (LabelInstr*) trueLabelNode_->value_;
    }
    const LabelInstr* falseLabel() const
    {
        return (LabelInstr*) falseLabelNode_->value_;
    }

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

struct SpillInstr : public AssignmentBase
{
/*
    constructor
*/
    SpillInstr();

/*
    getters
*/
    Reg* arg();
    const Reg* arg() const;
};

struct Spill : public SpillInstr
{
/*
    constructor
*/

    Spill(Reg* result, Reg* arg);

/*
    further methods
*/

    void genCode(std::ofstream& ofs);
    std::string toString() const;
};

//------------------------------------------------------------------------------

struct Reload : public SpillInstr
{
/*
    constructor
*/

    Reload(Reg* result, Reg* arg);

/*
    further methods
*/

    void genCode(std::ofstream& ofs);
    std::string toString() const;
};

/**
 * Instructions of type LabelInstr mark the bounds of a basic block. So swizzling
 * around other Instr won't invalidate pointers in basic blocks.
 */
// struct InvokeInstr : public InstrBase
// {
//     /// This type is used to specify calling conventions
//     enum Conventions
//     {
//         C_CALL,
//         PASCAL_CALL,
//         STD_CALL,
//         SWIFT_CALL
//     };
//
//     Function* function_;
//     Conventions conventions_;
//
//     RegList args_;  ///< all arguments
//     RegList in_;    ///< incoming arguments
//     RegList out_;   ///< outgoing arguments
//     RegList inout_; ///< in and outgoing arguments
//
//     InvokeInstr(Function* function, Conventions conventions)
//         : function_(function)
//         , conventions_(conventions)
//     {}
//
//     virtual std::string toString() const;
// };
//

//------------------------------------------------------------------------------

} // namespace me

#endif // ME_SSA_H
