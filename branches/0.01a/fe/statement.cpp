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

#include "fe/statement.h"

#include <sstream>
#include <typeinfo>

#include "fe/call.h"
#include "fe/class.h"
#include "fe/decl.h"
#include "fe/error.h"
#include "fe/expr.h"
#include "fe/exprlist.h"
#include "fe/functioncall.h"
#include "fe/memberfunction.h"
#include "fe/simdanalysis.h"
#include "fe/simdprefix.h"
#include "fe/signature.h"
#include "fe/symtab.h"
#include "fe/tuple.h"
#include "fe/type.h"
#include "fe/var.h"

#include "me/functab.h"
#include "me/offset.h"
#include "me/ssa.h"
#include "me/struct.h"

namespace swift {

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Statement::Statement(location loc)
    : Node(loc)
    , next_(0)
{}

Statement::~Statement()
{
    if (next_)
        delete next_;
};

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

DeclStatement::DeclStatement(Decl* decl, location loc)
    : Statement(loc)
    , decl_(decl)
{
    decl_->setAsStandAlone();
}

DeclStatement::~DeclStatement()
{
    delete decl_;
}

/*
 * further methods
 */

bool DeclStatement::analyze()
{
    if ( !decl_->analyze() )
        return false;

    const BaseType* bt = dynamic_cast<const BaseType*>( decl_->getType() );
    if (bt)
    {
        Class* _class = bt->lookupClass();

        if (_class->defaultCreate_ == Class::DEFAULT_NONE)
        {
            errorf( loc_, "class '%s' does not provide a default constructor 'create()'",
                    _class->id_->c_str() );

            return false;
        }
    }

    return true;
}

std::string DeclStatement::toString() const
{
    return decl_->toString();
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

ActionStatement::ActionStatement(SimdPrefix* simdPrefix, location loc)
    : Statement(loc)
    , simdPrefix_(simdPrefix)
{}

ActionStatement::~ActionStatement()
{
    delete simdPrefix_;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

ExprStatement::ExprStatement(SimdPrefix* simdPrefix, Expr* expr, location loc)
    : ActionStatement(simdPrefix, loc)
    , expr_(expr)
{}

ExprStatement::~ExprStatement()
{
    delete expr_;
}

bool ExprStatement::analyze()
{
    bool result = true;
    SimdAnalysis simdAnalysis(loc_);

    if (simdPrefix_)
    {
        expr_->simdAnalyze(simdAnalysis);

        int simdLength = simdAnalysis.checkAndGetSimdLength();
        if (simdLength == -1)
            return false;

        expr_->setSimdLength(simdLength);
        result &= simdPrefix_->analyze();
        simdPrefix_->genPreSSA();
    }

    result &= expr_->analyze();

    if (result && simdPrefix_)
        simdPrefix_->genPostSSA(simdAnalysis); 

    return result;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

AssignStatement::AssignStatement(
        SimdPrefix* simdPrefix, 
        int kind, 
        Tuple* tuple, 
        ExprList* exprList, 
        location loc)
    : ActionStatement(simdPrefix, loc)
    , kind_(kind)
    , tuple_(tuple)
    , exprList_(exprList)
{}

AssignStatement::~AssignStatement()
{
    delete tuple_;
    delete exprList_;
}

/*
 * virtual methods
 */

bool AssignStatement::analyze()
{
    bool result = true;
    SimdAnalysis simdAnalysis(loc_);
    int simdLength = 0;

    if (simdPrefix_)
    {
        exprList_->simdAnalyze(simdAnalysis);
        tuple_->simdAnalyze(simdAnalysis);

        simdLength = simdAnalysis.checkAndGetSimdLength();
        if (simdLength == -1)
            return false;

        exprList_->setSimdLength(simdLength);
        tuple_->setSimdLength(simdLength);

        result &= simdPrefix_->analyze();
        simdPrefix_->genPreSSA();
    }

    if ( exprList_->moreThanOne() )
    {
        if ( tuple_->moreThanOne() )
        {
            errorf(loc_, "either the left-hand side or the right-hand side of an "
                    "assignment statement must have exactly one element");

            return false;
        }

        return analyzeAssignCreate(simdLength);
    }

    FunctionCall* fc = exprList_->getFunctionCall();

    if (fc)
        result &= analyzeFunctionCall();
    else
        result &= analyzeAssignCreate(simdLength);

    if (result && simdPrefix_)
        simdPrefix_->genPostSSA(simdAnalysis);

    return result;
}

/*
 * further methods
 */

bool AssignStatement::constCheck()
{
    // check whether there is a const type in out
    for (const Tuple* iter = tuple_; iter != 0; iter = iter->next())
    {
        const TypeNode* typeNode = iter->typeNode();

        const Expr* expr = dynamic_cast<const Expr*>(typeNode);
        if ( expr && expr->getType()->isReadOnly() )
        {
            const Id* id = dynamic_cast<const Id*>(expr);

            if (id)
            {
                errorf(loc_, "assignment of read-only variable '%s'", 
                        id->id_->c_str() );
                return false;
            }
            else
            {
                errorf(loc_, "assignment of read-only location '%s'", 
                        expr->toString().c_str() );
                return false;
            }
        }   
    }

    return true;
}

bool AssignStatement::analyzeFunctionCall()
{
    // it is guaranteed here that exprList_ has exactly one element

    // if the rhs has only one item it has already been checked that fc is valid
    FunctionCall* fc = exprList_->getFunctionCall();

    if (!fc)
    {
        errorf(loc_, "the right-hand side of an assignment statement with"
                "more than one item on the left-hand side" 
                "must be a function call");

        return false;
    }

    bool result = fc->analyzeArgs();
    result &= tuple_->analyze();

    if (!result)
        return false;

    if ( !constCheck() )
        return false;

    fc->setTuple(tuple_);

    if ( !((Expr*) fc)->analyze() )
        return false;

    //TypeList out = tuple_->getTypeList();

    //if ( out[0]->isAtomic() )
        //atomicAssignment();

    //if ( !exprList_->moreThanOne() )
        //tuple_->emitStoreIfApplicable( exprList_->getExpr() );

    tuple_->emitStoreIfApplicable(fc);

    return true;
}

void AssignStatement::atomicAssignment()
{
    if ( !tuple_->typeNode()->isStoreNecessary() )
    {
        swiftAssert( dynamic_cast<me::Var*>(tuple_->getPlaceList()[0]), 
                "must be a Var here" );

        me::Var* lhsPlace = (me::Var*) tuple_->getPlaceList()[0];
        me::Op*  rhsPlace = exprList_->getPlaceList()[0];

        me::functab->appendInstr( new me::AssignInstr(kind_ , lhsPlace, rhsPlace) );
    }
}

bool AssignStatement::analyzeAssignCreate(int simdLength)
{
    const Decl* decl = dynamic_cast<const Decl*>(tuple_->typeNode());

    bool result = exprList_->analyze();
    result &= tuple_->analyze();

    if (!result)
        return false;

    TypeList in = exprList_->getTypeList(simdPrefix_);
    TypeList out = tuple_->getTypeList(simdPrefix_);
    
    if ( out[0]->isNonInnerBuiltin() )
    {
        result = out[0]->hasAssignCreate(in, decl, loc_);

        if (!result)
            return false;

        const Ptr* ptr = dynamic_cast<const Ptr*>( out[0] );
        if (ptr)
        {
            atomicAssignment();
            return true;
        }

        // TODO make all this R_UINT64 stuff arch independent

        const Container* container = dynamic_cast<const Container*>( out[0] );
        if (container)
        {
            if (decl)
            {
                swiftAssert( dynamic_cast<me::Var*>(tuple_->getPlaceList()[0]), 
                        "must be a Var here" );

                me::Var* location = (me::Var*) tuple_->getPlaceList()[0];
                me::Op* numElems = exprList_->getPlaceList()[0];

                // create temporaries
#ifdef SWIFT_DEBUG
                std::string ptrStr = "ptr_" + location->id_;
                std::string sizeStr = "size_" + location->id_;
                me::Reg* ptr  = me::functab->newReg(me::Op::R_PTR, &ptrStr);
                me::Reg* size = me::functab->newReg(me::Op::R_UINT64, &sizeStr);
#else // SWIFT_DEBUG
                me::Reg* ptr  = me::functab->newReg(me::Op::R_PTR);
                me::Reg* size = me::functab->newReg(me::Op::R_UINT64);
#endif // SWIFT_DEBUG

                // calculate size
                me::Const* elemSize = me::functab->newConst(me::Op::R_UINT64);
                elemSize->box().uint64_ = container->getInnerType()->sizeOf();
                me::AssignInstr* mul = 
                    new me::AssignInstr('*', size, numElems, elemSize);
                me::functab->appendInstr(mul);

                const Simd* simd = dynamic_cast<const Simd*>(container);
                if (simd)
                {
                    const BaseType* bt = simd->getInnerType()->isInner();

                    if (!bt)
                    {
                        errorf(loc_, "only simd-containers of base types are allowed");
                        return false;
                    }

                    /*
                     * do this to adjust the size:
                     * size = size + simdLength - 1
                     * size = size & !(simdLength - 1)
                     */

                    size_t simdLength = bt->lookupClass()->meSimdStruct_->getSimdLength();

                    me::Const* simdLength_minus_one = me::functab->newConst(me::Op::R_UINT64);
                    simdLength_minus_one->box().uint64_ = simdLength - 1ull;

                    me::Const* not_simdLength_minus_one = me::functab->newConst(me::Op::R_UINT64);
                    not_simdLength_minus_one->box().uint64_ = ~(simdLength - 1ull);

                    me::AssignInstr* plus = 
                        new me::AssignInstr('+', size, size, simdLength_minus_one);
                    me::functab->appendInstr(plus);

                    me::AssignInstr* _and = 
                        new me::AssignInstr(me::AssignInstr::AND, size, size, not_simdLength_minus_one);
                    me::functab->appendInstr(_and);
                }

                // malloc
                me::functab->appendInstr( new me::Malloc(ptr, size) );

                // store ptr
                me::Store* store = new me::Store( 
                        ptr, location, 0, Container::createContainerPtrOffset() ); // TODO
                me::functab->appendInstr(store);
            }
            else
            {
                // make a memcpy here
                swiftAssert(false, "TODO");
            }

            return true;
        }
    } // if isNonInnerBuiltin

    const BaseType* bt = (const BaseType*) out[0];
    Class* _class = bt->lookupClass();
    Method* assignCreate = symtab->lookupAssignCreate(_class, in, decl, loc_);

    if (!assignCreate)
        return false;

    if ( out[0]->isAtomic() )
        atomicAssignment();
    else
    {
        if ( !assignCreate->isTrivial() )
        {
            swiftAssert( typeid(*out[0]) == typeid(BaseType), "TODO" );

            Call call(exprList_, tuple_, assignCreate->sig_, simdLength);
            swiftAssert( typeid(*tuple_->typeNode()->getPlace()) == typeid(me::Reg),
                    "must be a Reg here" );
            call.addSelf( (me::Reg*) tuple_->typeNode()->getPlace() );
            call.emitCall();

            swiftAssert( !tuple_->typeNode()->isStoreNecessary(),
                    "can't emit store" );
        }
        // else -> do nothing
    }

    if ( !exprList_->moreThanOne() )
        tuple_->emitStoreIfApplicable( exprList_->getExpr() );

    return true;
}

/*
 * further methods
 */

std::string AssignStatement::toString() const
{
    return tuple_->toString() + " = " + exprList_->toString();
}

//------------------------------------------------------------------------------

/*
 *  constructor and destructor
 */

WhileStatement::WhileStatement(Expr* expr, Statement* statements, location loc)
    : Statement(loc)
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
    if (result && !expr_->getType()->isBool() )
    {
        errorf(loc_, "the prefacing expression of a while statement must be a boolean expression");
        result = false;
    }

    me::InstrNode* trueLabelNode;
    me::InstrNode* nextLabelNode;

    if (result)
    {
        // create labels
        trueLabelNode = new me::InstrList::Node( new me::LabelInstr() );
        nextLabelNode = new me::InstrList::Node( new me::LabelInstr() );

        // generate instructions as you can see above if types are correct
        me::functab->appendInstr( new me::BranchInstr(expr_->getPlace(), trueLabelNode, nextLabelNode) );
        me::functab->appendInstrNode(trueLabelNode);
    }

    // update scoping
    symtab->createAndEnterNewScope();

    // analyze each statement in the loop and keep acount of the result
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    if (result)
    {
        // generate instructions as you can see above
        me::functab->appendInstr( new me::GotoInstr(whileLabelNode) );
        me::functab->appendInstrNode(nextLabelNode);
    }

    // return to parent scope
    symtab->leaveScope();

    return result;
}

//------------------------------------------------------------------------------

/*
 *  constructor and destructor
 */

RepeatUntilStatement::RepeatUntilStatement(Statement* statements, Expr* expr, location loc)
    : Statement(loc)
    , expr_(expr)
    , statements_(statements)
{}

RepeatUntilStatement::~RepeatUntilStatement()
{
    delete statements_;
    delete expr_;
}

/*
 * further methods
 */

bool RepeatUntilStatement::analyze()
{
    /*
     * generate this SSA code:
     *
     * repeatLabelNode:
     *     //...
     *     expr
     *     IF expr_result THEN nextLabelNode ELSE repeatLabelNode
     * nextLabelNode:
     *     //...
     */

    bool result = true;

    // add repeatLabelNode
    me::InstrNode* repeatLabelNode = new me::InstrList::Node( new me::LabelInstr() );
    me::functab->appendInstrNode(repeatLabelNode);

    // update scoping
    symtab->createAndEnterNewScope();

    // analyze each statement in the loop and keep acount of the result
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    result &= expr_->analyze();

    // check whether expr_ is a boolean expr only if analyze resulted true
    if (result && !expr_->getType()->isBool() )
    {
        errorf(loc_, "the concluding expression of a do ... while statement must be a boolean expression");
        result = false;
    }

    if (result)
    {
        // create nextLabelNode
        me::InstrNode* nextLabelNode = new me::InstrList::Node( new me::LabelInstr() );

        // generate instructions as you can see above if types are correct
        me::functab->appendInstr( new me::BranchInstr(
                    expr_->getPlace(), nextLabelNode, repeatLabelNode) );
        me::functab->appendInstrNode(nextLabelNode);
    }

    // return to parent scope
    symtab->leaveScope();

    return result;
}

//------------------------------------------------------------------------------

/*
 *  constructor and destructor
 */

ScopeStatement::ScopeStatement(Statement* statements, location loc)
    : Statement(loc)
    , statements_(statements)
{}

ScopeStatement::~ScopeStatement()
{
    delete statements_;
}

/*
 * further methods
 */

bool ScopeStatement::analyze()
{
    bool result = true;

    // update scoping
    symtab->createAndEnterNewScope();

    // analyze each statement in the loop and keep acount of the result
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    symtab->leaveScope();

    return result;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

IfElStatement::IfElStatement(Expr* expr, Statement* ifBranch, Statement* elBranch, location loc)
    : Statement(loc)
    , expr_(expr)
    , ifBranch_(ifBranch)
    , elBranch_(elBranch)
{}

IfElStatement::~IfElStatement()
{
    delete expr_;
    delete ifBranch_;
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
            errorf(loc_, "the prefacing expression of an if statement must be a boolean expression");
            result = false;
        }
    }

    me::InstrNode* trueLabelNode;
    me::InstrNode* nextLabelNode;

    if (result)
    {
        // create labels
        trueLabelNode = new me::InstrList::Node( new me::LabelInstr() );
        nextLabelNode = new me::InstrList::Node( new me::LabelInstr() );
    }

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

        me::InstrNode* falseLabelNode;

        if (result)
        {
            falseLabelNode = new me::InstrList::Node( new me::LabelInstr() );

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

CFStatement::CFStatement(int kind, location loc)
    : Statement(loc)
    , kind_(kind)
{}

/*
 * further methods
 */

bool CFStatement::analyze()
{
    me::functab->appendInstr( new me::GotoInstr(me::functab->getFunctionEpilogue()) );
    return true;
}

} // namespace swift
