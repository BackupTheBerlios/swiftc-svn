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

#include "statement.h"

#include <sstream>
#include <typeinfo>

#include "fe/class.h"
#include "fe/decl.h"
#include "fe/error.h"
#include "fe/expr.h"
#include "fe/exprlist.h"
#include "fe/method.h"
#include "fe/signature.h"
#include "fe/symtab.h"
#include "fe/tupel.h"
#include "fe/type.h"
#include "fe/var.h"

#include "me/functab.h"
#include "me/offset.h"
#include "me/ssa.h"
#include "me/struct.h"

namespace swift {

/*
 * constructor and destructor
 */

Statement::Statement(int line)
    : Node(line)
    , next_(0)
{}

Statement::~Statement()
{
    if (next_)
        delete next_;
};

//------------------------------------------------------------------------------

ExprStatement::~ExprStatement()
{
    delete expr_;
}

bool ExprStatement::analyze()
{
    return expr_->analyze();
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

DeclStatement::DeclStatement(Decl* decl, int line /*= NO_LINE*/)
    : Statement(line)
    , decl_(decl)
{}


DeclStatement::~DeclStatement()
{
    delete decl_;
}

/*
 * further methods
 */

bool DeclStatement::analyze()
{
    bool result = decl_->analyze();

    if (result)
    {
        me::Var* op = (me::Var*) decl_->getPlace();
        me::functab->appendInstr( 
                new me::AssignInstr('=', op, me::functab->newUndef(op->type_)) );
    }

    return result;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

AssignStatement::AssignStatement(int kind, Tupel* tupel, ExprList* exprList, int line /*= NO_LINE*/)
    : Statement(line)
    , kind_(kind)
    , tupel_(tupel)
    , exprList_(exprList)
{}

AssignStatement::~AssignStatement()
{
    delete tupel_;
    delete exprList_;
}

/*
 * further methods
 */

bool AssignStatement::analyze()
{
    // chack params
    bool result = exprList_->analyze();

    // check result tupel
    result &= tupel_->analyze();

    if (!result)
        return false;

    TypeList argTypeList = exprList_->getTypeList();
    TypeList resTypeList = tupel_->getTypeList();

    swiftAssert( argTypeList.size() > 0, "must have at least one element" );
    swiftAssert( resTypeList.size() > 0, "must have at least one element" );

    if ( argTypeList.size() > 1 && resTypeList.size() > 1 )
    {
        errorf(line_, "either the left-hand side or the right-hand side of an" 
                "assignment statement must have exactly one element");

        return false;
    }

    if (resTypeList.size() == 1)
    {
        // -> this is a constructor call
        swiftAssert( typeid(*resTypeList[0]) == typeid(BaseType), "TODO" );
        const BaseType* bt = (const BaseType*) resTypeList[0];
        Class* _class = bt->lookupClass();

        std::string createStr("create");
        Class::MethodMap::const_iterator iter = _class->methods_.find(&createStr);
        swiftAssert( iter != _class->methods_.end(), "TODO");
        Class::MethodMap::const_iterator last = _class->methods_.upper_bound(&createStr);

        for (; iter != last; ++iter)
        {
            Method* create = iter->second;

            if ( !create->sig_->check(argTypeList) )
                continue; // the signature does not fit

            genConstructorCall(_class, create);
            return true;
        }

        errorf( line_, "no constructor found for class %s with the given arguments", 
                _class->id_->c_str() );

        return false;
    }
    else
    {
        // -> it is an ordinary call of a function
        FunctionCall* fc = exprList_->getFunctionCall();
        if (!fc)
        {
            errorf(line_, "the right-hand side of an assignment statement with"
                    "more than one item on the left-hand side" 
                    "must be a function call");

            return false;
        }

        // TODO
    }

    genSSA();

    // TODO
    //if (typeid(*expr_) == typeid(MemberAccess) )
        //delete ((MemberAccess*) expr_)->rootStructOffset_;

    return true;
}

void AssignStatement::genConstructorCall(Class* _class, Method* /*method*/)
{
    PlaceList lhsPlaces = tupel_->getPlaceList();
    PlaceList rhsPlaces = exprList_->getPlaceList();

    if ( BaseType::isBuiltin(_class->id_) )
    {
        me::functab->appendInstr( 
                new me::AssignInstr(kind_ , (me::Reg*) lhsPlaces[0], rhsPlaces[0]) );
        //tupel_->genStores();
        return;
    }

    // else TODO
    swiftAssert(false, "TODO");
}

void AssignStatement::genSSA()
{
    //swiftAssert( dynamic_cast<me::Var*>(expr_->place_),
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
}

//------------------------------------------------------------------------------

/*
 *  constructor and destructor
 */

WhileStatement::WhileStatement(Expr* expr, Statement* statements, int line /*= NO_LINE*/)
        : Statement(line)
        , expr_(expr)
        , statements_(statements)
{}

WhileStatement::~WhileStatement()
{
    delete expr_;
    delete statements_;
}

/*
 * further methods
 */

bool WhileStatement::analyze()
{
    /*
     * generate this SSA code:
     *
     * whileLabelNode:
     *     expr
     *     IF expr_result THEN trueLabelNode ELSE nextLabelNode
     * trueLabelNode:
     *     //...
     *     GOTO whileLabelNode
     * nextLabelNode:
     *     //...
     */

    me::InstrNode* whileLabelNode  = new me::InstrList::Node( new me::LabelInstr() );
    me::functab->appendInstrNode(whileLabelNode);

    bool result = expr_->analyze();

    // check whether expr_ is a boolean expr only if analyze resulted true
    if (result)
    {
        if ( !expr_->getType()->isBool() )
        {
            errorf(line_, "the prefacing expression of an while statement must be a boolean expression");
            result = false;
        }
    }

    // create labels
    me::InstrNode* trueLabelNode  = new me::InstrList::Node( new me::LabelInstr() );
    me::InstrNode* nextLabelNode  = new me::InstrList::Node( new me::LabelInstr() );

    if (result)
    {
        // generate instructions as you can see above if types are correct
        me::functab->appendInstr( new me::BranchInstr(expr_->getPlace(), trueLabelNode, nextLabelNode) );
        me::functab->appendInstrNode(trueLabelNode);
    }

    // update scoping
    symtab->createAndEnterNewScope();

    // analyze each statement in the loop and keep acount of the result
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    // generate instructions as you can see above
    if (result)
    {
        me::functab->appendInstr( new me::GotoInstr(whileLabelNode) );
        me::functab->appendInstrNode(nextLabelNode);
    }

    // return to parent scope
    symtab->leaveScope();

    return result;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

IfElStatement::IfElStatement(Expr* expr, Statement* ifBranch, Statement* elBranch, int line /*= NO_LINE*/)
    : Statement(line)
    , expr_(expr)
    , ifBranch_(ifBranch)
    , elBranch_(elBranch)
{}

IfElStatement::~IfElStatement()
{
    delete expr_;
    delete ifBranch_;
    if (elBranch_)
        delete elBranch_;
}

/*
 * further methods
 */

bool IfElStatement::analyze()
{
    bool result = expr_->analyze();

    // check whether expr_ is a boolean expr only if analyze resulted true
    if (result)
    {
        if ( !expr_->getType()->isBool() )
        {
            errorf(line_, "the prefacing expression of an if statement must be a boolean expression");
            result = false;
        }
    }

    // create labels
    me::InstrNode* trueLabelNode  = new me::InstrList::Node( new me::LabelInstr() );
    me::InstrNode* nextLabelNode  = new me::InstrList::Node( new me::LabelInstr() );

    if (!elBranch_)
    {
        /*
         * so here is only a plain if statement;
         * generate this SSA code:
         *
         *     IF expr THEN trueLabelNode ELSE nextLabelNode
         * trueLabelNode:
         *     //...
         * nextLabelNode:
         *     //...
         */

        if (result)
        {
            // generate me::BranchInstr if types are correct
            me::functab->appendInstr( new me::BranchInstr(expr_->getPlace(), trueLabelNode, nextLabelNode) );
            me::functab->appendInstrNode(trueLabelNode);
        }

        // update scoping
        symtab->createAndEnterNewScope();

        // analyze each statement in the if branch and keep acount of the result
        for (Statement* iter = ifBranch_; iter != 0; iter = iter->next_)
            result &= iter->analyze();

        // return to parent scope
        symtab->leaveScope();

        // generate instructions as you can see above
        if (result)
            me::functab->appendInstrNode(nextLabelNode);
    }
    else
    {
        /*
         * so we have an if-else-construct
         * generate this SSA code:
         *
         * IF expr THEN trueLabelNode ELSE falseLabelNode
         * trueLabelNode:
         *     //...
         *     GOTO nextLabelNode
         * falseLabelNode:
         *     //...
         * nextLabelNode:
         *     //...
         */

        me::InstrNode* falseLabelNode = new me::InstrList::Node( new me::LabelInstr() );

        if (result)
        {
            // generate me::BranchInstr if types are correct
            me::functab->appendInstr( new me::BranchInstr(expr_->getPlace(), trueLabelNode, falseLabelNode) );
            me::functab->appendInstrNode(trueLabelNode);
        }

        // update scoping
        symtab->createAndEnterNewScope();

        // analyze each statement in the if branch and keep acount of the result
        for (Statement* iter = ifBranch_; iter != 0; iter = iter->next_)
            result &= iter->analyze();

        // return to parent scope
        symtab->leaveScope();

        if (result)
        {
            // generate instructions as you can see above
            me::functab->appendInstr( new me::GotoInstr(nextLabelNode) );
            me::functab->appendInstrNode(falseLabelNode);
        }

        /*
         * now the else branch
         */

        // update scoping
        symtab->createAndEnterNewScope();

        // analyze each statement in the else branch and keep acount of the result
        for (Statement* iter = elBranch_; iter != 0; iter = iter->next_)
            result &= iter->analyze();

        // return to parent scope
        symtab->leaveScope();

        // generate instructions as you can see above
        if (result)
            me::functab->appendInstrNode(nextLabelNode);
    }

    return result;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

CFStatement::CFStatement(int kind, int line /*= NO_LINE*/)
    : Statement(line)
    , kind_(kind)
{}

/*
 * further methods
 */

bool CFStatement::analyze()
{
    me::functab->appendInstr( new me::GotoInstr(me::functab->getLastLabelNode()) );
    return true;
}

} // namespace swift
