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
        if ( !exprList_->analyze() )
            return false;
    }

    if (!result)
        return false;

    // everything ok. so insert the local
    local_ = symtab->createNewLocal(type_->clone(), id_, line_);

#ifdef SWIFT_DEBUG
    functab->newVar( local_->type_->baseType_->toRegType(), local_->varNr_, local_->id_ );
#else // SWIFT_DEBUG
    functab->newVar( local_->type_->baseType_->toRegType(), local_->varNr_ );
#endif // SWIFT_DEBUG

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


//------------------------------------------------------------------------------

AssignStatement::~AssignStatement()
{
    delete expr_;
    delete exprList_;
}

bool AssignStatement::analyze()
{
    bool result = expr_->analyze();
    result &= exprList_->analyze();

    // put the exprList_ in a more comfortable std::vector
    typedef List<Expr*> ArgList;
    ArgList argList;
    for (ExprList* iter = exprList_; iter != 0; iter = iter->next_)
        argList.append(iter->expr_);

    // only continue when analyze was correct
    if (result)
    {
        if (!expr_->lvalue_)
        {
            errorf(line_, "invalid lvalue in assignment");
            return false;
        }

        Class* _class = symtab->lookupClass(expr_->type_->baseType_->id_);

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
            {
                // -> we found a constructor
                genSSA();

                return true;
            }
        }

        errorf(line_, "no constructor found for this class with the given arguments");
    }

    return false;
}

void AssignStatement::genSSA()
{
    Var* entry = symtab->lookupVar(expr_->reg_->regNr_);
    functab->appendInstr( new AssignInstr(kind_, reg_, expr_->reg_) );
}

//------------------------------------------------------------------------------

// InitList::~InitList()
// {
//     delete child_;
//     delete next_;
//     delete expr_;
// }
//
// bool InitList::analyze()
// {
//     bool result = true;
//
//     std::vector<InitList*> initList;
//     for (InitList* iter = this; iter != 0; iter = iter->next_)
//         initList.push_back(iter);
//
//     typedef Class::Methods::iterator Iter;
//     Iter iter = symtab->class_->methods.find("create");
//     Iter last =  symtab->class_->methods.upper_bound("create");
//
//     std::vector<Method*> constructors;
//     for (; iter != last; ++iter)
//     {
//         if ( iter->second->params_.size() == initList.size() )
//             constructors.push_back(iter->second);
//     }
//
//     if ( constructors.empty() )
//         errorf( "no constructor for class %s found with %i arguments", class_->id_->c_str(), (int) initList.size() );
//
//     // try if one contructor fits
// }
//
// bool InitList::analyze()
// {
//     // check whether this is a leaf item
//     if (expr_)
//     {
//         swiftAssert(child_ == 0, "child_ must be zero, when there is an expr_");
//
//         // -> yes, it is
//         result = expr_->analyze();
//
//         // take type of this expression
//         type_ = expr_->type_->clone();
//
//         return result;
//     }
//
//     // This list keeps track of all types in this InitList
//     typedef List<Type*> TypeList;
//     TypeList typeList;
//
//     /*
//         find out all types of the init list in this level, check syntax
//         and do all this stuff recursively
//     */
//     for (InitList* iter = next_; iter != 0; iter = iter->next_)
//     {
//         // propagate to next level
//         result &= iter->child_->analyze();
//         // and take over the type
//         iter->type_ = iter->child_->type_;
//
//         typeList.append(iter->type_);
//     }
//
//     return result;
// }
//
// std::string InitList::toString() const
// {
//     std::ostringstream oss;
//     oss << "{";
//
//     if (child_)
//         oss << child_->toString();
//     else
//         oss << expr_->toString();
//
//     if (next_)
//         oss << next_->toString() << ' ';
//
//     return oss.str();
// }
