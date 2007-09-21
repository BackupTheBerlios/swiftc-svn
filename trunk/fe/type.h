#ifndef SWIFT_TYPE_H
#define SWIFT_TYPE_H

#include "fe/syntaxtree.h"

#include "me/pseudoreg.h"

//------------------------------------------------------------------------------

struct BaseType : public Node
{
    std::string* id_;
    bool builtin_;

    BaseType(std::string* id, int line = NO_LINE)
        : Node(line)
        , id_(id)
        , builtin_(false)
    {}
    ~BaseType()
    {
        if (id_)
            delete id_;
    }

    PseudoReg::RegType toRegType() const;
    BaseType* clone() const
    {
        return new BaseType( new std::string(*id_), NO_LINE);
    }

    std::string toString() const
    {
        return *id_;
    }
};

//------------------------------------------------------------------------------

struct Type : public Node
{
    BaseType*   baseType_;
    int         pointerCount_;

    Type(BaseType* baseType, int pointerCount, int line = NO_LINE)
        : Node(line)
        , baseType_(baseType)
        , pointerCount_(pointerCount)
    {}
    ~Type()
    {
        delete baseType_;
    }

    /// Creates a copy of this Type
    Type* clone() const
    {
        return new Type(baseType_->clone(), pointerCount_, NO_LINE);
    }

    std::string toString() const;
    bool analyze();

    /**
     * Check Type \p t1 and Type \p t2 for consistency
     *
     * @param t1 first type to be checked
     * @param t2 second type to be checked
    */
    static bool check(Type* t1, Type* t2);

    /// Checks whether a given type exists
    bool validate();

    /// Checks whether this Type is the builtin bool Type
    bool isBool();
};

//------------------------------------------------------------------------------

#endif // SWIFT_TYPE_H
