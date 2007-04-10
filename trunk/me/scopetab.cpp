#include "scopetab.h"

#include <algorithm>
#include <sstream>
#include <typeinfo>

#include "utils/assert.h"

using namespace std;

ScopeTab* scopetab = 0;

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

void Scope::dump(ofstream& ofs)
{
    // for all instructions in this scope
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        for (size_t i = 0; i < depth_ + 1; ++i)
            ofs << '\t';
        ofs << iter->value_->toString() << std::endl;
    }

    for (ScopeList::Node* iter = childScopes_.first(); ++iter != childScopes_.sentinel(); iter = iter->next())
        iter->value_->dump(ofs);
}

// -----------------------------------------------------------------------------

void Function::dump(ofstream& ofs)
{
    for (size_t i = 0; i < depth_; ++i)
            ofs << '\t';
    ofs << *id_ << std::endl;

    // for all instructions in this scope
    for (InstrList::Node* iter = instrList_.first(); iter != instrList_.sentinel(); iter = iter->next())
    {
        for (size_t i = 0; i < depth_ + 1; ++i)
            ofs << '\t';
        ofs << iter->value_->toString() << std::endl;
    }
}

// -----------------------------------------------------------------------------

ScopeTable::~ScopeTable()
{
    delete rootScope_;

    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        delete iter->second;
}

inline void ScopeTable::insert(PseudoReg* reg)
{
    swiftAssert(reg->id_, "no id in this reg");

    pair<Scope::RegMap::iterator, bool> p
        = currentScope()->regs_.insert( std::make_pair(reg->id_, reg) );

    swiftAssert(p.second, "there is already a reg with this id in the map");
}

Function* ScopeTable::insertFunction(std::string* id)
{
    Function* function = new Function( 0, new std::string(*id) );
    functions_.insert( std::make_pair(id, function) );

    return function;
}

PseudoReg* ScopeTable::newTemp(PseudoReg::RegType regType)
{
    static long counter = 0;

    ostringstream oss;
    /*
        '?' is a magic char used to start new temps
        because it is a vaild beginning char for labels in nasm/yasm
    */
    oss << "?" << counter;
    string* str = new string( oss.str() );

    PseudoReg* reg = new PseudoReg(str, regType);
    insert(reg);

    ++counter;

    return reg;
}

PseudoReg* ScopeTable::newRevision(PseudoReg::RegType regType, std::string* id, int revision)
{
    ostringstream oss;
    // '#' is a magic char used to divide the orignal name by the revision number
    oss << *id << '#' << revision;

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
        // '#' is a magic char used to divide the orignal name by the revision number
        oss << *id << '#' << revision;
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

#ifdef SWIFT_DEBUG
        bool isFunction = typeid(*scopeIter) == typeid(Function);
        swiftAssert(!isFunction , "pseudo reg not found");
        if ( isFunction )
            return 0;
#endif // SWIFT_DEBUG

        // not found, so climb up
        scopeIter = scopeIter->parent_;
    }

    return 0;
}

void ScopeTable::dump(const std::string& extension)
{
    ostringstream oss;
    oss << filename_ << extension;

    ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...
    ofs << filename_ << extension << ":" << std::endl << std::endl;

    for (FunctionMap::iterator iter = functions_.begin(); iter != functions_.end(); ++iter)
        iter->second->dump(ofs);

    // finish
    ofs.close();
}
