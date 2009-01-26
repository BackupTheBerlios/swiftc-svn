#ifndef SWIFT_TYPE_H
#define SWIFT_TYPE_H

#include "fe/syntaxtree.h"

#include "me/op.h"

namespace swift {

//------------------------------------------------------------------------------

struct BaseType : public Node
{
    std::string* id_;
    bool builtin_;

    /*
     * constructors and destructor
     */

    BaseType(std::string* id, int line = NO_LINE);
    BaseType(std::string* id, bool builtin);
    ~BaseType();

    /*
     * further methods
     */
    BaseType* clone() const;
    me::Op::Type toMeType() const;

    /**
     * Checks whether this is a builtin type.
     *
     * @return True if this is a builtin type, false otherwise
     */
    bool isBuiltin() const;

    typedef std::map<std::string, me::Op::Type> TypeMap;
    static TypeMap* typeMap_; 
};

//------------------------------------------------------------------------------

struct Type : public Node
{
    BaseType*   baseType_;
    int         pointerCount_;

    /*
     * constructor and destructor
     */

    Type(BaseType* baseType, int pointerCount, int line = NO_LINE);
    virtual ~Type();

    /*
     * further methods
     */

    /// Creates a copy of this Type
    Type* clone() const;

    /**
     * Check Type \p t1 and Type \p t2 for consistency
     *
     * @param t1 first type to be checked
     * @param t2 second type to be checked
    */
    static bool check(Type* t1, Type* t2);

    /// Checks whether a given type exists
    bool validate() const;

    /// Checks whether this Type is the builtin bool Type
    bool isBool() const;

    /**
     * Checks whether this is a builtin type.
     *
     * @return True if this is a built in type, false otherwise
     */
    bool isBuiltin() const;

    std::string toString() const;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TYPE_H
