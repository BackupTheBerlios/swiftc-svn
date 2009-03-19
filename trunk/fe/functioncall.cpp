#include "fe/functioncall.h"

#include "fe/error.h"
#include "fe/exprlist.h"
#include "fe/method.h"
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
            type_ = returnType_->clone();
        }
    }
    else
        type_ = 0;

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

    if (kind_ == 0)
    {
        // TODO -> global routine
    }
    else
    {
        if (classId_)
        {
            Class* _class = symtab->lookupClass(classId_);
            
            if (!_class)
            {
                errorf(line_, "class '%s' is not defined in this module", 
                        classId_->c_str() );
                return false;
            }
        }
        else
        {
            /*
             * 'self' reader or writer call
             */

            Method* method = symtab->currentMethod();

            if (method->methodQualifier_ == ROUTINE)
            {
                result = false;
                errorf(line_, "routines do not have a self pointer");
            }
        }
    }

    return result;
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

    if (expr_) 
    {
        /*
         * class reader, writer or routine call
         */

        result &= expr_->analyze();
    }

    //if (result)
        //genSSA();

    return result;
}

} // namespace swift
