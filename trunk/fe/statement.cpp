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
    Local* local = new Local(type_->clone(), id_, line_, 0);
    symtab->insert(local);

    swiftAssert( typeid(*local->type_->baseType_) == typeid(SimpleType), "TODO" );

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
    delete ifBranch_;
    delete elBranch_;
}

bool IfElStatement::analyze()
{
    bool result = expr_->analyze();

    // check whether expr_ is a bool expr only if analyze resulted true
    if (result)
    {
        if ( !expr_->type_->isBool() )
        {
            errorf(line_, "the prefacing expression of an if statement must be a bool expression");
            result = false;
        }
    }

    LabelInstr* ifLabel = new LabelInstr();
    LabelInstr* elLabel = new LabelInstr();
    LabelInstr* endifLabel = new LabelInstr();

    // generate IfInstr if types are correct
    if (result)
    {
        functab->appendInstr( new BranchInstr(expr_->reg_, ifLabel, elLabel) );
        functab->appendInstr(ifLabel);
    }

    Scope* current = symtab->currentScope();

    Scope* ifScope = new Scope(current);
    current->childScopes_.append(ifScope);
    symtab->enterScope(ifScope);

    // analyze each statement in the if branch and keep acount of the result
    for (Statement* iter = ifBranch_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    symtab->leaveScope();

    if (result)
        functab->appendInstr( new GotoInstr(endifLabel) );

    if (!elBranch_)
        // here is neither an else nor an elif
        return result;

    Scope* elScope = new Scope(current);
    current->childScopes_.append(elScope);
    symtab->enterScope(elScope);

    functab->appendInstr(elLabel);

    // analyze each statement in the if branch and keep acount of the result
    for (Statement* iter = ifBranch_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    symtab->leaveScope();

    if (result)
        functab->appendInstr(endifLabel);

    return result;
}
