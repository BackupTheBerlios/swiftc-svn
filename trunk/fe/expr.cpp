#include "expr.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

#include "fe/error.h"
#include "fe/method.h"
#include "fe/type.h"
#include "fe/symtab.h"

#include "me/functab.h"
#include "me/pseudoreg.h"
#include "me/ssa.h"

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
    delete type_;
}

//------------------------------------------------------------------------------

std::string Literal::toString() const
{
    std::ostringstream oss;

    switch (kind_)
    {
        case L_INDEX:   oss << index_       << "x";     break;

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

        case L_TRUE:    oss << "true";                  break;
        case L_FALSE:   oss << "false";                 break;

        case L_NIL:     oss << "nil";                   break;

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

        default:
            swiftAssert(false, "illegal case value");
            return "";
    }

    return oss.str();
}

PseudoReg::RegType Literal::toRegType() const
{
    switch (kind_)
    {
        case L_INDEX:
            return PseudoReg::R_INDEX;

        case L_INT:
            return PseudoReg::R_INT;
        case L_INT8:
            return PseudoReg::R_INT8;
        case L_INT16:
            return PseudoReg::R_INT16;
        case L_INT32:
            return PseudoReg::R_INT32;
        case L_INT64:
            return PseudoReg::R_INT64;
        case L_SAT8:
            return PseudoReg::R_SAT8;
        case L_SAT16:
            return PseudoReg::R_SAT16;

       case L_UINT:
            return PseudoReg::R_UINT;
        case L_UINT8:
            return PseudoReg::R_UINT8;
        case L_UINT16:
            return PseudoReg::R_UINT16;
        case L_UINT32:
            return PseudoReg::R_UINT32;
        case L_UINT64:
            return PseudoReg::R_UINT64;
        case L_USAT8:
            return PseudoReg::R_USAT8;
        case L_USAT16:
            return PseudoReg::R_USAT16;

        case L_REAL:
            return PseudoReg::R_REAL;
        case L_REAL32:
            return PseudoReg::R_REAL32;
        case L_REAL64:
            return PseudoReg::R_REAL64;

        case L_TRUE:
        case L_FALSE:
            return PseudoReg::R_BOOL;

        default:
            swiftAssert(false, "illegal switch-case-value");
    }

    return PseudoReg::R_INDEX; // avoid warning here
}

bool Literal::analyze()
{
    lvalue_ = false;

    switch (kind_)
    {
        case L_INDEX:   type_ = new Type(new BaseType(new std::string("index")),  0); break;

        case L_INT:     type_ = new Type(new BaseType(new std::string("int")),    0); break;
        case L_INT8:    type_ = new Type(new BaseType(new std::string("int8")),   0); break;
        case L_INT16:   type_ = new Type(new BaseType(new std::string("int16")),  0); break;
        case L_INT32:   type_ = new Type(new BaseType(new std::string("int32")),  0); break;
        case L_INT64:   type_ = new Type(new BaseType(new std::string("int64")),  0); break;
        case L_SAT8:    type_ = new Type(new BaseType(new std::string("sat8")),   0); break;
        case L_SAT16:   type_ = new Type(new BaseType(new std::string("sat16")),  0); break;

        case L_UINT:    type_ = new Type(new BaseType(new std::string("uint")),   0); break;
        case L_UINT8:   type_ = new Type(new BaseType(new std::string("uint8")),  0); break;
        case L_UINT16:  type_ = new Type(new BaseType(new std::string("uint16")), 0); break;
        case L_UINT32:  type_ = new Type(new BaseType(new std::string("uint32")), 0); break;
        case L_UINT64:  type_ = new Type(new BaseType(new std::string("uint64")), 0); break;
        case L_USAT8:   type_ = new Type(new BaseType(new std::string("usat8")),  0); break;
        case L_USAT16:  type_ = new Type(new BaseType(new std::string("usat16")), 0); break;

        case L_REAL:    type_ = new Type(new BaseType(new std::string("real")),   0); break;
        case L_REAL32:  type_ = new Type(new BaseType(new std::string("real32")), 0); break;
        case L_REAL64:  type_ = new Type(new BaseType(new std::string("real64")), 0); break;

        case L_TRUE: // like L_FALSE
        case L_FALSE:   type_ = new Type(new BaseType(new std::string("bool")),   0); break;

        case L_NIL:
            std::cout << "TODO" << std::endl;

        default:
            swiftAssert(false, "illegal switch-case-value");
    }

    if (gencode)
        genSSA();

    return true;
}

void Literal::genSSA()
{
    // create appropriate PseudoReg
    if (gencode)
        reg_ = new PseudoReg( toRegType() );

    switch (kind_)
    {
        case L_INDEX:   reg_->value_.index_     = index_;   break;

        case L_INT:     reg_->value_.int_       = int_;     break;
        case L_INT8:    reg_->value_.int8_      = int8_;    break;
        case L_INT16:   reg_->value_.int16_     = int16_;   break;
        case L_INT32:   reg_->value_.int32_     = int32_;   break;
        case L_INT64:   reg_->value_.int64_     = int64_;   break;
        case L_SAT8:    reg_->value_.sat8_      = sat8_;    break;
        case L_SAT16:   reg_->value_.sat16_     = sat16_;   break;

        case L_UINT:    reg_->value_.uint_      = uint_;    break;
        case L_UINT8:   reg_->value_.uint8_     = uint8_;   break;
        case L_UINT16:  reg_->value_.uint16_    = uint16_;  break;
        case L_UINT32:  reg_->value_.uint32_    = uint32_;  break;
        case L_UINT64:  reg_->value_.uint64_    = uint64_;  break;
        case L_USAT8:   reg_->value_.usat8_     = usat8_;   break;
        case L_USAT16:  reg_->value_.usat16_    = usat16_;  break;

        case L_REAL:    reg_->value_.real_      = real_;    break;
        case L_REAL32:  reg_->value_.real32_    = real32_;  break;
        case L_REAL64:  reg_->value_.real64_    = real64_;  break;

        case L_TRUE: // like L_FALSE
        case L_FALSE:   reg_->value_.bool_      = bool_;    break;

        case L_NIL:     reg_->value_.ptr_       = ptr_;     break;

        default:
            swiftAssert(false, "illegal switch-case-value");
    }
}

//------------------------------------------------------------------------------

bool UnExpr::analyze()
{
    lvalue_ = false;

    // return false when syntax is wrong
    if ( !op_->analyze() )
        return false;

    type_ = op_->type_->clone();

    if (c_ == '&')
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
    else if (c_ == '!')
    {
        if ( !op_->type_->isBool() )
        {
            errorf(op_->line_, "unary ! not used with a bool");
            return false;
        }
    }

    if (gencode)
        genSSA();

    return true;
}

void UnExpr::genSSA()
{
    reg_ = functab->newTemp( type_->baseType_->toRegType() );

    int kind;

    switch (kind_)
    {
        case '-':
            kind = AssignInstr::UNARY_MINUS;
            break;
        case NOT_OP:
            kind = AssignInstr::NOT;
            break;
        default:
            swiftAssert(kind_ == '^', "impossible switch/case value");
            kind = kind_;
    }

    functab->appendInstr( new AssignInstr(kind, reg_, op_->reg_) );
}

//------------------------------------------------------------------------------

std::string BinExpr::getExprName() const
{
    if (c_ == '[')
        return "index expression";

    return "binary expression";
}

std::string BinExpr::getOpString() const
{
    std::ostringstream oss;
    if (c_ == '[')
        oss << "[";
    else
        oss << " " << c_ << " ";

    return oss.str();
}

bool BinExpr::analyze()
{
    lvalue_ = false;

    // return false when syntax is wrong
    if ( !op1_->analyze() || !op2_->analyze() )
        return false;

    if ( (op1_->type_->pointerCount_ >= 1 || op2_->type_->pointerCount_ >= 1) )
    {
        errorf( op1_->line_, "%s used with pointer type", getExprName().c_str() );
        return false;
    }

    // checker whether there is an operator which fits
    Method::Signature sig;
    sig.params_.append( new Param(Param::ARG, op1_->type_->clone()) );
    sig.params_.append( new Param(Param::ARG, op2_->type_->clone()) );
    Method* method = symtab->lookupMethod(op1_->type_->baseType_->id_, operatorToString(kind_), OPERATOR, sig, line_);

    if (!method)
        return false;
    // else

    // find first out parameter and clone this type
    type_ = method->signature_.findFirstOut()->type_->clone();

    if (gencode)
        genSSA();

    return true;
}

void BinExpr::genSSA()
{
    reg_ = functab->newTemp( type_->baseType_->toRegType() );

    int kind;

    switch (kind_)
    {
        case EQ_OP:
            kind = AssignInstr::EQ;
            break;
        case NE_OP:
            kind = AssignInstr::NE;
            break;
        case LE_OP:
            kind = AssignInstr::LE;
            break;
        case GE_OP:
            kind = AssignInstr::GE;
            break;
        default:
            kind = kind_;
    }

    functab->appendInstr( new AssignInstr(kind, reg_, op1_->reg_, op2_->reg_) );
}

//------------------------------------------------------------------------------

/* replaced by AssignStatement

bool AssignExpr::analyze()
{
    // return false when syntax is wrong
    if ( !result_->analyze() || !expr_->analyze() )
        return false;


    if ( !Type::check(result_->type_, expr_->type_) )
    {
        errorf( result_->line_, "assign expression used with different types" );
        return false;
    }

    type_ = result_->type_->clone();

    lvalue_ = false;

    genSSA();

    return true;
}

void AssignExpr::genSSA()
{
    SymTabEntry* entry = symtab->lookupVar(result_->reg_->regNr_);

    // cast to local
    swiftAssert( typeid(*entry) == typeid(Local), "TODO: What if it is not a Local*?" );
    Local* local = (Local*) entry;

    swiftAssert( typeid(*local->type_->baseType_) == typeid(SimpleType), "TODO" );

    reg_ = result_->reg_;//functab->newVar( ((SimpleType*) local->type_->baseType_)->toRegType(), local->regNr_ );
    functab->appendInstr( new AssignInstr(kind_, reg_, expr_->reg_) );
}

*/

//------------------------------------------------------------------------------

bool Id::analyze()
{
    lvalue_ = true;
    type_ = symtab->lookupType(id_);

    if (type_ == 0)
    {
        errorf(line_, "'%s' was not declared in this scope", id_->c_str());
        return false;
    }

    type_ = type_->clone();

    if (gencode)
        genSSA();

    return true;
}

void Id::genSSA()
{
    SymTabEntry* entry = symtab->lookupVar(id_);
    swiftAssert( typeid(*entry) == typeid(Local), "This is not a Local!");
    reg_ = functab->lookupReg(entry->regNr_);

    if (!reg_)
    {
        // do the first revision
        Local* local = (Local*) entry;
#ifdef SWIFT_DEBUG
        reg_ = functab->newVar( local->type_->baseType_->toRegType(), local->regNr_, id_ );
#else // SWIFT_DEBUG
        reg_ = functab->newVar( local->type_->baseType_->toRegType(), local->regNr_ );
#endif // SWIFT_DEBUG
        local->regNr_ = reg_->regNr_;
        symtab->insertLocalByRegNr(local);
    }
}

//------------------------------------------------------------------------------

bool ExprList::analyze()
{
    bool result = true;

    // for each expr in this list
    for (ExprList* iter = this; iter != 0; iter = iter->next_)
        result &= iter->expr_->analyze();

    return result;
}

//------------------------------------------------------------------------------

bool FunctionCall::analyze()
{
    std::cout << "not yet implemented" << std::endl;

    genSSA();

    return true;
}

void FunctionCall::genSSA()
{
    std::cout << "not yet implemented" << std::endl;
//     place_ = new std::string("TODO");
};
