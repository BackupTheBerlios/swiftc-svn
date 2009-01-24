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
// helpers
//------------------------------------------------------------------------------

std::string args2str(const InstrBase::RHS& arg)
{
    if ( arg.empty() )
        return "";

    std::ostringstream oss;

    for (size_t i = 0; i < arg.size() - 1; ++i)
        oss << arg[i].op_->toString() << ", ";

    oss << arg[ arg.size() - 1 ].op_->toString();

    return oss.str();
}

std::string res2str(const InstrBase::LHS& res)
{
    if ( res.empty() )
        return "";

    std::ostringstream oss;

    for (size_t i = 0; i < res.size() - 1; ++i)
        oss << res[i].reg_->toString() << ", ";

    oss << res[ res.size() - 1 ].reg_->toString();

    return oss.str();
}


//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

InstrBase::InstrBase(size_t numLhs, size_t numRhs)
    : res_(numLhs)
    , arg_(numRhs)
    , constrained_(false)
{
    for (size_t i = 0; i < res_.size(); ++i)
        res_[i].constraint_ = NO_CONSTRAINT;

    for (size_t i = 0; i < arg_.size(); ++i)
        arg_[i].constraint_ = NO_CONSTRAINT;
}

InstrBase::~InstrBase()
{
    // delete all consts and undefs
    for (size_t i = 0; i < arg_.size(); ++i)
    {
        if (       typeid(*arg_[i].op_) == typeid(Const) 
                || typeid(*arg_[i].op_) == typeid(Undef) )
        {
            delete arg_[i].op_;
        }
    }
}

/*
 * further methods
 */

bool InstrBase::isRegUsed(Reg* reg) 
{
    // for each var on the arg
    for (size_t i = 0; i < arg_.size(); ++i) 
    {
        if (arg_[i].op_ == reg)
            return true;
    }

    // -> not found
    return false;
}

bool InstrBase::isRegDefined(Reg* reg) 
{
    // for each var on the res
    for (size_t i = 0; i < res_.size(); ++i) 
    {
        if (res_[i].reg_ == reg)
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

    bool operator () (const Res& res)
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

    bool operator () (const Arg& arg)
    {
        return op_ == arg.op_;
    }
};

Reg* InstrBase::findResult(Reg* reg)
{
    LHS::const_iterator iter = std::find_if( res_.begin(), res_.end(), ResFinder(reg) );

    if ( iter == res_.end() )
        return false;

    return iter->reg_;
}

Op* InstrBase::findArg(Op* op)
{
    RHS::const_iterator iter = std::find_if( arg_.begin(), arg_.end(), ArgFinder(op) );

    if ( iter == arg_.end() )
        return false;

    return iter->op_;
}

bool InstrBase::isConstrained() const
{
    return constrained_;
}

void InstrBase::constrain()
{
    constrained_ = true;
}

void InstrBase::unconstrainIfPossible()
{
    for (size_t i = 0; i < res_.size(); ++i)
    {
        if (res_[i].constraint_ != NO_CONSTRAINT)
            return;
    }

    for (size_t i = 0; i < arg_.size(); ++i)
    {
        if (arg_[i].constraint_ != NO_CONSTRAINT)
            return;
    }

    // all constraints were removed from this instruction
    constrained_ = false;
}

bool InstrBase::livesThrough(Reg* reg) const
{
    return liveIn_.contains(reg) && liveOut_.contains(reg);
}

InstrBase::OpType InstrBase::getOpType(size_t i) const
{
    if ( typeid(*arg_[i].op_) == typeid(Const) )
        return CONST;

    swiftAssert( typeid(*arg_[i].op_) == typeid(Reg), "must be a Reg" );
    Reg* reg = (Reg*) arg_[i].op_;

    if ( !liveOut_.contains(reg) )
        return REG_DEAD;
    else
        return REG;
}

bool InstrBase::isLastUse(InstrNode* instrNode, Reg* var)
{
    InstrBase* instr = instrNode->value_;

    return   instr->liveIn_ .contains(var)  // must be in the live in
        &&  !instr->liveOut_.contains(var); // but not in the live out
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
int LabelInstr::counter_ = 1;

LabelInstr::LabelInstr()
    : InstrBase(0, 0)
{
    std::ostringstream oss;
    oss << "L" << number2String(counter_);
    label_ = oss.str();

    ++counter_;
}

std::string LabelInstr::asmName() const
{
    return std::string(".") + label_;
}

//------------------------------------------------------------------------------

/*
 * constructors
 */

NOP::NOP(Op* op)
    : InstrBase(0, 1)
{
    arg_[0].op_ = op;
}

/*
 * further methods
 */

std::string NOP::toString() const
{
    std::ostringstream oss;
    oss << "NOP(";

    for (size_t i = 0; i < arg_.size() - 1; ++i)
    {
        if (arg_[i].op_)
            oss << arg_[i].op_->toString() << ", ";
    }

    if (arg_[arg_.size() - 1].op_ )
        oss << arg_[arg_.size()-1].op_->toString() << ')';

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
    res_[0].reg_ = result;
    res_[0].oldVarNr_ = result->varNr_;

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
    return res_[0].reg_;
}

const Reg* PhiInstr::result() const
{
    return res_[0].reg_;
}

int PhiInstr::oldResultNr() const
{
    return res_[0].oldVarNr_;
}

/*
    further methods
*/

std::string PhiInstr::toString() const
{
    std::ostringstream oss;
    // u03D5 is the unicode sign for a the greek phi letter
    oss << result()->toString() << "\t= \u03D5("; 

    for (size_t i = 0; i < arg_.size() - 1; ++i)
    {
        if (!sourceBBs_[i])
            continue;

        if (arg_[i].op_ && sourceBBs_[i])
            oss << arg_[i].op_->toString() << " (" << sourceBBs_[i]->value_->name() << "), ";
        else
            oss << "-, ";
    }

    if (arg_[arg_.size() - 1].op_ && sourceBBs_[arg_.size() - 1])
        oss << arg_[arg_.size()-1].op_->toString() << '(' << sourceBBs_[arg_.size()-1]->value_->name()  << ')';
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
    res_[0].reg_ = result;
    res_[0].oldVarNr_= result->varNr_;


    arg_[0].op_ = op1;

    if (op2)
        arg_[1].op_ = op2;
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
            opString = "\\>=";
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

bool AssignInstr::isComparison() const
{
    return (kind_ == EQ  || kind_ == NE  || 
            kind_ == '<' || kind_ == '>' ||
            kind_ == LE  || kind_ == GE);
}

bool AssignInstr::isArithmetic() const
{
    return (kind_ == '+' || kind_ == '-' || 
            kind_ == '*' || kind_ == '/');
}

std::string AssignInstr::toString() const
{
    std::string opString = getOpString();
    std::ostringstream oss;
    oss << res_[0].reg_->toString();

    for (size_t i = 1; i < res_.size(); ++i)
        oss << ", " << res_[i].reg_->toString();

    oss << '\t';

    // FIXME not accurate with dummy args

    // is this a binary, an unary instruction or an assignment?
    if (arg_.size() >= 2)
    {
        // it is a binary instruction
        oss << "= " << arg_[0].op_->toString() << " " << opString << " " << arg_[1].op_->toString();

        for (size_t i = 2; i < arg_.size(); ++i)
            oss << ", " << arg_[i].op_->toString();
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
        oss << ' ' << arg_[0].op_->toString();
    }

    if ( isConstrained() )
        oss << " (c)";

    return oss.str();
}

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
    : JumpInstr(0, 0, 1) // always no res, no arg, one target
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
    : JumpInstr(0, 1, 2) // always no results, one arg, two targets
    , cc_(CC_NOT_SET)
{
    swiftAssert(boolOp->type_ == Reg::R_BOOL, "this is not a boolean pseudo reg");
    arg_[0].op_ = boolOp;

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

Op* BranchInstr::getOp()
{
    return arg_[0].op_;
}

const Op* BranchInstr::getOp() const
{
    return arg_[0].op_;
}

/*
 * further methods
 */

std::string BranchInstr::toString() const
{
    std::ostringstream oss;
    oss << "IF " << arg_[0].op_->toString()
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
    res_[0].reg_ = result;
    arg_[0].op_  = arg;
}

/*
 * further methods
 */

std::string Spill::toString() const
{
    std::ostringstream oss;
    oss << res_[0].reg_->toString() << "\t= spill(" << arg_[0].op_->toString() << ")";

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
    res_[0].reg_ = result;
    arg_[0].op_ = arg;
}

/*
 * further methods
 */

std::string Reload::toString() const
{
    std::ostringstream oss;
    oss << res_[0].reg_->toString() << "\t= reload(" << arg_[0].op_->toString() << ")";

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

SetParams::SetParams(size_t numLhs)
    : InstrBase(numLhs, 0)
{}

/*
 * further methods
 */

std::string SetParams::toString() const
{
    swiftAssert( res_.size() >= 1, "must have at least one res" );
    
    std::ostringstream oss;
    oss << res2str(res_) << "\t= setParams()";

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

SetResults::SetResults(size_t numRhs)
    : InstrBase(0, numRhs)
{}

/*
 * further methods
 */

std::string SetResults::toString() const
{
    swiftAssert( arg_.size() >= 1, "must have at least one arg" );

    std::ostringstream oss;
    oss << "setResults(" << args2str(arg_) << ')';

    return oss.str();
}

} // namespace me
