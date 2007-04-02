#include "ssa.h"

#include <iostream>
#include <sstream>

#include "fe/expr.h"
#include "fe/symboltable.h"
#include "scopetable.h"

void EnterScopeInstr::updateScoping()
{
    if (enter_)
        scopetab.enter(scope_);
    else
        scopetab.leave();
}

// -----------------------------------------------------------------------------

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
    std::ostringstream oss;
    oss << result_->toString() << "\t= " << op1_->toString() << " " << c_ << " " << op2_->toString();

    return oss.str();
}

void BinInstr::genCode(std::ofstream& ofs)
{
    ofs << "bi" << std::endl;
};
