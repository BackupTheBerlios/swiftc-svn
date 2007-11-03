#ifndef SWIFT_CLASS_H
#define SWIFT_CLASS_H

#include <map>
#include <set>

#include "utils/list.h"

#include "fe/module.h"


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

    std::string* id_;
    ClassMember* classMember_; ///< Linked list of class members.

    MethodMap methods_; ///< Methods defined in this class.
    MemberVarMap memberVars_; ///< MemberVars defined in this class.

/*
    constructor and destructor
*/

    Class(std::string* id, int line = NO_LINE, Node* parent = 0);
    virtual ~Class();

/*
    further methods
*/

    virtual bool analyze();
};

//------------------------------------------------------------------------------

/**
 * This class represents either a MemberVar or a Method.
*/
struct ClassMember : public Node
{
    ClassMember* next_; ///< Linked List of class members.

/*
    constructor and destructor
*/
    ClassMember(int line, Node* parent = 0);
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
    std::string* id_;

/*
    constructor and destructor
*/

    MemberVar(Type* type, std::string* id, int line = NO_LINE, Node* parent = 0);
    virtual ~MemberVar();

/*
    further methods
*/

    virtual bool analyze();
};

#endif // SWIFT_CLASS_H
