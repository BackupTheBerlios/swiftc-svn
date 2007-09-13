#include "type.h"

#include <iostream>
#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

#include "fe/error.h"
#include "fe/expr.h"
#include "fe/symtab.h"


std::string Type::toString() const
{
    std::ostringstream oss;

    oss << baseType_->toString();
    for (int i = 0; 0 < pointerCount_; ++i)
        oss << '^';

    return oss.str();
}

/**
 * type checking <br>
*/
bool Type::check(Type* t1, Type* t2)
{
    if (t1->pointerCount_ != t2->pointerCount_)
        return false; // pointerCount_ does not match

    Class* class1 = symtab->lookupClass(t1->baseType_->id_);
    Class* class2 = symtab->lookupClass(t2->baseType_->id_);

    // both classes must exist
    swiftAssert(class1, "first class not found in the symbol table");
    swiftAssert(class2, "second class not found in the symbol table");

    if (class1 != class2) {
        // different pointers -> hence different types
        return false;
    }

    return true;
}

/**
 * checks whether this is a valid type
*/
bool Type::validate()
{
    if ( symtab->lookupClass(baseType_->id_) == 0 )
    {
        errorf( line_, "class '%s' is not defined in this module", baseType_->id_->c_str() );
        return false;
    }

    return true;
}

bool Type::isBool()
{
    return *baseType_->id_ == "bool";
}

PseudoReg::RegType BaseType::toRegType() const
{
    const std::string& id = *id_;

    if (id == "index")
        return PseudoReg::R_INDEX;
    else if (id == "INT")
        return PseudoReg::R_INT;
    else if (id == "int8")
        return PseudoReg::R_INT8;
    else if (id == "int16")
        return PseudoReg::R_INT16;
    else if (id == "int32")
        return PseudoReg::R_INT32;
    else if (id == "int64")
        return PseudoReg::R_INT64;
    else if (id == "sat8")
        return PseudoReg::R_SAT8;
    else if (id == "sat16")
        return PseudoReg::R_SAT16;

    else if (id == "uint")
        return PseudoReg::R_UINT;
    else if (id == "uint8")
        return PseudoReg::R_UINT8;
    else if (id == "uint16")
        return PseudoReg::R_UINT16;
    else if (id == "uint32")
        return PseudoReg::R_UINT32;
    else if (id == "uint64")
        return PseudoReg::R_UINT64;
    else if (id == "usat8")
        return PseudoReg::R_USAT8;
    else if (id == "usat16")
        return PseudoReg::R_USAT16;

    else if (id == "real")
        return PseudoReg::R_REAL;
    else if (id == "real32")
        return PseudoReg::R_REAL32;
    else if (id == "real64")
        return PseudoReg::R_REAL64;

    else if (id == "bool")
        return PseudoReg::R_BOOL;
    else
        return PseudoReg::R_STRUCT;
}
