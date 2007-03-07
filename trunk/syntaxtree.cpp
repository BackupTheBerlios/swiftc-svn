#include "syntaxtree.h"

#include <iostream>
#include <sstream>

#include "assert.h"
#include "alc.h"

namespace swift
{

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
    instrlist.append( new ModuleTagInstr(id_, true) );

    // for each definition
    for (Definition* iter = definitions_; iter != 0; iter = iter->next_)
        iter->analyze();

    instrlist.append( new ModuleTagInstr(id_, false) );

    return true;
}

} // namespace swift

#undef SWIFT_TO_STRING_ERROR
