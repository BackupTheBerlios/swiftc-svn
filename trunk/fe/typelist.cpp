#include "fe/typelist.h"

#include <utils/stringhelper.h>

#include "fe/type.h"

namespace swift {

const TypeItem::Type* getType() const
{
    return type_;
}

bool  TypeItem::isReadOnly() const
{
    if (modifier_ == CONST || modifier_ == NOT_INOUT)
        return true;

    swiftAssert(modifier_ == NOT_CONST || modifier_ == INOUT,
            "must be NOT_CONST or INOUT_ here");
    return false;
}

std::string TypeItem::toString() const
{
    std::string result;

    if (modifier_ == CONST)
        result = "const ";
    else if (modifier_ == INOUT)
        result = "inout ";

    result += type_->toString();

    return result; 
}

//------------------------------------------------------------------------------ 

std::string TypeList::toString() const
{
    return commaListPtr( begin(), end() ); 
}

} // namespace swift
