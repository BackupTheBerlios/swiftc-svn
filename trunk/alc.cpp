#include "alc.h"

#include <sstream>

#include "expr.h"
#include "symboltable.h"

namespace swift {

InstrList instrlist;

// -----------------------------------------------------------------------------

std::string ModuleTagInstr::toString() const
{
    std::ostringstream oss;

    if (enter_)
        oss << "enter ";
    else
        oss << "leave ";

    oss << "module " << *id_;

    return oss.str();
}

void ModuleTagInstr::enter()
{
    symtab.enterModule(/*id_*/);
}

void ModuleTagInstr::leave()
{
    symtab.leaveModule();
}

// -----------------------------------------------------------------------------

std::string ClassTagInstr::toString() const
{
    std::ostringstream oss;

    if (enter_)
        oss << "enter ";
    else
        oss << "leave ";

    oss << "class " << *id_;

    return oss.str();
}

void ClassTagInstr::enter()
{
    symtab.enterClass(id_);
}

void ClassTagInstr::leave()
{
    symtab.leaveClass();
}

// -----------------------------------------------------------------------------

std::string MethodTagInstr::toString() const
{
    std::ostringstream oss;

    if (enter_)
        oss << "enter ";
    else
        oss << "leave ";

    oss << "method " << *id_;

    return oss.str();
}

void MethodTagInstr::enter()
{
    symtab.enterMethod(id_);
}

void MethodTagInstr::leave()
{
    symtab.leaveMethod();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

std::string AssignInstr::toString() const
{
    std::ostringstream oss;
    oss << result_->toString() << c_ << expr_->toString();

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
    oss << result_->toString() << " = " << c_ << op_->toString();

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
    oss << result_->toString() << " = " << op1_->toString() << " " << c_ << " " << op2_->toString();

    return oss.str();
}

void BinInstr::genCode(std::ofstream& ofs)
{
    ofs << "bi" << std::endl;
};

//------------------------------------------------------------------------------

} // namespace swift
