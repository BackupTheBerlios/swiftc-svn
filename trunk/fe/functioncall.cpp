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

FunctionCall::FunctionCall(std::string* id, ExprList* exprList, int line)
    : Expr(line)
    , id_(id)
    , exprList_(exprList)
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
    me::CallInstr* call = new me::CallInstr( 
            out_.size(), in_.size(), *id_, false);
            //out_.size(), in_.size(), *id_, kind_ == 'v' ? true : false );

    for (size_t i = 0; i < in_.size(); ++i)
        call->arg_[i] = me::Arg( in_[i] );

    for (size_t i = 0; i < out_.size(); ++i)
        call->res_[i] = me::Res( out_[i], out_[i]->varNr_ );

    me::functab->appendInstr(call); 
};

/*
 * further methods
 */

MemberFunction* FunctionCall::getMemberFunction()
{
    return memberFunction_;
}

bool FunctionCall::analyze(TypeList& argTypeList, PlaceList& argPlaceList) const
{
    bool result = exprList_ 
           ? exprList_->analyze() 
           : true; // true when there is no ExprList

    argTypeList = exprList_ 
        ? exprList_->getTypeList() 
        : TypeList(); // use empty TypeList when there is no ExprList

    argPlaceList = exprList_
        ? exprList_->getPlaceList()
        : PlaceList(); // use empty TypeList when there is no ExprList

    return result;
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
             int kind,
             std::string* id, 
             ExprList* exprList, 
             int line)
    : FunctionCall(id, exprList, line)
    , returnType_(returnType)
    , kind_(kind)
{}

CCall::~CCall()
{
    delete returnType_;
}

/*
 * virtual methods
 */

bool CCall::analyze()
{
    TypeList argTypeList;
    PlaceList argPlaceList;
    bool result = FunctionCall::analyze(argTypeList, argPlaceList);

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

MemberFunctionCall::MemberFunctionCall(std::string* id, ExprList* exprList, int line)
    : FunctionCall(id, exprList, line)
{}

/*
 * further methods
 */

bool MemberFunctionCall::analyze(Class* _class, const TypeList& argTypeList)
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

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

RoutineCall::RoutineCall(std::string* classId, 
                         std::string* id, 
                         ExprList* exprList, 
                         int line)
    : MemberFunctionCall(id, exprList, line)
    , classId_(classId)
{}

RoutineCall::~RoutineCall()
{
    delete classId_;
}

/*
 * virtual methods
 */

bool RoutineCall::analyze()
{
    TypeList argTypeList;
    PlaceList argPlaceList;
    bool result = FunctionCall::analyze(argTypeList, argPlaceList);

    if (!result)
        return false;

    Class* _class;

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

    return MemberFunctionCall::analyze(_class, argTypeList);
}

std::string RoutineCall::toString() const
{
    return *classId_ + "::" + callToString();
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

MethodCall::MethodCall(Expr* expr, std::string* id, ExprList* exprList, int line)
    : MemberFunctionCall(id, exprList, line)
    , expr_(expr)
{}

MethodCall::~MethodCall()
{
    delete expr_;
}

/*
 * further methods
 */

bool MethodCall::analyze()
{
    TypeList argTypeList;
    PlaceList argPlaceList;
    bool result = FunctionCall::analyze(argTypeList, argPlaceList);

    if (!result)
        return false;

    Class* _class;

    if (expr_) 
    {
        if  ( !expr_->analyze() )
            return false;

        const BaseType* bt = expr_->getType()->unnestPtr();

        if ( bt->isReadOnly() && typeid(*this) == typeid(WriterCall) )
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

        if ( currentMethodQualifier == typeid(Reader) && typeid(*this) == typeid(WriterCall) )
        {
            errorf(line_, "'writer' of 'self' must not be used within a 'reader'");
            return false;
        }
        else if ( currentMethodQualifier == typeid(Routine) ) 
        {
            errorf(line_, "routines do not have a 'self' argument");
            return false;
        }
        else if ( currentMethodQualifier == typeid(Operator) ) 
        {
            errorf(line_, "operators do not have a 'self' argument");
            return false;
        }
    }

    /*
     * fill in_ and out_
     */

    if ( !MemberFunctionCall::analyze(_class, argTypeList) )
        return false;

    swiftAssert( dynamic_cast<Method*>(memberFunction_), 
            "must be castable to Method" );
    Method* method = (Method*) memberFunction_;

    in_.push_back(method->self_);

    genSSA();

    return true;
}

std::string MethodCall::toString() const
{
    return expr_->toString() + concatentationStr() + callToString();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

ReaderCall::ReaderCall(Expr* expr, std::string* id, ExprList* exprList, int line)
    : MethodCall(expr, id, exprList, line)
{}

/*
 * virtual methods
 */

std::string ReaderCall::concatentationStr() const
{
    return ":";
}

//bool ReaderCall::specialAnalyze() const
//{
    //return true;
//}

//------------------------------------------------------------------------------

/*
 * constructor
 */

WriterCall::WriterCall(Expr* expr, std::string* id, ExprList* exprList, int line)
    : MethodCall(expr, id, exprList, line)
{}

/*
 * virtual methods
 */

std::string WriterCall::concatentationStr() const
{
    return ".";
}

//bool WriterCall::specialAnalyze() const
//{
    //return true;
//}

//------------------------------------------------------------------------------

} // namespace swift
