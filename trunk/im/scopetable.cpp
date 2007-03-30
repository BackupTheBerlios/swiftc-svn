#include "scopetable.h"

#include <iostream>
#include <sstream>
#include <algorithm>

#include "utils/assert.h"

using namespace std;

ScopeTab scopetab;

// -----------------------------------------------------------------------------

inline void ScopeTable::insert(PseudoReg* reg)
{
    swiftAssert(reg->id_, "no id in this reg");

    pair<Scope::RegMap::iterator, bool> p
        = currentScope()->regs_.insert( std::make_pair(reg->id_, reg) );

    swiftAssert(p.second, "there is already a reg with this id in the map");
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

    PseudoReg* reg = new PseudoReg( new string(oss.str()), regType);
    insert(reg);

    return reg;
}

PseudoReg* ScopeTable::lookupReg(std::string* id, int revision)
{
    Scope* scopeIter = currentScope();

    // propagate the tree up until a function is found
    while (true)
    {
        Scope::RegMap::iterator regIter
            = scopeIter->regs_.find(id);
        if ( regIter != currentScope()->regs_.end() )
            return regIter->second;

        // not found, so climb up
        scopeIter = scopeIter->parent_;
//         ( typeid(*iter) == typeid(Function) )
        // TODO assertions
    }

    return 0;
}
