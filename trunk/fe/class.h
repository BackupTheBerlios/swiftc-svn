#ifndef SWIFT_CLASS_H
#define SWIFT_CLASS_H

#include <map>
#include <set>

#include "utils/list.h"

#include "fe/module.h"

namespace swift {

// forward declarations
struct ClassMember;
struct Method;
struct Type;

//------------------------------------------------------------------------------

/**
 * This is the representation of a Class in Swift.
 * It knows of its class members and methods.
 */
struct Class : public Definition
{
    typedef std::multimap<std::string*, Method*, StringPtrCmp> MethodMap;
    typedef MethodMap::iterator MethodIter;
    typedef std::map<std::string*, MemberVar*, StringPtrCmp> MemberVarMap;

    ClassMember* classMember_; ///< Linked list of class members.

    MethodMap methods_; ///< Methods defined in this class.
    MemberVarMap memberVars_; ///< MemberVars defined in this class.

    /**
     * Knows whether the given class defines a constructor,
     * if this is not the case a default constructor must be created artifically
     */
    bool hasCreate_;

/*
    constructor and destructor
*/

    Class(std::string* id, Symbol* parent, int line = NO_LINE);
    virtual ~Class();

/*
    further methods
*/

    /// Is called when a defaul constructor must be created artifically i.e. hasCreate_ == false
    void createDefaultConstructor();
    virtual bool analyze();
};

//------------------------------------------------------------------------------

/**
 * This class represents either a MemberVar or a Method.
*/
struct ClassMember : public Symbol
{
    ClassMember* next_; ///< Linked List of class members.

/*
    constructor and destructor
*/
    ClassMember(std::string* id, Symbol* parent, int line = NO_LINE);
    virtual ~ClassMember();

/*
    further Methods
*/

    virtual bool analyze() = 0;
};

//------------------------------------------------------------------------------

/**
 * This class represents a member variable of a Class.
 */
struct MemberVar : public ClassMember
{
    Type* type_;

/*
    constructor and destructor
*/

    MemberVar(Type* type, std::string* id, Symbol* parent = 0, int line = NO_LINE);
    virtual ~MemberVar();

/*
    further methods
*/

    virtual bool analyze();
};

} // namespace swift

#endif // SWIFT_CLASS_H
