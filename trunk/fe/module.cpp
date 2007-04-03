#include "module.h"

#include <iostream>

#include "fe/class.h"

Module::~Module()
{
    delete definitions_;

/*    for (ClassMap::iterator iter = classes_.begin(); iter != classes_.end(); ++iter)
        delete iter->second;*/
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
    // for each definition
    for (Definition* iter = definitions_; iter != 0; iter = iter->next_)
        iter->analyze();

    return true;
}
