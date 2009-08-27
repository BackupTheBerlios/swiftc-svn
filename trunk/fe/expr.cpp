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

#include "fe/call.h"
#include "fe/class.h"
#include "fe/error.h"
#include "fe/exprlist.h"
#include "fe/memberfunction.h"
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
    , neededAsLValue_(false)
    , doNotLoadPtr_(false)
{}

/*
 * further methods
 */

void Expr::neededAsLValue()
{
    neededAsLValue_ = true;
}

void Expr::doNotLoadPtr()
{
    doNotLoadPtr_ = true;
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
 * virtual methods
 */

bool Literal::analyze()
{
    if ( neededAsLValue_ )
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

std::string Literal::toString() const
{
    std::ostringstream oss;

    switch (kind_)
    {
        case L_INDEX:   oss << box_.size_       << "x";     break;

        case L_INT:     oss << box_.int_;                    break;
        case L_INT8:    oss << int(box_.int8_)   << "b";     break;
        case L_INT16:   oss << box_.int16_       << "w";     break;
        case L_INT32:   oss << box_.int32_       << "d";     break;
        case L_INT64:   oss << box_.int64_       << "q";     break;
        case L_SAT8:    oss << int(box_.int8_)   << "sb";    break;
        case L_SAT16:   oss << box_.int16_       << "sw";    break;

        case L_UINT:    oss << box_.uint_;                   break;
        case L_UINT8:   oss << int(box_.uint8_)  << "ub";    break;
        case L_UINT16:  oss << box_.uint16_      << "uw";    break;
        case L_UINT32:  oss << box_.uint32_      << "ud";    break;
        case L_UINT64:  oss << box_.uint64_      << "uq";    break;
        case L_USAT8:   oss << int(box_.uint8_)  << "usb";   break;
        case L_USAT16:  oss << box_.uint16_      << "usw";   break;

        case L_TRUE:    oss << "true";                  break;
        case L_FALSE:   oss << "false";                 break;

        // hence it is real, real32 or real64

        case L_REAL:
            oss << box_.float_;
        break;
        case L_REAL32:
            oss << box_.float_;
            if ( fmod(box_.float_, 1.0) == 0.0 )
                oss << ".d";
            else
                oss << "d";
            break;
        case L_REAL64:
            oss << box_.double_;
            if ( fmod(box_.double_, 1.0) == 0.0 )
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
 * further methods
 */

me::Op::Type Literal::toType() const
{
    swiftAssert(typeMap_->find(kind_) != typeMap_->end(), 
            "must be found here");
    return typeMap_->find(kind_)->second;
}

void Literal::genSSA()
{
    me::Const* literal;
    if (simdLength_)
    {
        literal = me::functab->newConst( me::Op::toSimd(toType()), simdLength_ );
        literal->broadcast(box_);
    }
    else
    {
        literal = me::functab->newConst( toType() );
        literal->box() = box_;
    }

    place_ = literal;
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
 * virtual methods
 */

bool Id::analyze()
{
    if (!simdLength_)
    {
        var_ = symtab->lookupVar(id_);

        if (var_ == 0)
        {
            errorf(line_, "'%s' was not declared in this scope", id_->c_str());
            return false;
        }

        type_ = var_->getType()->clone();
    }
    else
    {
        const Simd* simd = dynamic_cast<const Simd*>(type_);
        if (simd)
            return true;
    }

    const Container* container = dynamic_cast<Container*>(type_);

    if ( type_->isInternalAtomic() )
    {
        if (simdLength_)
        {
            if ( type_->isAtomic() )
            {
                me::Op::Type simdType = me::Op::toSimd( type_->toMeType() );
#ifdef SWIFT_DEBUG
                std::string tmpStr = std::string("s_") + *id_;
                me::Reg* simdVar = me::functab->newReg(simdType, &tmpStr);
#else // SWIFT_DEBUG
                me::Reg* simdVar = me::functab->newReg(simdType);
#endif // SWIFT_DEBUG

                me::Pack* pack = new me::Pack( simdVar, var_->getMeVar() );
                me::functab->appendInstr(pack);
                place_ = simdVar;
            }
            else
            {
                //const BaseType* bt = type_->unnestPtr();
                // TODO pack basetype
            }
        }
        else
            place_ = var_->getMeVar();
    }
    else
    {
        if ( container && (doNotLoadPtr_ || simdLength_) )
        {
#ifdef SWIFT_DEBUG
            std::string tmpStr = std::string("ptr_") + var_->getMeVar()->id_;
            me::Reg* ptr = me::functab->newReg(me::Op::R_PTR, &tmpStr);
#else // SWIFT_DEBUG
            me::Reg* ptr = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG

            me::Load* load = new me::Load(
                    ptr, 
                    var_->getMeVar(), 
                    0, 
                    Container::createContainerPtrOffset() );

            me::functab->appendInstr(load);

            place_ = ptr;
            type_->modifier() = REF;
        }
        else if (doNotLoadPtr_)
            place_ = var_->getMeVar();
        else
        {
            // mark type as reference
            type_->modifier() = REF;

#ifdef SWIFT_DEBUG
            std::string tmpStr = std::string("p_") + var_->getMeVar()->id_;
            me::Reg* tmp = me::functab->newReg(me::Op::R_PTR, &tmpStr);
#else // SWIFT_DEBUG
            me::Reg* tmp = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG

            me::functab->appendInstr( new me::LoadPtr(tmp, var_->getMeVar(), 0, 0) );
            place_ = tmp;
        }
    }

    return true;
}

void Id::simdAnalyze(SimdAnalyses& simdAnalyzes)
{
    SimdAnalysis result;
    result.ptr_ = 0;
    result.simdLength_ = 0;

    var_ = symtab->lookupVar(id_);

    if (var_ == 0)
    {
        errorf(line_, "'%s' was not declared in this scope", id_->c_str());
        return;
    }

    type_ = var_->getType()->clone();

    const Simd* simd = dynamic_cast<const Simd*>(type_);
    result.simdLength_ = 
        simd->getInnerType()->lookupClass()->meSimdStruct_->getSimdLength();

    if (simd)
    {
#ifdef SWIFT_DEBUG
        std::string tmpStr = std::string("ptr_") + var_->getMeVar()->id_;
        me::Reg* ptr = me::functab->newReg(me::Op::R_PTR, &tmpStr);
#else // SWIFT_DEBUG
        me::Reg* ptr = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG

        me::Load* load = new me::Load(
                ptr, 
                var_->getMeVar(), 
                0, 
                Container::createContainerPtrOffset() );

        me::functab->appendInstr(load);

        place_ = ptr;
        type_->modifier() = REF;

        result.ptr_ = ptr;
    }

    simdAnalyzes.push_back(result);
}

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
 * virtual methods
 */

bool UnExpr::analyze()
{
    if ( neededAsLValue_ )
    {
        errorf(line_, "lvalue required as left operand of assignment");
        return false;
    }

    // return false when syntax is wrong
    if ( !op_->analyze() )
        return false;

    //if (c_ == '&')
        //type_ = new Ptr( VAR, op_->getType()->varClone() );
    //else 
    if (c_ == '^')
    {
        const Ptr* ptr = dynamic_cast<const Ptr*>( op_->getType() );

        if (!ptr)
        {
            errorf(op_->line_, "unary ^ tried to dereference a non-pointer");
            return false;
        }

        type_ = ptr->getInnerType()->clone();
    }
    else if (kind_ == NOT_OP)
    {
        if ( !op_->getType()->isBool() )
        {
            errorf(op_->line_, "unary ! not used with a bool");
            return false;
        }

        type_ = op_->getType()->varClone();
    }
    else
        type_ = op_->getType()->varClone();

    genSSA();

    return true;
}

void UnExpr::setSimdLength(int simdLength)
{
    simdLength_ = simdLength_;
    op_->setSimdLength(simdLength);
}

void UnExpr::simdAnalyze(SimdAnalyses& simdAnalyzes)
{
    op_->simdAnalyze(simdAnalyzes);
}

std::string UnExpr::toString() const
{
    std::string* opString = operatorToString(kind_);
    std::string result = *opString + " " + op_->toString();
    delete opString;

    return result;
}

/*
 * further methods
 */

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
            swiftAssert(kind_ == '^', "impossible switch/case value");
            kind = kind_;
    }

    me::functab->appendInstr( new me::AssignInstr(kind, var, op_->getPlace()) );
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
    literal->box().ptr_ = 0;
    place_ = literal;

    return true;
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
    int selfModifier = REF;
    const std::type_info& methodQualifier = typeid( *symtab->currentMemberFunction() );

    if ( methodQualifier == typeid(Routine) )
    {
        errorf(line_, "routines do not have a 'self' argument");
        return false;
    }
    else if ( methodQualifier == typeid(Operator) )
    {
        errorf(line_, "operators do not have a 'self' argument");
        return false;
    }
    else if ( methodQualifier == typeid(Reader) )
        selfModifier = CONST_REF;

    swiftAssert( dynamic_cast<Method*>(symtab->currentMemberFunction()),
            "must be castable to Method" );

    type_ = symtab->currentClass()->createType(selfModifier);
    place_ = ((Method*) symtab->currentMemberFunction())->self_;

    return true;
}

std::string Self::toString() const
{
    return "self";
}

//------------------------------------------------------------------------------

/*
 * constructor 
 */

SimdIndex::SimdIndex(int line)
    : Expr(line)
{}

/*
 * virtual methods
 */

bool SimdIndex::analyze()
{
    return true;
}

std::string SimdIndex::toString() const
{
    return "@";
}

} // namespace swift
