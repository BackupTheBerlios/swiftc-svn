#include "ssa.h"

#include <iostream>
#include <sstream>

#include "me/functab.h"

//------------------------------------------------------------------------------

int LabelInstr::counter_ = 0;

LabelInstr::LabelInstr()
{
    std::ostringstream oss;
    oss << "?L" << counter_;
    label_ = oss.str();

    ++counter_;
}

//------------------------------------------------------------------------------

std::string AssignInstr::toString() const
{
    std::ostringstream oss;
    oss << result_->toString() << "\t" << c_ << " " << reg_->toString();

    return oss.str();
}

void AssignInstr::genCode(std::ofstream& ofs)
{
    ofs << "ai" << std::endl;
};

//------------------------------------------------------------------------------

std::string UnInstr::toString() const
{
    std::ostringstream oss;
    oss << result_->toString() << "\t= " << c_ << op_->toString();

    return oss.str();
}

void UnInstr::genCode(std::ofstream& ofs)
{
    ofs << "ui" << std::endl;
};

//------------------------------------------------------------------------------

std::string BinInstr::toString() const
{
    std::string opString;
    switch (kind_)
    {
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
    std::ostringstream oss;
    oss << result_->toString() << "\t= " << op1_->toString() << " " << opString << " " << op2_->toString();

    return oss.str();
}

void BinInstr::genCode(std::ofstream& ofs)
{
    ofs << "bi" << std::endl;
};

//------------------------------------------------------------------------------

std::string GotoInstr::toString() const
{
    std::ostringstream oss;
    oss << "GOTO " << label_->toString();

    return oss.str();
}

//------------------------------------------------------------------------------

std::string BranchInstr::toString() const
{
    std::ostringstream oss;
    oss << "IF " << boolReg_->toString()
        << " THEN " << trueLabel_->toString()
        << " ELSE " << falseLabel_->toString();

    return oss.str();
}
