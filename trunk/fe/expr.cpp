/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "fe/expr.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

#include "fe/class.h"
#include "fe/error.h"
#include "fe/exprlist.h"
#include "fe/method.h"
#include "fe/type.h"
#include "fe/signature.h"
#include "fe/symtab.h"
#include "fe/var.h"

#include "me/arch.h"
#include "me/functab.h"
#include "me/op.h"
#include "me/offset.h"
#include "me/ssa.h"
#include "me/struct.h"

/*
 * - every expr has to set its type, containing of
 *     - typeQualifier_
 *     - baseType_
 *     - pointerCount_
 * - every expr must decide whether it can be considered as an lvalue
 * 
 * - return true if everything is ok
 * - return false otherwise
 */

namespace swift {

//------------------------------------------------------------------------------

/*
 * constructor
 */

Expr::Expr(int line)
    : TypeNode(0, line)
    , place_(0)
    , neededAsLValue_(false)
{}

/*
 * virtual methods
 */

me::Var* Expr::getPlace()
{
    return (me::Var*) place_;
}

/*
 * further methods
 */

void Expr::neededAsLValue()
{
    neededAsLValue_ = true;
}

bool Expr::isNeededAsLValue() const
{
    return neededAsLValue_;
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
    if ( isNeededAsLValue() )
    {
        errorf(line_, "lvalue required as left operand of assignment");
        return false;
    }

    switch (kind_)
    {
        case L_INDEX:   type_ = new BaseType(CONST, new std::string("index")); break;

        case L_INT:     type_ = new BaseType(CONST, new std::string("int"));   break;
        case L_INT8:    type_ = new BaseType(CONST, new std::string("int8"));  break;
        case L_INT16:   type_ = new BaseType(CONST, new std::string("int16")); break;
        case L_INT32:   type_ = new BaseType(CONST, new std::string("int32")); break;
        case L_INT64:   type_ = new BaseType(CONST, new std::string("int64")); break;
        case L_SAT8:    type_ = new BaseType(CONST, new std::string("sat8"));  break;
        case L_SAT16:   type_ = new BaseType(CONST, new std::string("sat16")); break;

        case L_UINT:    type_ = new BaseType(CONST, new std::string("uint"));   break;
        case L_UINT8:   type_ = new BaseType(CONST, new std::string("uint8"));  break;
        case L_UINT16:  type_ = new BaseType(CONST, new std::string("uint16")); break;
        case L_UINT32:  type_ = new BaseType(CONST, new std::string("uint32")); break;
        case L_UINT64:  type_ = new BaseType(CONST, new std::string("uint64")); break;
        case L_USAT8:   type_ = new BaseType(CONST, new std::string("usat8"));  break;
        case L_USAT16:  type_ = new BaseType(CONST, new std::string("usat16")); break;

        case L_REAL:    type_ = new BaseType(CONST, new std::string("real"));   break;
        case L_REAL32:  type_ = new BaseType(CONST, new std::string("real32")); break;
        case L_REAL64:  type_ = new BaseType(CONST, new std::string("real64")); break;

        case L_TRUE: // like L_FALSE
        case L_FALSE:   type_ = new BaseType(CONST, new std::string("bool"));   break;

        default:
            swiftAssert(false, "illegal switch-case-value");
    }

    genSSA();

    return true;
}

void Literal::genSSA()
{
    // create appropriate Var
    me::Const* literal = me::functab->newConst( toType() );
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

        case L_TRUE:
        case L_FALSE:   literal->value_.bool_      = bool_;    break;

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
            break;

        default:
            swiftAssert(false, "illegal case value");
            return "";
    }

    return oss.str();
}

/*
 * static methods
 */

void Literal::initTypeMap()
{
    typeMap_ = new Literal::TypeMap();

    (*typeMap_)[L_TRUE]   = me::Op::R_BOOL;
    (*typeMap_)[L_FALSE]  = me::Op::R_BOOL;

    (*typeMap_)[L_INT8]   = me::Op::R_INT8;
    (*typeMap_)[L_INT16]  = me::Op::R_INT16;
    (*typeMap_)[L_INT32]  = me::Op::R_INT32;
    (*typeMap_)[L_INT64]  = me::Op::R_INT64;

    (*typeMap_)[L_SAT8]   = me::Op::R_SAT8;
    (*typeMap_)[L_SAT16]  = me::Op::R_SAT16;

    (*typeMap_)[L_UINT8]  = me::Op::R_UINT8;
    (*typeMap_)[L_UINT16] = me::Op::R_UINT16;
    (*typeMap_)[L_UINT32] = me::Op::R_UINT32;
    (*typeMap_)[L_UINT64] = me::Op::R_UINT64;

    (*typeMap_)[L_USAT8]  = me::Op::R_USAT8;
    (*typeMap_)[L_USAT16] = me::Op::R_USAT16;

    (*typeMap_)[L_REAL32] = me::Op::R_REAL32;
    (*typeMap_)[L_REAL64] = me::Op::R_REAL64;

    (*typeMap_)[L_INT]    = me::arch->getPreferedInt();
    (*typeMap_)[L_UINT]   = me::arch->getPreferedUInt();
    (*typeMap_)[L_INDEX]  = me::arch->getPreferedIndex();
    (*typeMap_)[L_REAL]   = me::arch->getPreferedReal();
}

void Literal::destroyTypeMap()
{
    delete typeMap_;
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
    var_ = symtab->lookupVar(id_);

    if (var_ == 0)
    {
        errorf(line_, "'%s' was not declared in this scope", id_->c_str());
        return false;
    }
    // else

    type_  = var_->getType()->clone();
    place_ = var_->getMeVar();

    return true;
}

void Id::genSSA() {} // do nothing

std::string Id::toString() const
{
    return *id_;
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
    if ( isNeededAsLValue() )
    {
        errorf(line_, "lvalue required as left operand of assignment");
        return false;
    }

    // return false when syntax is wrong
    if ( !op_->analyze() )
        return false;

    if (c_ == '&')
        type_ = new Ptr( VAR, op_->getType()->clone() );
    else if (c_ == '^')
    {
        Ptr* ptr = dynamic_cast<Ptr*>(type_);

        if (!ptr)
        {
            errorf(op_->line_, "unary ^ tried to dereference a non-pointer");
            return false;
        }

        type_ = ptr->getInnerType();
    }
    else if (c_ == '!')
    {
        if ( !op_->getType()->isBool() )
        {
            errorf(op_->line_, "unary ! not used with a bool");
            return false;
        }

        type_ = op_->getType()->clone();
    }
    else
        type_ = op_->getType()->clone();

    genSSA();

    return true;
}

void UnExpr::genSSA()
{
    me::Var* var = type_->createVar();
    place_ = var;
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
            swiftAssert(kind_ == '&', "impossible switch/case value");
            kind = kind_;
    }

    me::functab->appendInstr( new me::AssignInstr(kind, var, op_->getPlace()) );
}

std::string UnExpr::toString() const
{
    std::string* opString = operatorToString(kind_);
    std::string result = *opString + " " + op_->toString();
    delete opString;

    return result;
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
    if ( isNeededAsLValue() )
    {
        errorf(line_, "lvalue required as left operand of assignment");
        return false;
    }

    // return false when syntax is wrong
    if ( !op1_->analyze() | !op2_->analyze() ) // analyze both ops in all cases
        return false;

    const Ptr* ptr1 = dynamic_cast<const Ptr*>( op1_->getType() );
    const Ptr* ptr2 = dynamic_cast<const Ptr*>( op2_->getType() );

    if ( ptr1 || ptr2 )
    {
        errorf( op1_->line_, "%s used with pointer type", getExprName().c_str() );
        return false;
    }

    swiftAssert( typeid(*op1_->getType()) == typeid(BaseType), "must be a BaseType here" );
    swiftAssert( typeid(*op2_->getType()) == typeid(BaseType), "must be a BaseType here" );

    const BaseType* bt1 = (const BaseType*) op1_->getType();

    // check whether there is an operator which fits
    TypeList argTypeList;
    argTypeList.push_back(op1_->getType());
    argTypeList.push_back(op2_->getType());

    std::string* opString = operatorToString(kind_);
    Method* method = symtab->lookupMethod(
            symtab->lookupClass(bt1->getId()), opString, OPERATOR, argTypeList, line_);

    delete opString;

    if (!method)
        return false;

    // find first out parameter and clone this type
    type_ = method->sig_->getOut()[0]->clone();

    genSSA();

    return true;
}

void BinExpr::genSSA()
{
    me::Var* var = type_->createVar();
    place_ = var;
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

    if ( op1_->getType()->isAtomic() )
    {
        me::functab->appendInstr( 
                new me::AssignInstr(kind, var, op1_->getPlace(), op2_->getPlace()) );
    }
    else
        swiftAssert(false, "TODO");
}

std::string BinExpr::toString() const
{
    std::string* opString = operatorToString(kind_);
    std::string result = op1_->toString() + " " + *opString + " " + op2_->toString();
    delete opString;

    return result;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

MemberAccess::MemberAccess(Expr* expr, std::string* id, int line /*= NO_LINE*/)
    : Expr(line)
    , expr_(expr)
    , id_(id)
    , right_(true)
{}

MemberAccess::~MemberAccess()
{
    delete expr_;
    delete id_;
}

/*
 * further methods
 */

bool MemberAccess::analyze()
{
    MemberAccess* ma = dynamic_cast<MemberAccess*>(expr_);

    if (ma)
        ma->right_ = false;

    /*
     * The accesses are traversed from right to left to this point and from
     * left to right beyond this point.
     */
    if ( !expr_->analyze() )
        return false;

    // pass-through places
    if (!ma)
    {
        swiftAssert(expr_->getPlace()->type_ == me::Op::R_STACK, "must be a stack location")
        memPlace_ = (me::MemVar*) expr_->getPlace();
    }
    else
        memPlace_ = ma->memPlace_; // pass-through

    place_ = expr_->getPlace(); // pass-through

    /*
     * In a chain of member accesses there are two special accesses:
     * - the left most one -> ma = 0
     * - the right most one -> right = true
     */

    swiftAssert( typeid(*expr_->getType()) == typeid(BaseType), "TODO" );
    const BaseType* exprBT = (const BaseType*) expr_->getType();

    // get type and member var
    const std::string* typeId = exprBT->getId();
    Class* _class = exprBT->lookupClass();
    Class::MemberVarMap::const_iterator iter = _class->memberVars_.find(id_);

    if ( iter == _class->memberVars_.end() )
    {
        errorf( line_, "class '%s' does not have a member named %s", 
                typeId->c_str(), id_->c_str() );

        return false;
    }

    MemberVar* member = iter->second;
    type_ = member->type_->clone();

    structOffset_ = new me::StructOffset(_class->meStruct_, member->meMember_);

    if (ma)
    {
        ma->structOffset_->next_ = structOffset_;
        rootStructOffset_ = ma->rootStructOffset_;
    }
    else
        rootStructOffset_ = structOffset_;

    // create new place for the right most access if applicable
    if ( !isNeededAsLValue()  && right_)
        place_ = type_->createVar();

    if ( right_ && !isNeededAsLValue()  )
        genSSA();

    return true;
}

void MemberAccess::genSSA()
{
    me::Load* load = new me::Load( (me::Var*) place_, memPlace_, rootStructOffset_ );
    me::functab->appendInstr(load); 
}

//void MemberAccess::genStores()
//{
    //swiftAssert( dynamic_cast<me::Var*>(place_),
            //"expr_->place must be a me::Reg*" );

    //if ( typeid(*expr_) == typeid(MemberAccess))
    //{
        //MemberAccess* ma = (MemberAccess*) expr_;

        //me::Store* store = new me::Store( 
                //(me::Var*) ma->place_,              // memory variable
                //(me::Var*) exprList_->expr_->place_,// argument 
                //ma->rootStructOffset_);             // offset 
        //me::functab->appendInstr(store);
    //}
    //else
    //{
        //me::functab->appendInstr( 
                //new me::AssignInstr(kind_ , (me::Reg*) expr_->place_, exprList_->expr_->place_) );
    //}
//}

std::string MemberAccess::toString() const
{
    return expr_->toString() + "." + *id_;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Nil::Nil(Type* innerType, int line)
    : Expr(line)
    , innerType_(innerType)
{}

Nil::~Nil()
{
    delete innerType_;
}

/*
 * virtual methods
 */

bool Nil::analyze()
{
    if ( !innerType_->validate() )
        return false;
    
    type_ = new Ptr( CONST, innerType_->clone() );

    me::Const* literal = me::functab->newConst(me::Op::R_PTR);
    literal->value_.ptr_ = 0;
    place_ = literal;

    return true;
}

void Nil::genSSA() 
{
}

std::string Nil::toString() const
{
    return "nil{" + innerType_->toString() + "}";
}

//------------------------------------------------------------------------------

/*
 * constructor 
 */

Self::Self(int line)
    : Expr(line)
{}

/*
 * virtual methods
 */

bool Self::analyze()
{
    int methodQualifier = symtab->currentMethod()->methodQualifier_;

    int typeQualifier;
    switch (methodQualifier)
    {
        case READER:
            typeQualifier = CONST;
            break;

        case WRITER:
            typeQualifier = VAR;
            break;

        case ROUTINE:
            errorf(line_, "routines do not have a 'self' pointer");
            return false;

        case ASSIGN:
            errorf(line_, "assign operators do not have a 'self' pointer");
            return false;

        default:
            swiftAssert(false, "unreachable code");
            return false;
    }

    type_ = new Ptr( CONST, new BaseType(typeQualifier, symtab->currentClass()) );
    me::Reg* reg = me::functab->newReg(me::Op::R_PTR);
    // TODO
    place_ = reg;

    return true;
}

void Self::genSSA() 
{
}

std::string Self::toString() const
{
    return "self";
}


} // namespace swift
