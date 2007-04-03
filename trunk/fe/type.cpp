#include "type.h"

#include <iostream>
#include <sstream>

#include "utils/assert.h"

#include "fe/error.h"
#include "fe/expr.h"
#include "fe/symtab.h"


std::string Type::toString() const
{
    std::ostringstream oss;

    switch (typeQualifier_)
    {
        case VAR: oss << "var "; break;
        case CST: oss << "cst "; break;
        case DEF: oss << "def "; break;
        default:
            swiftAssert(false, "illegal case value");
            return "";
    }

    oss << baseType_->toString();
    for (int i = 0; 0 < pointerCount_; ++i)
        oss << '^';

    return oss.str();
}

/**
 * type checking <br>
 * This function does _NOT_ check the typeQualifier_
*/
bool Type::check(Type* t1, Type* t2)
{
    if (   t1->pointerCount_ != t2->pointerCount_
        || typeid(*t1->baseType_) != typeid(*t2->baseType_) )
        return false;

    /*
        Hence both types are of the same base type.
        Thus check all possibilities.
    */

    if ( typeid(*t1->baseType_) == typeid(SimpleType) )
    {
        if ( ((SimpleType*) t1->baseType_)->kind_ == ((SimpleType*) t2->baseType_)->kind_ )
            return true;
        else
            return false;
    }

    if ( typeid(*t1->baseType_) == typeid(Container) )
    {
        Container* container1 = (Container*) t1->baseType_;
        Container* container2 = (Container*) t2->baseType_;

        // check whether we have an array or an simd container
        if (container1->kind_ != container2->kind_)
            return false;

        if ( !check(container1->type_, container2->type_) )
            return false;

        if ( container1->expr_ == 0 && container2->expr_ == 0)
            return true;

        std::cout << "TODO in line " << t1->line_ << std::endl;
    }

    if ( typeid(*t1->baseType_) == typeid(UserType) )
    {
        UserType* u1 = (UserType*) t1->baseType_;
        UserType* u2 = (UserType*) t2->baseType_;

        Class* class1 = symtab.lookupClass(u1->id_);
        Class* class2 = symtab.lookupClass(u2->id_);

        // both classes must exist
        swiftAssert(class1, "first class not found in the symbol table");
        swiftAssert(class2, "second class not found in the symbol table");

        if (class1 != class2) {
            // different pointers -> hence different types
            return false;
        }

        // TODO templates
        return true;
    }

    swiftAssert(false, "impossible type-combo");

    return false;
}

/**
 * checks whether this is a valid type
*/
bool Type::validate()
{
    if ( typeid(*baseType_) == typeid(SimpleType) )
        return true;

    if ( typeid(*baseType_) == typeid(Container) )
    {
        Container* container = (Container*) baseType_;

        bool result = container->type_->validate();

        if ( !result )
            return false;

        if (container->expr_ == 0)
            return true;

        std::cout << "TODO: test def expr of conatainer. returning false" << std::endl;

        return false;
    }

    if ( typeid(*baseType_) == typeid(UserType) )
    {
        UserType* userType = (UserType*) baseType_;
        if (symtab.lookupClass( userType->id_ ) == 0)
        {
            errorf( userType->line_, "class '%s' is not defined in this module", userType->id_->c_str() );
            return false;
        }

        // TODO templates

        return true;
    }

    swiftAssert(false, "strange type given");
    return false;
}

/**
 * Returns the compatible typeQualifier_
*/
int Type::fitQualifier(Type* t1, Type* t2)
{
    if (t1->typeQualifier_ == t2->typeQualifier_)
        return t1->typeQualifier_;

    switch (t1->typeQualifier_)
    {
        case VAR:
            return VAR;

        case CST:
            if (t2->typeQualifier_ == VAR)
                return VAR;
            else
                return CST;

        case DEF:
            switch (t2->typeQualifier_)
            {
                case VAR:
                    return VAR;

                case CST:
                    return CST;

                case DEF:
                    return DEF;

                default:
                    swiftAssert(false, "invalid value in t2->typeQualifier_");
            }
        default:
            swiftAssert(false, "invalid value in t2->typeQualifier_");
            return VAR;
    }
}

//------------------------------------------------------------------------------

std::string SimpleType::toString() const
{
    switch (kind_)
    {
        case INDEX:     return "index";

        case INT:       return "int";
        case INT8:      return "int8";
        case INT16:     return "int16";
        case INT32:     return "int32";
        case INT64:     return "int64";
        case SAT8:      return "sat8";
        case SAT16:     return "sat16";

        case UINT:      return "uint";
        case UINT8:     return "uint8";
        case UINT16:    return "uint16";
        case UINT32:    return "uint32";
        case UINT64:    return "uint64";
        case USAT8:     return "usat8";
        case USAT16:    return "usat16";

        case REAL:      return "real";
        case REAL32:    return "real32";
        case REAL64:    return "real64";

        case CHAR:      return "char";
        case CHAR8:     return "char8";
        case CHAR16:    return "char16";

        case STRING:    return "string";
        case STRING8:   return "string8";
        case STRING16:  return "string16";

        case BOOL:      return "bool";

        default:
            swiftAssert(false, "illegal case value");
            return "";
    }
}

PseudoReg::RegType SimpleType::toRegType() const
{
    switch (kind_)
    {
        case   INDEX:
            return PseudoReg::R_INDEX;

        case   INT:
            return PseudoReg::R_INT;
        case   INT8:
            return PseudoReg::R_INT8;
        case   INT16:
            return PseudoReg::R_INT16;
        case   INT32:
            return PseudoReg::R_INT32;
        case   INT64:
            return PseudoReg::R_INT64;
        case   SAT8:
            return PseudoReg::R_SAT8;
        case   SAT16:
            return PseudoReg::R_SAT16;

        case   UINT:
            return PseudoReg::R_UINT;
        case   UINT8:
            return PseudoReg::R_UINT8;
        case   UINT16:
            return PseudoReg::R_UINT16;
        case   UINT32:
            return PseudoReg::R_UINT32;
        case   UINT64:
            return PseudoReg::R_UINT64;
        case   USAT8:
            return PseudoReg::R_USAT8;
        case   USAT16:
            return PseudoReg::R_USAT16;

        case   REAL:
            return PseudoReg::R_REAL;
        case   REAL32:
            return PseudoReg::R_REAL32;
        case   REAL64:
            return PseudoReg::R_REAL64;

        default:
            swiftAssert(false, "illegal switch-case-value");
            return PseudoReg::R_INDEX; // avoid warning here
    }
}

PseudoReg::RegType SimpleType::int2RegType(int i)
{
    switch (i)
    {
        case   INDEX:
        case L_INDEX:
            return PseudoReg::R_INDEX;

        case   INT:
        case L_INT:
            return PseudoReg::R_INT;
        case   INT8:
        case L_INT8:
            return PseudoReg::R_INT8;
        case   INT16:
        case L_INT16:
            return PseudoReg::R_INT16;
        case   INT32:
        case L_INT32:
            return PseudoReg::R_INT32;
        case   INT64:
        case L_INT64:
            return PseudoReg::R_INT64;
        case   SAT8:
        case L_SAT8:
            return PseudoReg::R_SAT8;
        case   SAT16:
        case L_SAT16:
            return PseudoReg::R_SAT16;

        case   UINT:
        case L_UINT:
            return PseudoReg::R_UINT;
        case   UINT8:
        case L_UINT8:
            return PseudoReg::R_UINT8;
        case   UINT16:
        case L_UINT16:
            return PseudoReg::R_UINT16;
        case   UINT32:
        case L_UINT32:
            return PseudoReg::R_UINT32;
        case   UINT64:
        case L_UINT64:
            return PseudoReg::R_UINT64;
        case   USAT8:
        case L_USAT8:
            return PseudoReg::R_USAT8;
        case   USAT16:
        case L_USAT16:
            return PseudoReg::R_USAT16;

        case   REAL:
        case L_REAL:
            return PseudoReg::R_REAL;
        case   REAL32:
        case L_REAL32:
            return PseudoReg::R_REAL32;
        case   REAL64:
        case L_REAL64:
            return PseudoReg::R_REAL64;

        default:
            swiftAssert(false, "illegal switch-case-value");
            return PseudoReg::R_INDEX; // avoid warning here
    }
}

//------------------------------------------------------------------------------

Container::~Container()
{
    delete type_;
    delete expr_;
}

std::string Container::getContainerType() const
{
    switch (kind_)
    {
        case ARRAY:
            return "array";
        case SIMD:
            return "simd";
        default:
            swiftAssert(false, "illegal case value");
            return "";
    }
}

std::string Container::toString() const
{
    std::ostringstream oss;
    oss << getContainerType() << "{" << type_->toString();

    if (expr_)
        oss << ", " << expr_->toString();

    oss << '}';

    return oss.str();
}
