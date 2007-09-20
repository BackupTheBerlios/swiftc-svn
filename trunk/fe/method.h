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

    Scope(Scope* parent)
        : parent_(parent)
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

    Parameter(Kind kind, Type* type, std::string* id, int line = NO_LINE, Node* parent = 0)
        : SymTabEntry(id, line, parent)
        , kind_(kind)
        , type_(type)
    {}
    ~Parameter();

    /// check whether the type of both Parameter objects fit
    static bool check(const Parameter* param1, const Parameter* param2);

    std::string toString() const;
};

//------------------------------------------------------------------------------

struct Method : public ClassMember
{
    struct Signature
    {
        typedef List<Parameter*> Params;
        Params params_;

        static bool check(const Signature& sig1, const Signature& sig2);
        static bool checkIngoing(const Signature& insig1, const Signature& insig2);
        bool checkIngoing(const Signature& insig) const;
    };

    int methodQualifier_;

    Statement* statements_;

    typedef std::set<Parameter*> Params;
    Params params_;

    Signature signature_;

    Scope* rootScope_;

    Method(int methodQualifier, std::string* id, int line = NO_LINE, Node* parent = 0)
        : ClassMember(id, line, parent)
        , methodQualifier_(methodQualifier)
        , rootScope_( new Scope(0) )
    {}
    ~Method();

    void appendParameter(Parameter* parameter);

    std::string toString() const;
    bool analyze();
};

#endif // SWIFT_METHOD_H
