#include "statement.h"

#include <sstream>
#include <typeinfo>

#include "fe/error.h"
#include "fe/expr.h"
#include "fe/symtab.h"
#include "fe/type.h"

#include "me/functab.h"
#include "me/ssa.h"

Declaration::~Declaration()
{
    delete type_;
    delete local_;
}

std::string Declaration::toString() const
{
    std::ostringstream oss;
    oss << type_->toString() << " " << *id_;

    return oss.str();
}

bool Declaration::analyze()
{
    if ( typeid(*type_->baseType_) == typeid(UserType) )
    {
        UserType* userType = (UserType*) type_->baseType_;
        // it is a user defined type - so check whether it has been defined
        if (symtab->lookupClass(userType->id_) == 0)
        {
            errorf( line_, "class '%s' is not defined in this module", type_->baseType_->toString().c_str() );
            return false;
        }
    }

    // everything ok. so insert the local
    local_ = new Local(type_->clone(), id_, line_, 0);
    local_->varNr_ = symtab->varNr();
    symtab->insert(local_);

    swiftAssert( typeid(*local_->type_->baseType_) == typeid(SimpleType), "TODO" );

    return true;
}

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

IfElStatement::~IfElStatement()
{
    delete expr_;
    delete ifBranch_;
    if (elBranch_)
        delete elBranch_;
}

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
    InstrList::Node* trueLabelNode  = new InstrList::Node( new LabelInstr() );
    InstrList::Node* nextLabelNode  = new InstrList::Node( new LabelInstr() );

    if (!elBranch_)
    {
        /*
            so here is only a plain if statement
            generate this SSA code:

                IF expr THEN trueLabelNode
            trueLabelNode:
                //...
            nextLabelNode:
                //...
        */
        if (result)
        {
            // generate BranchInstr if types are correct
            functab->appendInstr( new BranchInstr(expr_->reg_, trueLabelNode, nextLabelNode) );
            functab->appendInstrNode(trueLabelNode);
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
            functab->appendInstrNode(nextLabelNode);
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
        InstrList::Node* falseLabelNode = new InstrList::Node( new LabelInstr() );

        if (result)
        {
            // generate BranchInstr if types are correct
            functab->appendInstr( new BranchInstr(expr_->reg_, trueLabelNode, falseLabelNode) );
            functab->appendInstrNode(trueLabelNode);
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
            functab->appendInstr( new GotoInstr(nextLabelNode) );
            functab->appendInstrNode(falseLabelNode);
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
            functab->appendInstrNode(nextLabelNode);
    }

    return result;
}
