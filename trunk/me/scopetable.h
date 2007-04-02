#ifndef SWIFT_SCOPETABLE_H
#define SWIFT_SCOPETABLE_H

#include <map>
#include <stack>

#include "utils/list.h"
#include "utils/stringptrcmp.h"
#include "pseudoreg.h"
#include "ssa.h"

//------------------------------------------------------------------------------

/**
 * Capsulates an abstract scope. Each Scope has its own set of vars/pseudoregs
 * and can have child scopes. Each scope knows of its parent, its children and
 * its depth in the scope tree.
 */
struct Scope
{
    Scope* parent_; /// 0 if root scope
    size_t depth_;  /// depth of this scope in the scope tree

    typedef List<Scope*> ScopeList;
    ScopeList childScopes_;

    typedef std::map<std::string*, PseudoReg*, StringPtrCmp> RegMap;
    RegMap regs_;

    typedef List<InstrBase*> InstrList;
    InstrList instrList_;

    Scope(Scope* parent)
        : parent_(parent)
        // 0 if root scope, parent_->depth_ + 1 otherwise
        , depth_(parent_ ? parent_->depth_ + 1 : 0)
    {}
};

//------------------------------------------------------------------------------

/**
 * @brief Specialization of a scope
 * Function has in, intout and out goint parameters and, of course, an identifier.
*/
struct Function : public Scope
{
    std::string* id_;

    RegMap in_;
    RegMap inout_;
    RegMap out_;

    Function(Scope* parent, std::string* id)
        : Scope(parent)
        , id_(id)
    {}
};

//------------------------------------------------------------------------------

struct ScopeTable
{
    enum {
        NO_REVISION = -1
    };

    Scope* rootScope_;

    typedef std::map<std::string*, Function*, StringPtrCmp> FunctionMap;
    FunctionMap functions_; // all functions

    std::stack<Scope*> scopeStack_; // keeps account of current scope;

    ScopeTable()
        : rootScope_( new Scope(0) )
    {
        scopeStack_.push(rootScope_);
    }

    Function* insertFunction(std::string* id);

    void enter(Scope* scope)
    {
        scopeStack_.push(scope);
    }

    void leave()
    {
        scopeStack_.pop();
    }

    Scope* currentScope()
    {
        return scopeStack_.top();
    }

    void appendInstr(InstrBase* instr) {
        currentScope()->instrList_.append(instr);
    }

    PseudoReg* newTemp(PseudoReg::RegType regType);
    PseudoReg* newRevision(PseudoReg::RegType regType, std::string* id, int revision);
    PseudoReg* lookupReg(std::string* id, int revision = NO_REVISION);

private:

    void insert(PseudoReg* reg);
};

typedef ScopeTable ScopeTab;
extern ScopeTab scopetab;

#endif // SWIFT_SCOPETABLE_H
