#include "statement.h"

#include <sstream>
#include <typeinfo>

#include "fe/error.h"
#include "fe/expr.h"
#include "fe/symtab.h"
#include "fe/type.h"

#include "me/scopetab.h"
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

    // do first revision
    ++local->revision_;
    scopetab->newRevision( ((SimpleType*) local->type_->baseType_)->toRegType(), local->id_, local->revision_ );

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

//     // generate IfElInstr if types are correct
//     if (result)
//     {
//         scopetab->appendInstr( new IfInstr(expr_->reg_) );
//     }
//
    SwiftScope* current = symtab->currentScope();

    SwiftScope* ifScope = new SwiftScope(current);
    current->childScopes_.append(ifScope);
    symtab->enterScope(ifScope);

    scopetab->enterNewScope();
    DummyInstr* ifDummy = new DummyInstr();
    scopetab->appendInstr(ifDummy);

    // analyze each statement in the if branch and keep acount of the result
    for (Statement* iter = ifBranch_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    symtab->leaveScope();
    scopetab->leave();

    if (!elBranch_)
        // here is neither an else nor an elif
        return result;

    return true;

    SwiftScope* elScope = new SwiftScope(current);
    current->childScopes_.append(elScope);
    symtab->enterScope(elScope);

    scopetab->enterNewScope();
    DummyInstr* elDummy = new DummyInstr();
    scopetab->appendInstr(elDummy);

    // analyze each statement in the else/elif branch and keep acount of the result
    for (Statement* iter = elBranch_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    symtab->leaveScope();
    scopetab->leave();

    return result;
}
