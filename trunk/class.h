#ifndef SWIFT_CLASS_H
#define SWIFT_CLASS_H

#include <vector>
#include <map>

#include "syntaxtree.h"

namespace swift {

// forward declarations
struct Type;

//------------------------------------------------------------------------------

struct Local : public SymTabEntry
{
    Type* type_;

    Local(Type* type, std::string* id, int line, Node* parent = 0)
        : SymTabEntry(id, line, parent)
        , type_(type)
    {}

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
    typedef std::map<std::string*, Method*, StringPtrCmp>       MethodMap;
    typedef std::map<std::string*, MemberVar*, StringPtrCmp>    MemberVarMap;

    ClassMember* classMember_;

    MethodMap       methods_;
    MemberVarMap    memberVars_;

    Class(std::string* id, int line = -1, Node* parent = 0)
        : Definition(id, line, parent)
    {}
    ~Class()
    {
        delete id_;
        delete classMember_;
    }

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

    MemberVar(Type* type, std::string* id, int line = -1, Node* parent = 0)
        : ClassMember(id, line, parent)
        , type_(type)
    {}
    ~MemberVar();

    std::string toString() const;
    bool analyze();
};

//------------------------------------------------------------------------------

struct Parameter : public SymTabEntry
{
    int             parameterQualifier_;
    Type*           type_;
    Parameter*      next_;

/*    Parameter()
        : SymTabEntry(-1)
    {}*/
    Parameter(int parameterQualifier, Type* type, std::string* id, int line = -1, Node* parent = 0)
        : SymTabEntry(id, line, parent)
        , parameterQualifier_(parameterQualifier)
        , type_(type)
    {}
    ~Parameter();

    std::string toString() const;
};

//------------------------------------------------------------------------------

struct Method : public ClassMember
{
    typedef std::map<std::string*, Local*, StringPtrCmp> LocalMap;

    int             methodQualifier_;
    Type*           returnType_;

    Statement*      statements_;

    std::vector<Parameter*> params_;
    LocalMap                locals_;

    Method(int methodQualifier, Type* returnType, std::string* id, int line = -1, Node* parent = 0)
        : ClassMember(id, line, parent)
        , methodQualifier_(methodQualifier)
        , returnType_(returnType)
    {
        params_.reserve(10); // should be enough for most methods
    }
    ~Method();

    std::string toString() const;
    bool analyze();
};

} // namespace swift

#endif // SWIFT_CLASS_H
