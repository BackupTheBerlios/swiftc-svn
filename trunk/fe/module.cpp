#include "module.h"

#include <iostream>

#include "fe/class.h"

Module::~Module()
{
    // for each definition
    for (DefinitionList::Node* iter = definitions_.first(); iter != definitions_.sentinel(); iter = iter->next())
        delete iter->value_;
}

std::string Module::toString() const
{
/*    std::ostringstream oss;

    for (Definition* iter = definitions_; iter != 0; iter = iter->next_)
        oss << iter->toString() << std::endl;

    return oss.str();*/

    return *id_;
}

bool Module::analyze()
{
    bool result = true;

    // for each definition
    for (DefinitionList::Node* iter = definitions_.first(); iter != definitions_.sentinel(); iter = iter->next())
        result &= iter->value_->analyze();

    return result;
}
