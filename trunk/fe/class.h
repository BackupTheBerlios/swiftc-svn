#ifndef SWIFT_CLASS_H
#define SWIFT_CLASS_H

#include <map>
#include <set>

#include "utils/list.h"

#include "fe/module.h"
#include "fe/syntaxtree.h"


// forward declarations
struct Type;
struct LabelInstr;

//------------------------------------------------------------------------------

struct Local : public SymTabEntry
{
    /// it is important that every local gets is own copy of type_
    Type* type_;

    Local(Type* type, std::string* id, int line, Node* parent = 0)
        : SymTabEntry(id, line, parent)
        , type_(type)
    {}
    ~Local();

    std::string toString() const
    {
        return *id_;
    }
};

//------------------------------------------------------------------------------

struct ClassMember : public SymTabEntry
{
    ClassMember* next_;

    ClassMember(std::string* id, int line, Node* parent = 0)
        : SymTabEntry(id, line, parent)
        , next_(0)
    {}
    ~ClassMember()
    {
        delete next_;
    }
    virtual bool analyze() = 0;
};

//------------------------------------------------------------------------------

struct Class : public Definition
{
    typedef std::multimap<std::string*, Method*, StringPtrCmp> MethodMap;
    typedef MethodMap::iterator MethodIter;
    typedef std::map<std::string*, MemberVar*, StringPtrCmp> MemberVarMap;

    ClassMember* classMember_;

    MethodMap methods_;
    MemberVarMap memberVars_;

    Class(std::string* id, int line = NO_LINE, Node* parent = 0)
        : Definition(id, line, parent)
    {}
    ~Class();

    std::string toString() const
    {
        return *id_;
    }
    bool analyze();
};

//------------------------------------------------------------------------------

struct MemberVar : public ClassMember
{
    Type*           type_;

    MemberVar(Type* type, std::string* id, int line = NO_LINE, Node* parent = 0)
        : ClassMember(id, line, parent)
        , type_(type)
    {}
    ~MemberVar();

    std::string toString() const;
    bool analyze();
};

#endif // SWIFT_CLASS_H
