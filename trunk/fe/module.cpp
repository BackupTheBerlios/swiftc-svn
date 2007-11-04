#include "module.h"

#include <iostream>

#include "fe/class.h"

/*
    constructor and destructor
*/

Module::Module(std::string* id, int line /*= NO_LINE*/)
    : Symbol(id, 0, line) // modules don't have parents
{}

Module::~Module()
{
    // for each definition
    for (DefinitionList::Node* iter = definitions_.first(); iter != definitions_.sentinel(); iter = iter->next())
        delete iter->value_;
}

/*
    further methods
*/

bool Module::analyze()
{
    bool result = true;

    // for each definition
    for (DefinitionList::Node* iter = definitions_.first(); iter != definitions_.sentinel(); iter = iter->next())
        result &= iter->value_->analyze();

    return result;
}

std::string Module::toString() const
{
    return *id_;
}

//------------------------------------------------------------------------------

/*
    constructor
*/

Definition::Definition(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : Symbol(id, parent, line)
{}
