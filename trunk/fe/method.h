#ifndef SWIFT_METHOD_H
#define SWIFT_METHOD_H

#include <map>
#include <string>

#include "utils/stringhelper.h"

#include "fe/class.h"
#include "fe/syntaxtree.h"
#include "fe/type.h"
#include "fe/statement.h"

#include "me/pseudoreg.h"

// forward declaration
struct Local;
struct LabelInstr;

//------------------------------------------------------------------------------

struct Scope
{
    Scope* parent_;/// 0 if root

    typedef List<Scope*> ScopeList;
    ScopeList childScopes_;

    typedef std::map<std::string*, Local*, StringPtrCmp> LocalMap;
    LocalMap locals_;

    typedef std::map<int, Local*> RegNrMap;
    RegNrMap regNrs_;

    LabelInstr* lastLabel_;

    Scope(Scope* parent)
        : parent_(parent)
        , lastLabel_(0)
    {}
    ~Scope();

    /// Returns the local by the id, of this scope or parent scopes. 0 if nothing was found.
    Local* lookupLocal(std::string* id);
    /// Returns the local by regNr, of this scope or parent scopes. 0 if nothing was found.
    Local* lookupLocal(int);
};

//------------------------------------------------------------------------------

struct Parameter : public SymTabEntry
{
    enum Kind
    {
        ARG,
        RES,
        RES_INOUT
    };

    Kind            kind_;
    Type*           type_;
    Parameter*      next_;

    Parameter(Kind kind, Type* type, std::string* id, int line = NO_LINE, Node* parent = 0)
        : SymTabEntry(id, line, parent)
        , kind_(kind)
        , type_(type)
    {}
    ~Parameter();

    bool operator == (const Parameter& parameter) const;

    std::string toString() const;
};

//------------------------------------------------------------------------------

struct Method : public ClassMember
{
    struct Signature
    {
        typedef List<Parameter*> Params;
        Params params_;

        bool operator == (const Signature& sig) const;
    };

    int methodQualifier_;
    Parameter* returnTypeList_;

    Statement* statements_;

    typedef std::set<Parameter*> Params;
    Params params_;

    Signature signature_;

    Scope* rootScope_;

    Method(int methodQualifier, Parameter* returnTypeList, std::string* id, int line = NO_LINE, Node* parent = 0)
        : ClassMember(id, line, parent)
        , methodQualifier_(methodQualifier)
        , returnTypeList_(returnTypeList)
        , rootScope_( new Scope(0) )
    {}
    ~Method();

    void insertReturnTypesInSymtab();

    void appendParameter(Parameter* parameter);

    std::string toString() const;
    bool analyze();
};

#endif // SWIFT_METHOD_H
