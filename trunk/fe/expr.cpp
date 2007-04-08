#include "expr.h"

#include <cmath>
#include <iostream>
#include <sstream>

#include "utils/assert.h"

#include "fe/error.h"
#include "fe/type.h"
#include "fe/symtab.h"

#include "me/pseudoreg.h"
#include "me/scopetab.h"
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

std::string* extractOriginalId(std::string* id) {
    // reverse search should usually be faster
    size_t index = id->find_last_of('#');
    swiftAssert( index != std::string::npos, "This is not a revised variable" );

    return new std::string(id->substr(0, index));
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

        case L_TRUE: // like L_FALSE
        case L_FALSE:   type_ = new Type(DEF, new SimpleType(BOOL),   0); break;

        case L_NIL:
            std::cout << "TODO" << std::endl;

        default:
            swiftAssert(false, "illegal switch-case-value");
    }

    genSSA();

    return true;
}

void Literal::genSSA()
{
    // create appropriate PseudoReg
    reg_ = new PseudoReg(0, SimpleType::int2RegType(kind_));

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
    // return false when syntax is wrong
    if ( !op_->analyze() )
        return false;

    // check type
    if ( typeid(*op_->type_->baseType_) != typeid(SimpleType) )
    {
        errorf(op_->line_, "unary operator used with wrong type");
        return false;
    }

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
    else if (c_ == '!')
    {
        if ( !op_->type_->isBool() )
        {
            errorf(op_->line_, "unary ! not used with a bool");
            return false;
        }
    }

    genSSA();

    return true;
}

void UnExpr::genSSA()
{
    swiftAssert( typeid(*type_->baseType_) == typeid(SimpleType), "wrong type here");
    // no revision necessary, temps occur only once
    reg_ = scopetab->newTemp( ((SimpleType*) type_->baseType_)->toRegType() );

    scopetab->appendInstr( new UnInstr(kind_, reg_, op_->reg_) );
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

bool BinExpr::analyze()
{
    // return false when syntax is wrong
    if ( !op1_->analyze() || !op2_->analyze() )
        return false;

    if (kind_ == ',')
    {
        // FIXME remove comma operator
        type_ = op2_->type_->clone();
        lvalue_ = op2_->lvalue_;

        return true;
    }

    if ( (op1_->type_->pointerCount_ >= 1 || op2_->type_->pointerCount_ >= 1) )
    {
        errorf( op1_->line_, "%s used with pointer type", getExprName().c_str() );
        return false;
    }

    // check whether both types are compatible
    if ( !Type::check(op1_->type_, op2_->type_) )
    {
        errorf( op1_->line_, "%s used with different types", getExprName().c_str() );
        return false;
    }

    // check type
    if ( typeid(*op1_->type_->baseType_) != typeid(SimpleType) )
    {
        errorf(op1_->line_, "binary operator used with wrong type");
        return false;
    }

    // check bool cases
    if (    kind_ == '<'   || kind_ == '>'
        ||  kind_ == LE_OP || kind_ == GE_OP
        ||  kind_ == EQ_OP || kind_ == NE_OP)
    {
        type_ = new Type(DEF, new SimpleType(BOOL), 0);
    }
    else
        type_ = op1_->type_->clone();

//             if (    (typeid(*op1_->type_->baseType_) != typeid(SimpleType))
//             ||  (((SimpleType*) op1_->type_->baseType_)->kind_ != BOOL) )
//         {
//             errorf(op1_->line_, "binary boolean operator not used with a bool");
//             return false;
//         }


    // init typeQualifier_ with compatible qualifier
    type_->typeQualifier_ = Type::fitQualifier(op1_->type_, op2_->type_);

    lvalue_ = false;

    genSSA();

    return true;
}

void BinExpr::genSSA()
{
    swiftAssert( typeid(*type_->baseType_) == typeid(SimpleType), "wrong type here" );

    // no revision necessary, temps occur only once
    reg_ = scopetab->newTemp( ((SimpleType*) type_->baseType_)->toRegType() );

    int kind;

    switch (kind_)
    {
        case EQ_OP:
            kind = BinInstr::EQ;
            break;
        case NE_OP:
            kind = BinInstr::NE;
            break;
        case LE_OP:
            kind = BinInstr::LE;
            break;
        case GE_OP:
            kind = BinInstr::GE;
            break;
        default:
            kind = kind_;
    }

    scopetab->appendInstr( new BinInstr(kind, reg_, op1_->reg_, op2_->reg_) );
}

//------------------------------------------------------------------------------

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
    // take id of the swift symtab and extract original name
    std::string* id = extractOriginalId(result_->reg_->id_);
    SymTabEntry* entry = symtab->lookupVar(id);

    // cast to local
    swiftAssert( typeid(*entry) == typeid(Local), "TODO: What if it is not a Local*?" );
    Local* local = (Local*) entry;

    swiftAssert( typeid(*local->type_->baseType_) == typeid(SimpleType), "TODO" );

    // do next revision
    ++local->revision_;
    reg_ = scopetab->newRevision( ((SimpleType*) local->type_->baseType_)->toRegType(), id,  local->revision_);

    // delete original id now. it is not needed anymore
    delete id;

    scopetab->appendInstr( new AssignInstr(kind_, reg_, expr_->reg_) );
}

//------------------------------------------------------------------------------

bool Id::analyze()
{
    lvalue_ = true;
    type_ = symtab->lookupType(id_)->clone();

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
    SymTabEntry* entry = symtab->lookupVar(id_);
    swiftAssert( typeid(*entry) == typeid(Local), "This is not a Local!");

    reg_ = scopetab->lookupReg(id_, entry->revision_);
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
