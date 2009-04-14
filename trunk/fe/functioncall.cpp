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

#include "fe/functioncall.h"

#include "fe/error.h"
#include "fe/exprlist.h"
#include "fe/memberfunction.h"
#include "fe/signature.h"
#include "fe/symtab.h"
#include "fe/type.h"

namespace swift {

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

FunctionCall::FunctionCall(std::string* id, 
                           ExprList* exprList, 
                           int kind,
                           int line /*= NO_LINE*/)
    : Expr(line)
    , id_(id)
    , exprList_(exprList)
    , kind_(kind)
{}

FunctionCall::~FunctionCall()
{
    delete exprList_;
    delete id_;
}

/*
 * virtual methods
 */

void FunctionCall::genSSA()
{
    //size_t numRes = 0;
    //if (returnType_)
        //numRes = 1;

    //PlaceList places = exprList_->getPlaceList();

    //me::CallInstr* call = new me::CallInstr( 
            //numRes, places.size(), *id_, kind_ == 'v' ? true : false );

    //for (size_t i = 0; i < places.size(); ++i)
        //call->arg_[i] = me::Arg( places[i] );

    //for (size_t i = 0; i < numRes; ++i)
        //call->res_[i] = me::Res( (me::Var*) place_, ((me::Var*) place_)->varNr_ );

    //me::functab->appendInstr(call); 
};


/*
 * further methods
 */

MemberFunction* FunctionCall::getMemberFunction()
{
    return memberFunction_;
}

void FunctionCall::analyze(bool& result, TypeList& argTypeList, PlaceList& argPlaceList) const
{
    result = exprList_ 
           ? exprList_->analyze() 
           : true; // true when there is no ExprList

    argTypeList = exprList_ 
        ? exprList_->getTypeList() 
        : TypeList(); // use empty TypeList when there is no ExprList

    argPlaceList = exprList_
        ? exprList_->getPlaceList()
        : PlaceList(); // use empty TypeList when there is no ExprList
}

bool FunctionCall::analyze(Class* _class, const TypeList& argTypeList)
{
    memberFunction_ = symtab->lookupMemberFunction(_class, id_, argTypeList, line_);

    if (!memberFunction_)
        return false;

    const TypeList& out = memberFunction_->sig_->getOut();

    if ( !out.empty() )
        type_ = out[0]->clone();
    else
        type_ = 0;

    return true;
}

std::string FunctionCall::callToString() const
{
    return *id_ + '(' + (exprList_ ? exprList_->toString() : "") + ')';
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

CCall::CCall(Type* returnType, 
             std::string* id, 
             ExprList* exprList, 
             int kind,
             int line /*= NO_LINE*/)
    : FunctionCall(id, exprList, kind, line)
    , returnType_(returnType)
{}

CCall::~CCall()
{
    if (returnType_)
        delete returnType_;
}

/*
 * virtual methods
 */

bool CCall::analyze()
{
    bool result;
    TypeList argTypeList;
    PlaceList argPlaceList;
    FunctionCall::analyze(result, argTypeList, argPlaceList);

    if (returnType_)
    {
        result &= returnType_->validate();

        if (result)
        {
            me::Var* var = returnType_->createVar();
            place_ = var;
            type_ = returnType_->constClone();
        }
    }
    else
        type_ = 0;

    return result;
}

std::string CCall::toString() const
{
    std::string result;

    if (kind_ == C_CALL)
        result = "c_call ";
    else
        result = "vc_call ";

    if (returnType_)
        result += returnType_->toString() + " ";

    result += callToString();

    return result;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

RoutineCall::RoutineCall(std::string* classId, 
                         std::string* id, 
                         ExprList* exprList, 
                         int kind,
                         int line /*= NO_LINE*/)
    : FunctionCall(id, exprList, kind, line)
    , classId_(classId)
{}

RoutineCall::~RoutineCall()
{
    if (classId_)
        delete classId_;
}

/*
 * virtual methods
 */

bool RoutineCall::analyze()
{
    bool result;
    TypeList argTypeList;
    PlaceList argPlaceList;
    FunctionCall::analyze(result, argTypeList, argPlaceList);

    if (!result)
        return false;

    Class* _class;

    if (kind_ == 0)
    {
        swiftAssert(false, "TODO -> global routine");
        _class = 0;
    }
    else
    {
        if (classId_)
        {
            _class = symtab->lookupClass(classId_);
            
            if (!_class)
            {
                errorf(line_, "class '%s' is not defined in this module", 
                        classId_->c_str() );
                return false;
            }
        }
        else
            _class = symtab->currentClass();
    }

    return FunctionCall::analyze(_class, argTypeList);
}

std::string RoutineCall::toString() const
{
    return *classId_ + "::" + callToString();
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

MethodCall::MethodCall(Expr* expr, 
                       std::string* id, 
                       ExprList* exprList, 
                       int kind,
                       int line /*= NO_LINE*/)
    : FunctionCall(id, exprList, kind, line)
    , expr_(expr)
{}

MethodCall::~MethodCall()
{
    if (expr_)
        delete expr_;
}

/*
 * further methods
 */

bool MethodCall::analyze()
{
    bool result;
    TypeList argTypeList;
    PlaceList argPlaceList;
    FunctionCall::analyze(result, argTypeList, argPlaceList);

    if (!result)
        return false;

    Class* _class;

    if (expr_) 
    {
        if  ( !expr_->analyze() )
            return false;

        const BaseType* bt = expr_->getType()->unnestPtr();

        if (bt->isReadOnly() && kind_ == WRITER)
        {
            errorf( line_, "'writer' used on read-only location '%s'",
                    expr_->toString().c_str() );
            return false;
        }

        _class = bt->lookupClass();
    }
    else
    {
        _class = symtab->currentClass();

        const std::type_info& currentMethodQualifier = 
            typeid( *symtab->currentMemberFunction() );

        if (currentMethodQualifier == typeid(Reader) && kind_ == WRITER)
        {
            errorf(line_, "'writer' of 'self' must not be used within a 'reader'");
            return false;
        }
        else if ( currentMethodQualifier == typeid(Routine) ) 
        {
            errorf(line_, "routines do not have a 'self' pointer");
            return false;
        }
        else if ( currentMethodQualifier == typeid(Operator) ) 
        {
            errorf(line_, "operators do not have a 'self' pointer");
            return false;
        }
    }

    return FunctionCall::analyze(_class, argTypeList);
}

std::string MethodCall::toString() const
{
    char access;

    if (kind_ == READER)
        access = ':';
    else
    {
        swiftAssert(kind_ == WRITER, "must be a writer");
        access = '.';
    }

    return expr_->toString() + access + callToString();
}

} // namespace swift
