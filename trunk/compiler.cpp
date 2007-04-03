#include "compiler.h"

#include <iostream>
#include <fstream>
#include <typeinfo>

#include "utils/assert.h"
#include "me/scopetable.h"
#include "me/ssa.h"

bool Compiler::genCode()
{
}

bool Compiler::optimize()
{
    return true;
}

bool Compiler::buildAssemblyCode()
{
    return true;
}

std::string Compiler::toString()
{
    return syntaxTree_.rootModule_->toString();
}
