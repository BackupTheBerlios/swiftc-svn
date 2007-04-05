#include "syntaxtree.h"

#include <iostream>
#include <sstream>

#include "utils/assert.h"

#include "fe/module.h"
#include "fe/symtab.h"

#include "me/ssa.h"


SyntaxTree* syntaxtree = 0;

//------------------------------------------------------------------------------

SyntaxTree::~SyntaxTree()
{
    delete rootModule_;
}

bool SyntaxTree::analyze()
{
    symtab->reset();
    symtab->enterModule();

    bool result = rootModule_->analyze();

    symtab->leaveModule();

    return result;
}
