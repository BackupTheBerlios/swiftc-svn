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

#include "fe/call.h"
#include "fe/decl.h"
#include "fe/error.h"
#include "fe/exprlist.h"
#include "fe/memberfunction.h"
#include "fe/signature.h"
#include "fe/symtab.h"
#include "fe/tupel.h"
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
    , tupel_(0)
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
};

/*
 * further methods
 */

bool FunctionCall::analyzeArgs()
{
    return exprList_ 
        ? exprList_->analyze() 
        : true; // true when there is no ExprList
}

void FunctionCall::setTupel(Tupel* tupel)
{
    tupel_ = tupel;
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
    TypeList argTypeList = exprList_ 
        ? exprList_->getTypeList() 
        : TypeList(); // use empty TypeList when there is no ExprList

    if ( returnType_ && !returnType_->validate() )
        return false;

    if ( !tupel_ && exprList_ && !exprList_->analyze() )
        return false;

    PlaceList argPlaceList = exprList_ 
        ?  exprList_->getPlaceList() 
        : PlaceList(); // use empty PlaceList when there is no ExprList

    // append in-params
    for (size_t i = 0; i < argPlaceList.size(); ++i)
        in_.push_back( argPlaceList[i] );

    if (returnType_)
        out_.push_back( returnType_->createVar() );

    // set place and type as it is needed by the parent expr
    place_ = out_.empty() ? 0 : out_[0];
    type_  = returnType_ ? returnType_->clone() : 0;

    /*
     * create actual call
     */

    me::CallInstr* call = new me::CallInstr( 
            out_.size(), in_.size(), *id_, kind_ == 'v' ? true : false );

    for (size_t i = 0; i < in_.size(); ++i)
        call->arg_[i] = me::Arg( in_[i] );

    for (size_t i = 0; i < out_.size(); ++i)
        call->res_[i] = me::Res( out_[i] );

    me::functab->appendInstr(call); 

    //if (tupel_)
        //tupel_->emitStoreIfApplicable(this);

    return true;
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
    if (!tupel_)
    {
        if ( exprList_ && !exprList_->analyze() )
            return false;
    }

    TypeList argTypeList = exprList_ 
        ? exprList_->getTypeList() 
        : TypeList(); // use empty TypeList when there is no ExprList

    if (classId_)
    {
        class_ = symtab->lookupClass(classId_);
        
        if (!class_)
        {
            errorf(line_, "class '%s' is not defined in this module", 
                    classId_->c_str() );
            return false;
        }
    }
    else
        class_ = symtab->currentClass();

    memberFunction_ = symtab->lookupMemberFunction(class_, id_, argTypeList, line_);

    if (!memberFunction_)
        return false;

    Call call(exprList_, tupel_, memberFunction_->sig_);

    if ( !call.emitCall() )
        return false;

    if (!tupel_)
    {
        // set place and type as it is needed by the parent expr
        place_ = call.getPrimaryPlace();
        type_ = call.getPrimaryType();
    }
    //else
        //tupel_->emitStoreIfApplicable(this);

    return true;
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
    TypeList argTypeList = exprList_ 
        ? exprList_->getTypeList() 
        : TypeList(); // use empty TypeList when there is no ExprList

    me::Reg* self;

    if (expr_) 
    {
        if  ( !expr_->analyze() )
            return false;

        const BaseType* bt = expr_->getType()->unnestPtr();
        class_ = bt->lookupClass();

        if ( bt->isReadOnly() && !handleReadOnlyBaseType() )
            return false;

        if ( bt->isBuiltin() )
        {
            // TODO
            memberFunction_ = symtab->lookupMemberFunction(class_, id_, argTypeList, line_);

            if (!memberFunction_)
                return false;

            if (!tupel_)
            {
                // set place and type as it is needed by the parent expr
                place_ = memberFunction_->sig_->getOutParam(0)->getType()->createVar();
                type_ = memberFunction_->sig_->getOutParam(0)->getType()->clone();
            }
            // TODO

            return true;
        }
        else
        {
            swiftAssert( dynamic_cast<const me::Var*>(expr_->getPlace()),
                    "must be castable to Var" );

            memberFunction_ = symtab->lookupMemberFunction(class_, id_, argTypeList, line_);
            self = expr_->getType()->derefToInnerstPtr( (me::Var*) expr_->getPlace() );
        }
    }
    else
    {
        class_ = symtab->currentClass();

        // TODO get rid of if else and use virtual members
        const std::type_info& currentMethodQualifier = 
            typeid( *symtab->currentMemberFunction() );

        if ( currentMethodQualifier == typeid(Reader) && !handleReadOnlyBaseType() )
            return false;
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

        memberFunction_ = symtab->lookupMemberFunction(class_, id_, argTypeList, line_);
        self = ((Method*) memberFunction_)->self_;
    }

    if (!tupel_)
    {
        if ( exprList_ && !exprList_->analyze() )
            return false;
    }

    if (!memberFunction_)
        return false;

    Call call(exprList_, tupel_, memberFunction_->sig_);
    call.addSelf(self);

    if ( !call.emitCall() )
        return false;

    if (!tupel_)
    {
        // set place and type as it is needed by the parent expr
        place_ = call.getPrimaryPlace();
        type_ = call.getPrimaryType();
    }

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

bool ReaderCall::handleReadOnlyBaseType() const
{
    return true;
}

std::string ReaderCall::concatentationStr() const
{
    return ":";
}

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

bool WriterCall::handleReadOnlyBaseType() const
{
    if (expr_)
        errorf( line_, "'writer' used on read-only location '%s'",
                expr_->toString().c_str() );
    else
        errorf(line_, "'writer' of 'self' must not be used within a 'reader'");

    return false;
}

std::string WriterCall::concatentationStr() const
{
    return ".";
}

//------------------------------------------------------------------------------

} // namespace swift
