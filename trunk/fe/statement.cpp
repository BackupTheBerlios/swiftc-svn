#include "statement.h"

#include <sstream>
#include <typeinfo>

#include "fe/class.h"
#include "fe/error.h"
#include "fe/expr.h"
#include "fe/method.h"
#include "fe/symtab.h"
#include "fe/type.h"
#include "fe/var.h"

#include "me/functab.h"
#include "me/ssa.h"

namespace swift {

/*
    constructor and destructor
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

/**
 * Used internally by Declaration and AssignStatement.
 */
struct Assignment
{
    Type*       type_;      ///< Type of the rvalue.
    ExprList*   exprList_;  ///< The rvalue.
    int line_;

/*
    constructor
*/

    Assignment(Type* type, ExprList* exprList, int line)
        : type_(type)
        , exprList_(exprList)
        , line_(line)
    {}

/*
    further methods
*/
    bool analyze();
};

bool Assignment::analyze()
{
    bool result = true;
    result &= exprList_->analyze();

    if (result == false)
        return false;

    // put the exprList_ in a more comfortable List
    typedef List<Expr*> ArgList;
    ArgList argList;
    for (ExprList* iter = exprList_; iter != 0; iter = iter->next_)
        argList.append(iter->expr_);

    Class* _class = symtab->lookupClass(type_->baseType_->id_);

    std::string createStr("create");
    Class::MethodIter iter = _class->methods_.find(&createStr);
    swiftAssert( iter != _class->methods_.end(), "TODO");
    Class::MethodIter last = _class->methods_.upper_bound(&createStr);

    for (; iter != last; ++iter)
    {
        Method* create = iter->second;

        if ( create->sig_.params_.size() != argList.size() )
            continue; // the number of arguments does not match

        // -> number of arguments fits, so check types
        ArgList::Node* argIter = argList.first();
        Sig::Params::Node* createIter = create->sig_.params_.first();

        bool argCheckResult = true;

        while ( argIter != argList.sentinel() && argCheckResult )
        {
            argCheckResult = Type::check( argIter->value_->type_, createIter->value_->type_);

            // move forward
            argIter = argIter->next_;
            createIter = createIter->next_;
        }

        if (argCheckResult)
            return true;
    }

    errorf(line_, "no constructor found for this class with the given arguments");

    return false;
}

//------------------------------------------------------------------------------

/*
    constructor and destructor
*/

Declaration::Declaration(Type* type, std::string* id, ExprList* exprList, int line /*= NO_LINE*/)
    : Statement(line)
    , type_(type)
    , id_(id)
    , exprList_(exprList)
    , local_(0) // This will be created in analyze
{}


Declaration::~Declaration()
{
    delete type_;
    delete local_;
    delete exprList_;
}

/*
    further methods
*/

bool Declaration::analyze()
{
    bool result = true;

    // check whether this type exists
    result &= type_->validate();

    // do we have an initialization here?
    if (exprList_)
    {
        Assignment assignment(type_, exprList_, line_);
        result &= assignment.analyze();
    }

    if (!result)
        return false;

    // everything ok. so insert the local
    local_ = symtab->createNewLocal(type_, id_, line_);

#ifdef SWIFT_DEBUG
    me::Reg* reg = me::functab->newVar( local_->type_->baseType_->toType(), local_->varNr_, local_->id_ );
#else // SWIFT_DEBUG
    me::Reg* reg = me::functab->newVar( local_->type_->baseType_->toType(), local_->varNr_ );
#endif // SWIFT_DEBUG

    if (exprList_)
    {
        if ( type_->isBuiltin() )
            me::functab->appendInstr( new me::AssignInstr('=', reg, exprList_->expr_->place_) );
        else
            swiftAssert(false, "TODO");
    }

    return true;
}

//------------------------------------------------------------------------------

/*
    constructor and destructor
*/

AssignStatement::AssignStatement(int kind, Expr* expr, ExprList* exprList, int line /*= NO_LINE*/)
    : Statement(line)
    , kind_(kind)
    , expr_(expr)
    , exprList_(exprList)
{}

AssignStatement::~AssignStatement()
{
    delete expr_;
    delete exprList_;
}

/*
    further methods
*/

bool AssignStatement::analyze()
{
    bool result = expr_->analyze();

    Assignment assignment(expr_->type_, exprList_, line_);
    result &= assignment.analyze();

    if (result)
    {
        genSSA();

        return true;
    }
    // else

    return false;
}

void AssignStatement::genSSA()
{
    if ( expr_->type_->isBuiltin() )
    {
        swiftAssert( dynamic_cast<me::Reg*>(expr_->place_), "expr_->place must be a me::Reg*");
        me::functab->appendInstr( new me::AssignInstr(kind_ , (me::Reg*) expr_->place_, exprList_->expr_->place_) );
    }
    else
        swiftAssert(false, "TODO");
}

//------------------------------------------------------------------------------

/*
    constructor and destructor
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
    further methods
*/

bool WhileStatement::analyze()
{
    bool result = expr_->analyze();

    // check whether expr_ is a boolean expr only if analyze resulted true
    if (result)
    {
        if ( !expr_->type_->isBool() )
        {
            errorf(line_, "the prefacing expression of an if statement must be a boolean expression");
            result = false;
        }
    }

    // create labels
    me::InstrNode* whileLabelNode = new me::InstrList::Node( new me::LabelInstr() );
    me::InstrNode* trueLabelNode  = new me::InstrList::Node( new me::LabelInstr() );
    me::InstrNode* nextLabelNode  = new me::InstrList::Node( new me::LabelInstr() );

    /*
        generate this SSA code:

        whileLabelNode:
            IF expr THEN trueLabelNode ELSE nextLabelNode
        trueLabelNode:
            //...
            GOTO whileLabelNode
        nextLabelNode:
            //...
    */
    if (result)
    {
        // generate instructions as you can see above if types are correct
        me::functab->appendInstrNode(whileLabelNode);
        me::functab->appendInstr( new me::BranchInstr(expr_->place_, trueLabelNode, nextLabelNode) );
        me::functab->appendInstrNode(trueLabelNode);
    }

    // update scoping
    symtab->createAndEnterNewScope();

    // analyze each statement in the loop and keep acount of the result
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    // return to parent scope
    symtab->leaveScope();

    // generate instructions as you can see above
    if (result)
    {
        me::functab->appendInstr( new me::GotoInstr(whileLabelNode) );
        me::functab->appendInstrNode(nextLabelNode);
    }

    return result;
}

//------------------------------------------------------------------------------

/*
    constructor and destructor
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
    further methods
*/

bool IfElStatement::analyze()
{
    bool result = expr_->analyze();

    // check whether expr_ is a boolean expr only if analyze resulted true
    if (result)
    {
        if ( !expr_->type_->isBool() )
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
            so here is only a plain if statement;
            generate this SSA code:

                IF expr THEN trueLabelNode ELSE nextLabelNode
            trueLabelNode:
                //...
            nextLabelNode:
                //...
        */
        if (result)
        {
            // generate me::BranchInstr if types are correct
            me::functab->appendInstr( new me::BranchInstr(expr_->place_, trueLabelNode, nextLabelNode) );
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
            so we have an if-else-construct
            generate this SSA code:

            IF expr THEN trueLabelNode ELSE falseLabelNode
            trueLabelNode:
                //...
                GOTO nextLabelNode
            falseLabelNode:
                //...
            nextLabelNode:
                //...
        */
        me::InstrNode* falseLabelNode = new me::InstrList::Node( new me::LabelInstr() );

        if (result)
        {
            // generate me::BranchInstr if types are correct
            me::functab->appendInstr( new me::BranchInstr(expr_->place_, trueLabelNode, falseLabelNode) );
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
            now the else branch
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

} // namespace swift
