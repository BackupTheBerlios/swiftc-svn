#include "ssa.h"

#include <cstring>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include "utils/stringhelper.h"

#include "me/functab.h"
#include "me/op.h"

namespace me {

//------------------------------------------------------------------------------

bool InstrBase::isLastUse(InstrNode* instrNode, Reg* var)
{
    InstrBase* instr = instrNode->value_;

    return  instr->liveIn_ .find(var) != instr->liveOut_.end()  // must be in the live in
        &&  instr->liveOut_.find(var) == instr->liveOut_.end(); // but not in the live out
}

std::string InstrBase::livenessString() const
{
    std::ostringstream oss;

    // print live in infos
    oss << "\tlive IN:" << std::endl;

    REGSET_EACH(iter, liveIn_)
        oss << "\t\t" << (*iter)->toString() << std::endl;

    // print live out infos
    oss << "\tlive OUT:" << std::endl;

    REGSET_EACH(iter, liveOut_)
        oss << "\t\t" << (*iter)->toString() << std::endl;

    return oss.str();
}

//------------------------------------------------------------------------------

// init static
int LabelInstr::counter_ = 0;

LabelInstr::LabelInstr()
{
    std::ostringstream oss;
    oss << "L" << number2String(counter_);
    label_ = oss.str();

    ++counter_;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

AssignmentBase::AssignmentBase(size_t numLhs, size_t numRhs)
    : numLhs_(numLhs)
    , lhs_( numLhs ? new Reg*[numLhs] : 0 )
    , lhsOldVarNr_( new int[numLhs] )
    , lhsConstraints_(0)
    , numRhs_(numRhs)
    , rhs_( numRhs ? new  Op*[numRhs] : 0 )
    , rhsConstraints_(0)
{}

AssignmentBase::~AssignmentBase()
{
    // delete all literals
    for (size_t i = 0; i < numRhs_; ++i)
    {
        me::Literal* literal = dynamic_cast<me::Literal*>(rhs_[i]);

        if ( literal )
            delete literal;
    }

    if (lhs_)
        delete[] lhs_;
    if (rhs_)
        delete[] rhs_;
    if (lhsConstraints_)
        delete[] lhsConstraints_;
    if (rhsConstraints_)
        delete[] rhsConstraints_;

    destroyLhsOldVarNrs(); // TODO this should be done earlier
}

/*
 * further methods
 */

bool AssignmentBase::isRegUsed(Reg* reg) 
{
    // for each var on the rhs
    for (size_t i = 0; i < numRhs_; ++i) 
    {
        if (rhs_[i] == reg)
            return true;
    }

    // -> not found
    return false;
}

bool AssignmentBase::isRegDefined(Reg* reg) 
{
    // for each var on the lhs
    for (size_t i = 0; i < numLhs_; ++i) 
    {
        if (lhs_[i] == reg)
            return true;
    }

    // -> not found
    return false;
}

void AssignmentBase::destroyLhsOldVarNrs()
{
    delete[] lhsOldVarNr_;
}

Reg* AssignmentBase::findResult(Reg* reg)
{
    Reg** result = std::find(lhs_, lhs_ + numLhs_, reg);
    return result == lhs_ + numLhs_ ? 0 : *result;
}

Op* AssignmentBase::findArg(Op* op)
{
    Op** result = std::find(rhs_, rhs_ + numRhs_, op);
    return result == rhs_ + numRhs_ ? 0 : *result;
}

bool AssignmentBase::isConstrainted() const
{
    return lhsConstraints_ == 0 && rhsConstraints_ == 0;
}

void AssignmentBase::constraint()
{
    swiftAssert(lhsConstraints_ == 0, "already inited");
    swiftAssert(rhsConstraints_ == 0, "already inited");

    lhsConstraints_ = new int[numLhs_];
    rhsConstraints_ = new int[numRhs_];
}

AssignmentBase::OpType AssignmentBase::getOpType(size_t i) const
{
    if ( typeid(*rhs_[i]) == typeid(me::Literal) )
        return CONST;

    swiftAssert( typeid(*rhs_[i]) == typeid(Reg), "must be a Reg" );
    Reg* reg = (Reg*) rhs_[i];

    if ( liveOut_.find(reg) == liveOut_.end() )
        return REG_DEAD;
    else
        return REG;
}

//------------------------------------------------------------------------------

/*
 * constructors
 */

NOP::NOP(Reg* reg)
    : AssignmentBase(0, 1)
{
    rhs_[0] = reg;
}

NOP::NOP(size_t numRegs, Reg** regs)
    : AssignmentBase(0, numRegs)
{
    for (size_t i = 0; i < numRegs; ++i)
    {
        rhs_[i] = regs[i];
    }
}

/*
 * further methods
 */

std::string NOP::toString() const
{
    std::ostringstream oss;
    oss << "NOP(";

    for (size_t i = 0; i < numRhs_ - 1; ++i)
    {
        if (rhs_[i])
            oss << rhs_[i]->toString() << ", ";
    }

    if (rhs_[numRhs_ - 1] )
        oss << rhs_[numRhs_-1]->toString() << ')';

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

PhiInstr::PhiInstr(Reg* result, size_t numRhs)
    : AssignmentBase(1, numRhs) // phi functions always have one result
    , sourceBBs_( new BBNode*[numRhs] )
{
    // init result
    lhs_[0] = result;
    lhsOldVarNr_[0] = result->varNr_;

    // fill everthing with zero -- needed for some debugging
    memset(sourceBBs_, 0, sizeof(BBNode*) * numRhs);
}

PhiInstr::~PhiInstr()
{
    delete[] sourceBBs_;
}

/*
 * getters
 */

Reg* PhiInstr::result()
{
    return lhs_[0];
}

const Reg* PhiInstr::result() const
{
    return lhs_[0];
}

int PhiInstr::oldResultNr() const
{
    return lhsOldVarNr_[0];
}

/*
    further methods
*/

std::string PhiInstr::toString() const
{
    std::ostringstream oss;
    oss << result()->toString() << "\t= phi(";

    for (size_t i = 0; i < numRhs_ - 1; ++i)
    {
        if (!sourceBBs_[i])
            continue;

        if (rhs_[i] && sourceBBs_[i])
            oss << rhs_[i]->toString() << " (" << sourceBBs_[i]->value_->name() << "), ";
        else
            oss << "-, ";
    }

    if (rhs_[numRhs_ - 1] && sourceBBs_[numRhs_ - 1])
        oss << rhs_[numRhs_-1]->toString() << '(' << sourceBBs_[numRhs_-1]->value_->name()  << ')';
    else
        oss << '-';

    oss << ')';

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

AssignInstr::AssignInstr(int kind, Reg* result, Op* op1, Op* op2 /*= 0*/)
    // an AssignInstr always have exactly one result and one or two args
    : AssignmentBase(1, op2 ? 2 : 1) 
    , kind_(kind)
{
    lhs_[0] = result;
    lhsOldVarNr_[0] = result->varNr_;

    rhs_[0] = op1;

    if (op2)
        rhs_[1] = op2;
}

/*
 * further methods
 */

std::string AssignInstr::getOpString() const
{
    std::string opString;
    switch (kind_)
    {
        case UNARY_MINUS:
            opString = "-";
            break;
        case NOT:
            opString = "NOT";
            break;
        case AND:
            opString = "AND";
            break;
        case OR:
            opString = "OR";
            break;
        case XOR:
            opString = "XOR";
            break;
        case EQ:
            opString = "==";
            break;
        case NE:
            opString = "!=";
            break;
        case LE:
            opString = "<=";
            break;
        case GE:
            opString = ">=";
            break;
        case '<':
            opString = "\\<"; // must be escaped for graphviz' dot
            break;
        case '>':
            opString = "\\>"; // must be escaped for graphviz' dot
            break;
        default:
            opString = c_;
    }

    return opString;
}

std::string AssignInstr::toString() const
{
    std::string opString = getOpString();
    std::ostringstream oss;
    oss << lhs_[0]->toString() << '\t';

    // is this a binary, an unary instruction or an assignment?
    if (numRhs_ == 2)
    {
        // it is a binary instruction
        oss << "= " << rhs_[0]->toString() << " " << opString << " " << rhs_[1]->toString();
    }
    else
    {
        if ( kind_ == NOT || kind_ == UNARY_MINUS || kind_ == '^' )
        {
            // it is an unary instruction
            oss << "= " << opString;
        }
        else
        {
            // it is an assignment
            oss << opString;
        }
        oss << ' ' << rhs_[0]->toString();
    }

    return oss.str();
}

void AssignInstr::genCode(std::ofstream& ofs)
{
    ofs << "bi" << std::endl;
};

//------------------------------------------------------------------------------

/*
 * constructor
 */

JumpInstr::JumpInstr(size_t numLhs, size_t numRhs, size_t numTargets)
    : AssignmentBase(numLhs, numRhs)
    , numTargets_(numTargets)
{
    swiftAssert(numTargets <= 2, 
        "currently only a maximum of 2 targets are supported");
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

GotoInstr::GotoInstr(InstrNode* instrTarget)
    : JumpInstr(0, 0, 1) // always no lhs, no rhs, one target
{
    instrTargets_[0] = instrTarget;

    swiftAssert( typeid(*instrTargets_[0]->value_) == typeid(LabelInstr),
        "must be a node to a LabelInstr here");
}

/*
 * getters
 */

LabelInstr* GotoInstr::label()
{
    return (LabelInstr*) instrTargets_[0]->value_;
}

const LabelInstr* GotoInstr::label() const
{
    return (LabelInstr*) instrTargets_[0]->value_;
}

/*
 * further methods
 */

std::string GotoInstr::toString() const
{
    std::ostringstream oss;
    oss << "GOTO " << label()->toString();

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

BranchInstr::BranchInstr(Op* boolOp, InstrNode* trueLabel, InstrNode* falseLabel)
    : JumpInstr(0, 1, 2) // always no results, one rhs, two targets
{
    swiftAssert(boolOp->type_ == Reg::R_BOOL, "this is not a boolean pseudo reg");
    rhs_[0] = boolOp;

    instrTargets_[0] = trueLabel;
    instrTargets_[1] = falseLabel;

    swiftAssert( typeid(*instrTargets_[0]->value_) == typeid(LabelInstr),
        "must be a node to a LabelInstr here");
    swiftAssert( typeid(*instrTargets_[1]->value_) == typeid(LabelInstr),
        "must be a node to a LabelInstr here");
}

/*
 * getters
 */

LabelInstr* BranchInstr::trueLabel() 
{
    return (LabelInstr*) instrTargets_[0]->value_;
}

const LabelInstr* BranchInstr::trueLabel() const
{
    return (LabelInstr*) instrTargets_[0]->value_;
}

LabelInstr* BranchInstr::falseLabel() 
{
    return (LabelInstr*) instrTargets_[1]->value_;
}

const LabelInstr* BranchInstr::falseLabel() const
{
    return (LabelInstr*) instrTargets_[1]->value_;
}

/*
 * further methods
 */

std::string BranchInstr::toString() const
{
    std::ostringstream oss;
    oss << "IF " << rhs_[0]->toString()
        << " THEN " << trueLabel()->toString()
        << " ELSE " << falseLabel()->toString();

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Spill::Spill(Reg* result, Reg* arg)
    : AssignmentBase(1, 1)
{
    swiftAssert( !arg->isMem(), "arg must not be a memory var" );
    swiftAssert( result->isMem(), "result must be a memory var" );
    lhs_[0] = result;
    rhs_[0] = arg;
}

/*
 * further methods
 */

void Spill::genCode(std::ofstream& /*ofs*/)
{
    // TODO
}

std::string Spill::toString() const
{
    std::ostringstream oss;
    oss << lhs_[0]->toString() << "\t= spill(" << rhs_[0]->toString() << ")";

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Reload::Reload(Reg* result, Reg* arg)
    : AssignmentBase(1, 1)
{
    swiftAssert( arg->isMem(), "arg must be a memory var" );
    swiftAssert( !result->isMem(), "result must not be a memory var" );
    lhs_[0] = result;
    rhs_[0] = arg;
}

/*
 * further methods
 */

void Reload::genCode(std::ofstream& /*ofs*/)
{
    // TODO
}

std::string Reload::toString() const
{
    std::ostringstream oss;
    oss << lhs_[0]->toString() << "\t= reload(" << rhs_[0]->toString() << ")";

    return oss.str();
}

} // namespace me
