#include "syntaxtree.h"

#include <iostream>
#include <sstream>

#include "utils/assert.h"
#include "me/ssa.h"

//------------------------------------------------------------------------------

std::string SymTabEntry::extractOriginalId() {
    swiftAssert( revision_ == REVISED_VAR, "This is not a revised variable" );

    // reverse search should usually be faster
    size_t index = id_->find_last_of('!');

    return id_->substr(0, index);
}

//------------------------------------------------------------------------------

std::string Module::toString() const
{
/*    std::ostringstream oss;

    for (Definition* iter = definitions_; iter != 0; iter = iter->next_)
        oss << iter->toString() << std::endl;

    return oss.str();*/

    return *id_;
}

//------------------------------------------------------------------------------

bool Module::analyze()
{
    // for each definition
    for (Definition* iter = definitions_; iter != 0; iter = iter->next_)
        iter->analyze();

    return true;
}
