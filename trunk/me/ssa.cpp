#include "ssa.h"

#include <iostream>
#include <sstream>

#include "utils/stringhelper.h"

#include "me/functab.h"

//------------------------------------------------------------------------------

int LabelInstr::counter_ = 0;

LabelInstr::LabelInstr()
{
    std::ostringstream oss;
    oss << "L" << number2String(counter_);
    label_ = oss.str();

    ++counter_;
}

//------------------------------------------------------------------------------

std::string PhiInstr::toString() const
{
    std::ostringstream oss;
    oss << result_->toString() << "\t= phi(";

    for (size_t i = 0; i < argc_ - 1; ++i)
    {
        if (args_[i])
            oss << args_[i]->toString() << " (" << sourceBBs_[i]->value_->name() << "), ";
        else
            oss << "-, ";
    }

    if (args_[argc_ - 1])
        oss << args_[argc_-1]->toString() << '(' << sourceBBs_[argc_-1]->value_->name()  << ')';
    else
        oss << '-';

    oss << ')';

    return oss.str();
}

//------------------------------------------------------------------------------

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
        default:
            opString = c_;
    }

    return opString;
}

std::string AssignInstr::toString() const
{
    std::string opString = getOpString();
    std::ostringstream oss;
    oss << result_->toString() << '\t';

    // is this a binary, an unary instruction or an assignment?
    if (op2_)
    {
        // it is a binary instruction
        oss << "= " << op1_->toString() << " " << opString << " " << op2_->toString();
    }
    else
    {
        if ( isUnaryInstr() )
        {
            // it is an unary instruction
            oss << "= " << opString;
        }
        else
        {
            // it is an assignment
            oss << opString;
        }
        oss << ' ' << op1_->toString();
    }

    return oss.str();
}

void AssignInstr::genCode(std::ofstream& ofs)
{
    ofs << "bi" << std::endl;
};

//------------------------------------------------------------------------------

std::string GotoInstr::toString() const
{
    std::ostringstream oss;
    oss << "GOTO " << label()->toString();

    return oss.str();
}

//------------------------------------------------------------------------------

std::string BranchInstr::toString() const
{
    std::ostringstream oss;
    oss << "IF " << boolReg_->toString()
        << " THEN " << trueLabel()->toString()
        << " ELSE " << falseLabel()->toString();

    return oss.str();
}

//------------------------------------------------------------------------------

std::string InvokeInstr::toString() const
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
