#include "syntaxtree.h"

#include <iostream>
#include <sstream>
#include <stack>

#include "utils/assert.h"

#include "fe/module.h"
#include "fe/symtab.h"

#include "me/ssa.h"


SyntaxTree* syntaxtree = 0;

//------------------------------------------------------------------------------

/*
    constructor
*/

Node::Node(int line = NO_LINE, Node* parent /*= 0*/)
    : line_(line)
    , parent_(parent)
{}

//------------------------------------------------------------------------------

/*
    constructor
*/

Symbol::Symbol(std::string* id, int line /*= NO_LINE*/, Node* parent /*= 0*/)
    : Node(line, parent)
    , id_(id)
{}

Symbol::~Symbol()
{
    delete id_;
}

/*
    further methods
*/

std::string Symbol::toString() const
{
    return *id_;
}

std::string Symbol::getFullName(const Node*) const
{
    // build full class name with module names: module1.module2.Class1.Class2 etc
    std::stack<std::string*> idStack;
    idStack.push(*_class->id_);

    // put all identifiers on the stack
    for (Symbol* iter = this; iter != 0; iter = iter->parent_)
        idStack.push(iter->id_);

    std::ostringstream oss;

    // and build string
    while ( !idStack.empty() )
    {
        oss << idStack.top();
        idStack.pop();

        if ( !idStack.empty() )
            oss << '.';
    }

    return oss.str();
}

//------------------------------------------------------------------------------

/*
    destructor
*/

SyntaxTree::~SyntaxTree()
{
std::cout << "destroy..." << std::endl;
    delete rootModule_;
}

/*
    further methods
*/

bool SyntaxTree::analyze()
{
    symtab->reset();
    symtab->enterModule();

    bool result = rootModule_->analyze();

    symtab->leaveModule();

    return result;
}
