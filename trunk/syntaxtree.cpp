#include "syntaxtree.h"

#include <iostream>
#include <sstream>

#include "assert.h"
#include "ssa.h"

namespace swift
{

//------------------------------------------------------------------------------

std::string SymTabEntry::extractOriginalId() {
    swiftAssert( revision_ == -1, "This is not a revised variable" );

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
    instrlist.append( new ModuleTagInstr(id_, true) );

    // for each definition
    for (Definition* iter = definitions_; iter != 0; iter = iter->next_)
        iter->analyze();

    instrlist.append( new ModuleTagInstr(id_, false) );

    return true;
}

} // namespace swift
