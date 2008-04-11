#include "ssa.h"

#include <iostream>
#include <sstream>

#include "utils/stringhelper.h"

#include "me/functab.h"
#include "me/op.h"

namespace me {

//------------------------------------------------------------------------------

bool InstrBase::isLastUse(InstrNode* instrNode, Reg* var)
{
    InstrBase* instr = instrNode->value_;
    InstrBase* prev  = instrNode->prev()->value_;

    return instr->liveOut_.find(var) == instr->liveOut_.end()  // mustn't be in the current instruction
        &&  prev->liveOut_.find(var) !=  prev->liveOut_.end(); // must be in the previous one
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

std::string GotoInstr::toString() const
{
    std::ostringstream oss;
    oss << "GOTO " << label()->toString();

    return oss.str();
}

//------------------------------------------------------------------------------

/*
    constructor and destructor
*/

AssignmentBase::AssignmentBase(size_t numLhs, size_t numRhs)
    : lhs_( new Reg*[numLhs] )
    , lhsOldVarNr_( new int[numLhs] )
    , numLhs_(numLhs)
    , rhs_( new Op*[numRhs] )
    , numRhs_(numRhs)
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

    delete[] lhs_;
    delete[] rhs_;

    destroyLhsOldVarNrs(); // TODO this should be done earlier
}

/*
    further methods
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

//------------------------------------------------------------------------------

/*
    constructor and destructor
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
    getters and setters
*/

// Reg* PhiInstr::result()
// {
//     return lhs_[0];
// }
//
// const Reg* PhiInstr::result() const
// {
//     return lhs_[0];
// }
//
// int PhiInstr::resultOldVarNr() const
// {
//     return lhsOldVarNr_[0];
// }

/*
    further methods
*/

std::string PhiInstr::toString() const
{
    std::ostringstream oss;
    oss << lhs_[0]->toString() << "\t= phi(";

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
    constructor
*/

AssignInstr::AssignInstr(int kind, Reg* result, Op* op1, Op* op2 /*= 0*/)
    : AssignmentBase(1, op2 ? 2 : 1) // An AssignInstr always have exactly one result and one or two args
    , kind_(kind)
{
    lhs_[0] = result;
    lhsOldVarNr_[0] = result->varNr_;

    rhs_[0] = op1;
    if (op2)
        rhs_[1] = op2;
}

/*
    further methods
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
    constructor
*/

BranchInstr::BranchInstr(Op* boolOp, InstrNode* trueLabelNode, InstrNode* falseLabelNode)
    : AssignmentBase(0, 1) // BranchInstr always have exactly one rhs var and no results
    , trueLabelNode_(trueLabelNode)
    , falseLabelNode_(falseLabelNode)
{
    swiftAssert(boolOp->type_ == Reg::R_BOOL, "this is not a boolean pseudo reg");
    rhs_[0] = boolOp;

    swiftAssert( typeid(*trueLabelNode->value_) == typeid(LabelInstr),
        "trueLabelNode must be a node to a LabelInstr");
    swiftAssert( typeid(*falseLabelNode->value_) == typeid(LabelInstr),
        "falseLabelNode must be a node to a LabelInstr");
}

/*
    further methods
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

/*std::string InvokeInstr::toString() const
{
    std::string result = "INVOKE ";
    result += *function_->id_;

    result += "IN: ";
    for (const RegList::Node* iter = in_.first(); iter != in_.sentinel(); iter = iter->next())
    {
        result += iter->value_->toString();
        result += ' ';
    }

    result += "INOUT: ";
    for (const RegList::Node* iter = inout_.first(); iter != inout_.sentinel(); iter = iter->next())
    {
        result += iter->value_->toString();
        result += ' ';
    }

    result += "OUT: ";
    for (const RegList::Node* iter = out_.first(); iter != out_.sentinel(); iter = iter->next())
    {
        result += iter->value_->toString();
        result += ' ';
    }

    return result;
}
*/

//------------------------------------------------------------------------------

/*
    constructor
*/

SpillInstr::SpillInstr()
    : AssignmentBase(1, 1) // SpillInstr always have exactly one result and one arg
{}

/*
    getters
*/

Reg* SpillInstr::arg()
{
    swiftAssert( typeid(*lhs_[0]) == typeid(Reg), "lhs_[0] must be a Reg*" );
    return (Reg*) lhs_[0];
}

const Reg* SpillInstr::arg() const
{
    swiftAssert( typeid(*lhs_[0]) == typeid(Reg), "lhs_[0] must be a Reg*" );
    return (Reg*) lhs_[0];
}

//------------------------------------------------------------------------------

/*
    constructor
*/

Spill::Spill(Reg* result, Reg* arg)
    : SpillInstr()
{
    swiftAssert( result->isMem(),  "result must be a memory var" );
    swiftAssert( !arg->isMem(), "arg must not be a memory var" );

    lhs_[0] = result;
    rhs_[0] = arg;
}

/*
    further methods
*/

void Spill::genCode(std::ofstream& /*ofs*/)
{
    // TODO
}

std::string Spill::toString() const
{
    return "TODO";
}

//------------------------------------------------------------------------------

/*
    constructor
*/

Reload::Reload(Reg* result, Reg* arg)
    : SpillInstr()
{
    swiftAssert( !result->isMem(), "result must not be a memory var");
    swiftAssert( arg->isMem(), "arg must be a memory var");

    lhs_[0] = result;
    rhs_[0] = arg;
}

/*
    further methods
*/

void Reload::genCode(std::ofstream& /*ofs*/)
{
    // TODO
}

std::string Reload::toString() const
{
    return "TODO";
}

} // namespace me
