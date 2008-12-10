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

/*
 * constructor and destructor
 */

InstrBase::InstrBase(size_t numLhs, size_t numRhs)
    : lhs_(numLhs)
    , rhs_(numRhs)
    , constrainted_(false)
{}

InstrBase::~InstrBase()
{
    // delete all literals
    for (size_t i = 0; i < rhs_.size(); ++i)
    {
        me::Literal* literal = dynamic_cast<me::Literal*>(rhs_[i].op_);

        if ( literal )
            delete literal;
    }
}

/*
 * further methods
 */

bool InstrBase::isRegUsed(Reg* reg) 
{
    // for each var on the rhs
    for (size_t i = 0; i < rhs_.size(); ++i) 
    {
        if (rhs_[i].op_ == reg)
            return true;
    }

    // -> not found
    return false;
}

bool InstrBase::isRegDefined(Reg* reg) 
{
    // for each var on the lhs
    for (size_t i = 0; i < lhs_.size(); ++i) 
    {
        if (lhs_[i].reg_ == reg)
            return true;
    }

    // -> not found
    return false;
}

struct ResFinder
{
    Reg* reg_;

    ResFinder(Reg* reg)
        : reg_(reg)
    {}

    bool operator () (const me::InstrBase::Res& res)
    {
        return reg_ == res.reg_;
    }
};

struct ArgFinder
{
    Op* op_;

    ArgFinder(Op* op)
        : op_(op)
    {}

    bool operator () (const me::InstrBase::Arg& arg)
    {
        return op_ == arg.op_;
    }
};

Reg* InstrBase::findResult(Reg* reg)
{
    LHS::const_iterator iter = std::find_if( lhs_.begin(), lhs_.end(), ResFinder(reg) );

    if ( iter == lhs_.end() )
        return false;

    return iter->reg_;
}

Op* InstrBase::findArg(Op* op)
{
    RHS::const_iterator iter = std::find_if( rhs_.begin(), rhs_.end(), ArgFinder(op) );

    if ( iter == rhs_.end() )
        return false;

    return iter->op_;
}

bool InstrBase::isConstrainted() const
{
    return constrainted_;
}

void InstrBase::constraint()
{
    constrainted_ = true;
}

InstrBase::OpType InstrBase::getOpType(size_t i) const
{
    if ( typeid(*rhs_[i].op_) == typeid(me::Literal) )
        return CONST;

    swiftAssert( typeid(*rhs_[i].op_) == typeid(Reg), "must be a Reg" );
    Reg* reg = (Reg*) rhs_[i].op_;

    if ( liveOut_.find(reg) == liveOut_.end() )
        return REG_DEAD;
    else
        return REG;
}

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
    : InstrBase(0, 0)
{
    std::ostringstream oss;
    oss << "L" << number2String(counter_);
    label_ = oss.str();

    ++counter_;
}

//------------------------------------------------------------------------------

/*
 * constructors
 */

NOP::NOP(Op* op)
    : InstrBase(0, 1)
{
    rhs_[0].op_ = op;
}

/*
 * further methods
 */

std::string NOP::toString() const
{
    std::ostringstream oss;
    oss << "NOP(";

    for (size_t i = 0; i < rhs_.size() - 1; ++i)
    {
        if (rhs_[i].op_)
            oss << rhs_[i].op_->toString() << ", ";
    }

    if (rhs_[rhs_.size() - 1].op_ )
        oss << rhs_[rhs_.size()-1].op_->toString() << ')';

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

PhiInstr::PhiInstr(Reg* result, size_t numRhs)
    : InstrBase(1, numRhs) // phi functions always have one result
    , sourceBBs_( new BBNode*[numRhs] )
{
    // init result
    lhs_[0].reg_ = result;
    lhs_[0].oldVarNr_ = result->varNr_;

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
    return lhs_[0].reg_;
}

const Reg* PhiInstr::result() const
{
    return lhs_[0].reg_;
}

int PhiInstr::oldResultNr() const
{
    return lhs_[0].oldVarNr_;
}

/*
    further methods
*/

std::string PhiInstr::toString() const
{
    std::ostringstream oss;
    oss << result()->toString() << "\t= phi(";

    for (size_t i = 0; i < rhs_.size() - 1; ++i)
    {
        if (!sourceBBs_[i])
            continue;

        if (rhs_[i].op_ && sourceBBs_[i])
            oss << rhs_[i].op_->toString() << " (" << sourceBBs_[i]->value_->name() << "), ";
        else
            oss << "-, ";
    }

    if (rhs_[rhs_.size() - 1].op_ && sourceBBs_[rhs_.size() - 1])
        oss << rhs_[rhs_.size()-1].op_->toString() << '(' << sourceBBs_[rhs_.size()-1]->value_->name()  << ')';
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
    : InstrBase(1, op2 ? 2 : 1) 
    , kind_(kind)
{
    lhs_[0].reg_ = result;
    lhs_[0].oldVarNr_= result->varNr_;


    rhs_[0].op_ = op1;

    if (op2)
        rhs_[1].op_ = op2;
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
    oss << lhs_[0].reg_->toString() << '\t';

    // is this a binary, an unary instruction or an assignment?
    if (rhs_.size() == 2)
    {
        // it is a binary instruction
        oss << "= " << rhs_[0].op_->toString() << " " << opString << " " << rhs_[1].op_->toString();
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
        oss << ' ' << rhs_[0].op_->toString();
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
    : InstrBase(numLhs, numRhs)
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
    rhs_[0].op_ = boolOp;

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
    oss << "IF " << rhs_[0].op_->toString()
        << " THEN " << trueLabel()->toString()
        << " ELSE " << falseLabel()->toString();

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Spill::Spill(Reg* result, Reg* arg)
    : InstrBase(1, 1)
{
    swiftAssert( !arg->isMem(), "arg must not be a memory var" );
    swiftAssert( result->isMem(), "result must be a memory var" );
    lhs_[0].reg_ = result;
    rhs_[0].op_  = arg;
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
    oss << lhs_[0].reg_->toString() << "\t= spill(" << rhs_[0].op_->toString() << ")";

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Reload::Reload(Reg* result, Reg* arg)
    : InstrBase(1, 1)
{
    swiftAssert( arg->isMem(), "arg must be a memory var" );
    swiftAssert( !result->isMem(), "result must not be a memory var" );
    lhs_[0].reg_ = result;
    rhs_[0].op_ = arg;
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
    oss << lhs_[0].reg_->toString() << "\t= reload(" << rhs_[0].op_->toString() << ")";

    return oss.str();
}

} // namespace me
