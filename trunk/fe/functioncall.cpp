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

bool FunctionCall::analyzeExprList() const
{
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
    // TODO
    //bool result = FunctionCall::analyze(argTypeList, argPlaceList);
    bool result;

    if (returnType_)
    {
        result &= returnType_->validate();

        if (result)
        {
            me::Var* var = returnType_->createVar();
            place_ = var;
            type_ = returnType_->varClone();
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

//bool MemberFunctionCall::analyze(const TypeList& argTypeList)
//{
//}

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
    // TODO
    //bool result = FunctionCall::analyze(argTypeList, argPlaceList);
    bool result;

    if (!result)
        return false;

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

    // TODO
    //return MemberFunctionCall::analyze(argTypeList);
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

        class_ = bt->lookupClass();
    }
    else
    {
        class_ = symtab->currentClass();

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

    if (!tupel_)
    {
        if ( exprList_ && !exprList_->analyze() )
            return false;
    }

    TypeList argTypeList = exprList_ 
        ? exprList_->getTypeList() 
        : TypeList(); // use empty TypeList when there is no ExprList

    PlaceList argPlaceList = exprList_
        ? exprList_->getPlaceList()
        : PlaceList(); // use empty PlaceList when there is no ExprList

    memberFunction_ = symtab->lookupMemberFunction(class_, id_, argTypeList, line_);

    if (!memberFunction_)
        return false;

    /*
     * fill in_ and out_
     */

    //if ( !MemberFunctionCall::analyze(argTypeList) )
        //return false;

    swiftAssert( memberFunction_->sig_->getNumIn() == argPlaceList.size(),
            "sizes must match here" );

    /*
     * are there locations to put the results or do we have to create them? 
     */

    if (tupel_)
    {
        PlaceList resPlaceList = tupel_->getPlaceList();

        swiftAssert( memberFunction_->sig_->getNumOut() == resPlaceList.size(),
                "sizes must match here" );

        // examine results
        Tupel* tupelIter = tupel_;
        for (size_t i = 0; i < memberFunction_->sig_->getNumOut(); ++i)
        {
            const Param* param = memberFunction_->sig_->getOutParam(i);

            if ( param->getType()->isAtomic() )
            {
                // -> this one is an ordinary out-param
                out_.push_back( (me::Var*) resPlaceList[i] );
            }
            else
            {
                // -> this one is a hidden in-param
                swiftAssert( param->getType()->isInternalAtomic(), 
                        "must actually be a pointer" );

                in_.push_back( resPlaceList[i] );
            }

            tupelIter = tupelIter->next();
        }
    }
    else
    {
        // no tupel given -> create results

        for (size_t i = 0; i < memberFunction_->sig_->getNumOut(); ++i)
        {
            const Param* param = memberFunction_->sig_->getOutParam(i);

            // create place to hold the result and init with undef
#ifdef SWIFT_DEBUG
            std::string resStr = std::string("res");
            me::Reg* res = me::functab->newReg( param->getType()->toMeType(), &resStr );
#else // SWIFT_DEBUG
            me::Reg* res = me::functab->newReg( param->getType()->toMeType() );
#endif // SWIFT_DEBUG

            me::AssignInstr* ai = new me::AssignInstr(
                    '=', res, me::functab->newUndef(res->type_) );
            me::functab->appendInstr(ai);

            if ( param->getType()->isAtomic() )
            {
                // -> this one is an ordinary out-param
                out_.push_back(res);
            }
            else
            {
#ifdef SWIFT_DEBUG
                std::string tmpStr = std::string("p_res");
                me::Reg* tmp = me::functab->newReg(me::Op::R_PTR, &tmpStr);
#else // SWIFT_DEBUG
                me::Reg* tmp = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG

                me::functab->appendInstr( new me::LoadPtr(tmp, res, 0) );

                // -> this one is a hidden in-param
                in_.push_back(tmp);
            }
        }

        /*
         * set place and type as it is needed by the parent expr
         */
        if ( out_.empty() )
        {
            place_ = 0;
            // type_ is inited with 0
        }
        else
        {
            place_ = (me::Var*) out_[0];
            type_ = memberFunction_->sig_->getOutParam(0)->getType()->varClone();
        }
    }

    // now append ordinary in-params
    for (size_t i = 0; i < argPlaceList.size(); ++i)
        in_.push_back( argPlaceList[i] );

    /*
     * create actual call
     */

    me::CallInstr* call = new me::CallInstr( 
            out_.size(), in_.size(), memberFunction_->meId_, false);
            //out_.size(), in_.size(), *id_, kind_ == 'v' ? true : false );

    for (size_t i = 0; i < in_.size(); ++i)
        call->arg_[i] = me::Arg( in_[i] );

    for (size_t i = 0; i < out_.size(); ++i)
        call->res_[i] = me::Res( out_[i], out_[i]->varNr_ );

    me::functab->appendInstr(call); 

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
