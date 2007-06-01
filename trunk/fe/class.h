#ifndef SWIFT_CLASS_H
#define SWIFT_CLASS_H

#include <vector>
#include <map>

#include "utils/list.h"

#include "fe/module.h"
#include "fe/syntaxtree.h"


// forward declarations
struct Type;

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
    typedef std::map<std::string*, Method*, StringPtrCmp> MethodMap;
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

//------------------------------------------------------------------------------

struct Parameter : public SymTabEntry
{
    int             parameterQualifier_;
    Type*           type_;
    Parameter*      next_;

    Parameter(int parameterQualifier, Type* type, std::string* id, int line = NO_LINE, Node* parent = 0)
        : SymTabEntry(id, line, parent)
        , parameterQualifier_(parameterQualifier)
        , type_(type)
    {}
    ~Parameter();

    std::string toString() const;
};

//------------------------------------------------------------------------------

struct SwiftScope
{
    SwiftScope* parent_;/// 0 if root

    typedef List<SwiftScope*> ScopeList;
    ScopeList childScopes_;

    typedef std::map<std::string*, Local*, StringPtrCmp> LocalMap;
    LocalMap locals_;

    typedef std::map<int, Local*> RegNrMap;
    RegNrMap regNrs_;

    SwiftScope(SwiftScope* parent)
        : parent_(parent)
    {}
    ~SwiftScope();

    /// Returns the local by the id, of this scope or parent scopes. 0 if nothing was found.
    Local* lookupLocal(std::string* id);
    /// Returns the local by regNr, of this scope or parent scopes. 0 if nothing was found.
    Local* lookupLocal(int);
    void replaceRegNr(int oldNr, int newNr);
};

//------------------------------------------------------------------------------

struct Method : public ClassMember
{
    int methodQualifier_;
    Type* returnType_;

    Statement* statements_;

    std::vector<Parameter*> params_;

    SwiftScope* rootScope_;

    Method(int methodQualifier, Type* returnType, std::string* id, int line = NO_LINE, Node* parent = 0)
        : ClassMember(id, line, parent)
        , methodQualifier_(methodQualifier)
        , returnType_(returnType)
        , rootScope_( new SwiftScope(0) )
    {
        // should be enough for most methods
        params_.reserve(10);
    }
    ~Method();

    std::string toString() const;
    bool analyze();
};

#endif // SWIFT_CLASS_H
