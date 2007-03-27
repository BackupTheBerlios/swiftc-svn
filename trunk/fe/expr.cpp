#include "expr.h"

#include <cmath>
#include <iostream>
#include <sstream>

#include "../utils/assert.h"

#include "error.h"
#include "type.h"
#include "symboltable.h"

#include "../im/ssa.h"

/*
    - every expr has to set its type, containing of
        - typeQualifier_
        - baseType_
        - pointerCount_
    - every expr must decide whether it can be considered as an lvalue_

    - return true if everything is ok
    - return false otherwise
*/

//------------------------------------------------------------------------------

Expr::~Expr()
{
    if (type_)
    {
        delete type_;
        type_ = 0;
    }
}

//------------------------------------------------------------------------------

std::string Literal::toString() const
{
    std::ostringstream oss;

    switch (kind_)
    {
        case L_INT:     oss << int_;                    break;
        case L_INT8:    oss << int(int8_)   << "b";     break;
        case L_INT16:   oss << int16_       << "w";     break;
        case L_INT32:   oss << int32_       << "d";     break;
        case L_INT64:   oss << int64_       << "q";     break;
        case L_SAT8:    oss << int(sat8_)   << "sb";    break;
        case L_SAT16:   oss << sat16_       << "sw";    break;

        case L_UINT:    oss << uint_;                   break;
        case L_UINT8:   oss << int(uint8_)  << "ub";    break;
        case L_UINT16:  oss << uint16_      << "uw";    break;
        case L_UINT32:  oss << uint32_      << "ud";    break;
        case L_UINT64:  oss << uint64_      << "uq";    break;
        case L_USAT8:   oss << int(usat8_)  << "usb";   break;
        case L_USAT16:  oss << usat16_      << "usw";   break;

        // hence it is real, real32 or real64

        case L_REAL:
            oss << real_;
        break;
        case L_REAL32:
            oss << real32_;
            if ( fmod(real32_, 1.0) == 0.0 )
                oss << ".d";
            else
                oss << "d";
            break;
        case L_REAL64:
            oss << real64_;
            if ( fmod(real64_, 1.0) == 0.0 )
                oss << ".q";
            else
                oss << "q"; // FIXME
                std::cout << "fjkdlfjkdjfdl" << fmod(real64_, 1.0) << std::endl;
            break;

        SWIFT_TO_STRING_ERROR
    }

    return oss.str();
}

bool Literal::analyze()
{
    lvalue_ = false;

    switch (kind_)
    {
        case L_INDEX:   type_ = new Type(DEF, new SimpleType(INDEX),  0); break;

        case L_INT:     type_ = new Type(DEF, new SimpleType(INT),    0); break;
        case L_INT8:    type_ = new Type(DEF, new SimpleType(INT8),   0); break;
        case L_INT16:   type_ = new Type(DEF, new SimpleType(INT16),  0); break;
        case L_INT32:   type_ = new Type(DEF, new SimpleType(INT32),  0); break;
        case L_INT64:   type_ = new Type(DEF, new SimpleType(INT64),  0); break;
        case L_SAT8:    type_ = new Type(DEF, new SimpleType(SAT8),   0); break;
        case L_SAT16:   type_ = new Type(DEF, new SimpleType(SAT16),  0); break;

        case L_UINT:    type_ = new Type(DEF, new SimpleType(UINT),   0); break;
        case L_UINT8:   type_ = new Type(DEF, new SimpleType(UINT8),  0); break;
        case L_UINT16:  type_ = new Type(DEF, new SimpleType(UINT16), 0); break;
        case L_UINT32:  type_ = new Type(DEF, new SimpleType(UINT32), 0); break;
        case L_UINT64:  type_ = new Type(DEF, new SimpleType(UINT64), 0); break;
        case L_USAT8:   type_ = new Type(DEF, new SimpleType(USAT8),  0); break;
        case L_USAT16:  type_ = new Type(DEF, new SimpleType(USAT16), 0); break;

        case L_REAL:    type_ = new Type(DEF, new SimpleType(REAL),   0); break;
        case L_REAL32:  type_ = new Type(DEF, new SimpleType(REAL32), 0); break;
        case L_REAL64:  type_ = new Type(DEF, new SimpleType(REAL64), 0); break;

        default:
            swiftAssert(false, "illegal switch-case-value");
    }

    genSSA();

    return true;
}

void Literal::genSSA()
{
    place_ = 0;
}

//------------------------------------------------------------------------------

std::string UnExpr::toString() const
{
    return *place_->id_;
}

bool UnExpr::analyze()
{
    // return false when syntax is wrong
    if ( !op_->analyze() )
        return false;

    // TODO VAR CST DEF

    lvalue_ = false;
    type_ = op_->type_->clone();

    if      (c_ == '&')
        ++type_->pointerCount_;
    else if (c_ == '^')
    {
        if (type_->pointerCount_ == 0)
        {
            errorf(op_->line_, "unary ^ tried to dereference a non-pointer");
            return false;
        }

        --type_->pointerCount_;
    }

    genSSA();

    return true;
}

void UnExpr::genSSA()
{
    Local* tmp = symtab.newTemp(type_);
    // no revision necessary, temps occur only once
    place_ = tmp;

    instrlist.append( new UnInstr(kind_, this, op_) );
}

//------------------------------------------------------------------------------

std::string BinExpr::getExprName() const
{
    if (c_ == ',' )
        return "comma expression";
    if (c_ == '[')
        return "index expression";

    return "binary expression";
}

std::string BinExpr::getOpString() const
{
    std::ostringstream oss;
    if      (c_ == ',' )
        oss << ", ";
    else if (c_ == '[')
        oss << "[";
    else
        oss << " " << c_ << " ";

    return oss.str();
}

std::string BinExpr::toString() const
{
    return *place_->id_;
}

bool BinExpr::analyze()
{
    // return false when syntax is wrong
    if ( !op1_->analyze() || !op2_->analyze() )
        return false;

    if (kind_ == ',')
    {
        type_ = op2_->type_->clone();
        lvalue_ = op2_->lvalue_;

        return true;
    }

    if ( (op1_->type_->pointerCount_ >= 1 || op2_->type_->pointerCount_ >= 1) )
    {
        errorf( op1_->line_, "%s used with pointer type", getExprName().c_str() );
        return false;
    }

    if ( !Type::check(op1_->type_, op2_->type_) )
    {
        errorf( op1_->line_, "%s used with different types", getExprName().c_str() );
        return false;
    }

    type_ = op1_->type_->clone();
    // init typeQualifier_ with compatible qualifier
    type_->typeQualifier_ = Type::fitQualifier(op1_->type_, op2_->type_);

    lvalue_ = false;

    genSSA();

    return true;
}

void BinExpr::genSSA()
{
    Local* tmp = symtab.newTemp(type_);
    // no revision necessary, temps occur only once
    place_ = tmp;

    instrlist.append( new BinInstr(kind_, this, op1_, op2_) );
}

//------------------------------------------------------------------------------

std::string AssignExpr::toString() const
{
    return *place_->id_;
}

bool AssignExpr::analyze()
{
    // return false when syntax is wrong
    if ( !result_->analyze() || !expr_->analyze() )
        return false;

    if (!result_->lvalue_)
    {
        errorf(result_->line_, "invalid lvalue in assignment");
        return false;
    }

    if ( !Type::check(result_->type_, expr_->type_) )
    {
        errorf( result_->line_, "assign expression used with different types" );
        return false;
    }

    type_ = result_->type_->clone();
    // init typeQualifier_ with compatible qualifier
    type_->typeQualifier_ = Type::fitQualifier(result_->type_, expr_->type_);

    lvalue_ = false;

    genSSA();

    return true;
}

void AssignExpr::genSSA()
{
    swiftAssert( typeid(*result_->place_) == typeid(Local), "TODO: What if it is not a Local*?" );

    Local* local = (Local*) result_->place_;
    // do next revision
    place_ = symtab.newRevision(local);
    instrlist.append( new AssignInstr(kind_, this, expr_) );
}

//------------------------------------------------------------------------------

bool Id::analyze()
{
    lvalue_ = true;
    type_ = symtab.lookupType(id_);

    if (type_ == 0)
    {
        errorf(line_, "'%s' was not declared in this scope", id_->c_str());
        return false;
    }

    genSSA();

    return true;
}

void Id::genSSA()
{
    SymTabEntry* entry = symtab.lookupVar(id_);

    // check whether we already have a revised variable
    if (entry->revision_ == SymTabEntry::REVISED_VAR)
    {
        place_ = entry;
        return;
    }
    // else
    swiftAssert( typeid(*entry) == typeid(Local), "This is not a Local!");
    place_ = symtab.lookupLastRevision((Local*) entry);
}

//------------------------------------------------------------------------------

std::string FunctionCall::toString() const
{
//     std::ostringstream oss;
//
//     oss << *id_ << '(';
//
//     for (Arg* iter = args_; iter != 0; iter = iter->next_) {
//         oss << iter->expr_->toString();
//
//         if (iter->next_)
//             oss << ", ";
//     }
//
//     oss << ')';
    std::cout << "TODO" << std::endl;

    return std::string("TODO");
}

bool FunctionCall::analyze()
{
//     place_ = new std::string("TODO");

    std::cout << "not yet implemented" << std::endl;

    genSSA();

    return true;
}

void FunctionCall::genSSA()
{
    std::cout << "not yet implemented" << std::endl;
//     place_ = new std::string("TODO");
};
