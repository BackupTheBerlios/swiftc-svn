#include "fe/typelist.h"

#include <utils/stringhelper.h>

#include "fe/type.h"

namespace swift {

std::string TypeList::toString() const
{
    return commaListPtr( begin(), end() ); 
}

} // namespace swift
