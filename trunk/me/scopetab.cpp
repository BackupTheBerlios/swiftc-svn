#include "scopetab.h"

#include <iostream>
#include <sstream>
#include <algorithm>

#include "utils/assert.h"

using namespace std;

ScopeTab scopetab;

// -----------------------------------------------------------------------------

Scope::~Scope()
{
    // delete all child scopes
    for (ScopeList::Node* iter = childScopes_.first(); iter != childScopes_.sentinel(); iter = iter->next())
        delete iter->value_;

    // delete all instructions
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
        delete iter->value_;

    // delete all pseudo regs
    for (RegMap::iterator iter = regs_.begin(); iter != regs_.end(); ++iter)
        delete iter->second;
}

// -----------------------------------------------------------------------------

Function::~Function()
{
// this is currenty deleted by SymTabEntry
//     delete id_;
}

// -----------------------------------------------------------------------------

inline void ScopeTable::insert(PseudoReg* reg)
{
    swiftAssert(reg->id_, "no id in this reg");

    pair<Scope::RegMap::iterator, bool> p
        = currentScope()->regs_.insert( std::make_pair(reg->id_, reg) );

    swiftAssert(p.second, "there is already a reg with this id in the map");
}

Function* ScopeTable::insertFunction(std::string* id)
{
    Function* function = new Function(currentScope(), id);
    functions_.insert( std::make_pair(id, function) );

    return function;
}

PseudoReg* ScopeTable::newTemp(PseudoReg::RegType regType)
{
    static long counter = 0;

    ostringstream oss;
    // '@' is a magic char used to start new temps
    oss << "@" << counter;
    string* str = new string( oss.str() );

    PseudoReg* reg = new PseudoReg(str, regType);
    insert(reg);

    ++counter;

    return reg;
}

PseudoReg* ScopeTable::newRevision(PseudoReg::RegType regType, std::string* id, int revision)
{
    ostringstream oss;
    // '!' is a magic char used to divide the orignal name by the revision number
    oss << *id << "!" << revision;

    PseudoReg* reg = new PseudoReg( new string(oss.str()), regType );
    insert(reg);

    return reg;
}

PseudoReg* ScopeTable::lookupReg(std::string* id, int revision)
{
    std::string lookupId;

    if (revision != NO_REVISION)
    {
        ostringstream oss;
        // '!' is a magic char used to divide the orignal name by the revision number
        oss << *id << "!" << revision;
        lookupId = oss.str();
    }
    else
        lookupId = *id;

    Scope* scopeIter = currentScope();

    // propagate the tree up until a function is found
    while (true)
    {
        Scope::RegMap::iterator regIter
            = scopeIter->regs_.find(&lookupId);
        if ( regIter != currentScope()->regs_.end() )
            return regIter->second;

        // not found, so climb up
        scopeIter = scopeIter->parent_;
//         ( typeid(*iter) == typeid(Function) )
        // TODO assertions
    }

    return 0;
}

void ScopeTab::destroy()
{
    delete rootScope_;

    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        delete iter->second;
}

