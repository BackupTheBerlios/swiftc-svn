#include "module.h"

#include <iostream>

#include "fe/class.h"

/*
    constructor and destructor
*/

Module::Module(std::string* id, int line /*= NO_LINE*/, Node* parent /*= 0*/)
    : Node(line, parent)
    , id_(id)
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

Definition::Definition(std::string* id, int line, Node* parent /*= 0*/)
    : Symbol(id, line, parent)
{}
