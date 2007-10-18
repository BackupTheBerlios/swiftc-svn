#ifndef SWIFT_SSA_H
#define SWIFT_SSA_H

#include <fstream>

#include "utils/graph.h"
#include "utils/list.h"

#include "me/pseudoreg.h"

// forward declarations
struct Function;
struct BasicBlock;
typedef Graph<BasicBlock>::Node BBNode;

//------------------------------------------------------------------------------

/**
 * @brief Base class for all instructions
 */
struct InstrBase
{
    /**
     * Only LITERAL PseudoRegs must be deleted here. Other (true) PseudoReg
     * will be deleted by the functab.
    */
    virtual ~InstrBase() {}

    virtual std::string toString() const = 0;
};

typedef List<InstrBase*> InstrList;

#define INSTRLIST_EACH(iter, instrList) \
    for (InstrList::Node* (iter) = (instrList).first(); (iter) != (instrList).sentinel(); (iter) = (iter)->next())

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * @brief Implements phi functions
 * These instructions can artificially increase the life time of PseudoReg
 */
struct PhiInstr : public InstrBase
{
    PseudoReg* result_;

    PseudoReg** args_;
    BBNode**    sourceBBs_;
    size_t      argc_;

    int oldResultVar_;

    PhiInstr(PseudoReg* result, size_t argc);
    ~PhiInstr();

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
struct AssignInstr : public InstrBase
{
    enum
    {
        // be sure not to collide with ascii codes
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

    PseudoReg* result_;
    PseudoReg* op1_;
    PseudoReg* op2_;

    int oldResultVar_;

    AssignInstr(int kind, PseudoReg* result, PseudoReg* op1, PseudoReg* op2 = 0);
    ~AssignInstr();

    std::string toString() const;
    void genCode(std::ofstream& ofs);

    /**
     * Returns whether this is an unary Instruction i.e. kind_ is
     * NOT, UNARY_MINUS or DEREF
    */
    bool isUnaryInstr()  const {
        return op2_ == 0 && (kind_ == NOT || kind_ == UNARY_MINUS || kind_ == '^');
    }

    std::string getOpString() const;

    void replaceVar();
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
    InstrList::Node* labelNode_;
    BBNode* succBB_;

    GotoInstr(InstrList::Node* labelNode)
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

struct BranchInstr : public InstrBase
{
    PseudoReg* boolReg_;

    InstrList::Node* trueLabelNode_;
    InstrList::Node* falseLabelNode_;

    BBNode* trueBB_;
    BBNode* falseBB_;

    BranchInstr(PseudoReg* boolReg, InstrList::Node* trueLabelNode, InstrList::Node* falseLabelNode);
    ~BranchInstr();

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

/**
 * Instructions of type LabelInstr mark the bounds of a basic block. So swizzling
 * around other Instr won't invalidate pointers in basic blocks.
 */
struct InvokeInstr : public InstrBase
{
    /// This type is used to specify calling conventions
    enum Conventions
    {
        C_CALL,
        PASCAL_CALL,
        STD_CALL,
        SWIFT_CALL
    };

    Function* function_;
    Conventions conventions_;

    RegList args_;  ///< all arguments
    RegList in_;    ///< incoming arguments
    RegList out_;   ///< outgoing arguments
    RegList inout_; ///< in and outgoing arguments

    InvokeInstr(Function* function, Conventions conventions)
        : function_(function)
        , conventions_(conventions)
    {}

    virtual std::string toString() const;
};


//------------------------------------------------------------------------------

#endif // SWIFT_SSA_H
