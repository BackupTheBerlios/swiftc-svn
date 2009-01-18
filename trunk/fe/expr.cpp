#include "fe/expr.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

#include "fe/error.h"
#include "fe/method.h"
#include "fe/type.h"
#include "fe/symtab.h"
#include "fe/var.h"

#include "me/functab.h"
#include "me/op.h"
#include "me/ssa.h"

/*
 * - every expr has to set its type, containing of
 *     - typeQualifier_
 *     - baseType_
 *     - pointerCount_
 * - every expr must decide whether it can be considered as an lvalue_
 * 
 * - return true if everything is ok
 * - return false otherwise
 */

namespace swift {

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Expr::Expr(int line)
    : Node(line)
    , lvalue_(false)
    , type_(0)
{}

Expr::~Expr()
{
    delete type_;
}

//------------------------------------------------------------------------------

/*
 * Init static
 */

Literal::TypeMap* Literal::typeMap_ = 0;

/*
 * constructor
 */

Literal::Literal(int kind, int line /*= NO_LINE*/)
    : Expr(line)
    , kind_(kind)
{}

/*
 * further methods
 */

me::Op::Type Literal::toType() const
{
    swiftAssert(typeMap_->find(kind_) != typeMap_->end(), 
            "must be found here");
    return typeMap_->find(kind_)->second;
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

    genSSA();

    return true;
}

void Literal::genSSA()
{
    // create appropriate Reg
    me::Const* literal = new me::Const( toType() );
    place_ = literal;

    switch (kind_)
    {
        case L_INDEX:   literal->value_.index_     = index_;   break;

        case L_INT8:    literal->value_.int8_      = int8_;    break;
        case L_INT16:   literal->value_.int16_     = int16_;   break;
        case L_INT: // TODO do this arch independently
        case L_INT32:   literal->value_.int32_     = int32_;   break;
        case L_INT64:   literal->value_.int64_     = int64_;   break;
        case L_SAT8:    literal->value_.sat8_      = sat8_;    break;
        case L_SAT16:   literal->value_.sat16_     = sat16_;   break;

        case L_UINT8:   literal->value_.uint8_     = uint8_;   break;
        case L_UINT16:  literal->value_.uint16_    = uint16_;  break;
        case L_UINT: // TODO do this arch independently
        case L_UINT32:  literal->value_.uint32_    = uint32_;  break;
        case L_UINT64:  literal->value_.uint64_    = uint64_;  break;
        case L_USAT8:   literal->value_.usat8_     = usat8_;   break;
        case L_USAT16:  literal->value_.usat16_    = usat16_;  break;

        case L_REAL: // TODO do this arch independently
        case L_REAL32:  literal->value_.real32_    = real32_;  break;
        case L_REAL64:  literal->value_.real64_    = real64_;  break;

        case L_TRUE: // like L_FALSE
        case L_FALSE:   literal->value_.bool_      = bool_;    break;

        case L_NIL:     literal->value_.ptr_       = ptr_;     break;

        default:
            swiftAssert(false, "illegal switch-case-value");
    }
}

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

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Id::Id(std::string* id, int line /*= NO_LINE*/)
    : Expr(line)
    , id_(id)
{}

Id::~Id()
{
    delete id_;
}

/*
 * further methods
 */

bool Id::analyze()
{
    lvalue_ = true;
    Var* var = symtab->lookupVar(id_);

    if (var == 0)
    {
        errorf(line_, "'%s' was not declared in this scope", id_->c_str());
        return false;
    }
    else
        type_ = var->type_->clone();

    place_ = me::functab->lookupReg(var->varNr_);
    if (place_ == 0)
    {
        me::Reg* reg;
#ifdef SWIFT_DEBUG
        reg = me::functab->newVar( var->type_->baseType_->toType(), var->varNr_, id_ );
#else // SWIFT_DEBUG
        reg = me::functab->newVar( var->type_->baseType_->toType(), var->varNr_ );
#endif // SWIFT_DEBUG
        place_ = reg;

        var->varNr_ = reg->varNr_;
        //symtab->insertLocalByVarNr(local);
    }

    return true;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

UnExpr::UnExpr(int kind, Expr* op, int line /*= NO_LINE*/)
    : Expr(line)
    , kind_(kind)
    , op_(op)
{}

UnExpr::~UnExpr()
{
    delete op_;
}

/*
 * further methods
 */

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

    genSSA();

    return true;
}

void UnExpr::genSSA()
{
    me::Reg* reg = me::functab->newSSA( type_->baseType_->toType() );
    place_ = reg;

    int kind;

    switch (kind_)
    {
        case '-':
            kind = me::AssignInstr::UNARY_MINUS;
            break;
        case NOT_OP:
            kind = me::AssignInstr::NOT;
            break;
        default:
            swiftAssert(kind_ == '^', "impossible switch/case value");
            kind = kind_;
    }

    me::functab->appendInstr( new me::AssignInstr(kind, reg, op_->place_) );
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

BinExpr::BinExpr(int kind, Expr* op1, Expr* op2, int line /*= NO_LINE*/)
    : Expr(line)
    , kind_(kind)
    , op1_(op1)
    , op2_(op2)
{}

BinExpr::~BinExpr()
{
    delete op1_;
    delete op2_;
}

/*
 * further methods
 */

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
    if ( !op1_->analyze() | !op2_->analyze() ) // analyze both ops in all cases
        return false;

    if ( (op1_->type_->pointerCount_ >= 1 || op2_->type_->pointerCount_ >= 1) )
    {
        errorf( op1_->line_, "%s used with pointer type", getExprName().c_str() );
        return false;
    }

    // check whether there is an operator which fits
    Sig sig;
    sig.params_.append( new Param(Param::ARG, op1_->type_->clone(), 0, 0) );
    sig.params_.append( new Param(Param::ARG, op2_->type_->clone(), 0, 0) );
    std::string* opString = operatorToString(kind_); // TODO remove pointer stuff here
    Method* method = symtab->lookupMethod(op1_->type_->baseType_->id_, opString, OPERATOR, sig, line_, SymTab::CHECK_JUST_INGOING);

    delete opString;

    if (!method)
    {
        errorf( line_, "no operator %c (%s, %s) defined in class %s",
            c_, op1_->type_->toString().c_str(),
            op2_->type_->toString().c_str(),
            op1_->type_->toString().c_str() );
        return false;
    }
    // else

    // find first out parameter and clone this type
    type_ = method->sig_.findFirstOut()->value_->type_->clone();
    swiftAssert(type_, "an operator should always return a type");

    genSSA();

    return true;
}

void BinExpr::genSSA()
{
    me::Reg* reg = me::functab->newSSA( type_->baseType_->toType() );
    place_ = reg;

    int kind;

    switch (kind_)
    {
        case EQ_OP:
            kind = me::AssignInstr::EQ;
            break;
        case NE_OP:
            kind = me::AssignInstr::NE;
            break;
        case LE_OP:
            kind = me::AssignInstr::LE;
            break;
        case GE_OP:
            kind = me::AssignInstr::GE;
            break;
        default:
            kind = kind_;
    }

    if ( op1_->type_->isBuiltin() )
        me::functab->appendInstr( new me::AssignInstr(kind, reg, op1_->place_, op2_->place_) );
    else
        swiftAssert(false, "TODO");
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

ExprList::ExprList(Expr* expr, ExprList* next /*= 0*/, int line /*= NO_LINE*/)
    : Node(line)
    , expr_(expr)
    , next_(next)
{}

ExprList::~ExprList()
{
    delete expr_;
    if (next_)
        delete next_;
}

/*
 * further methods
 */

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

} // namespace swift
