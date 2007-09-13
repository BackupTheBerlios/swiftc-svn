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
        delete id_;
    }

    PseudoReg::RegType toRegType() const;
    BaseType* clone() const
    {
        return new BaseType(id_, NO_LINE);
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

    virtual Type* clone() const
    {
        return new Type(baseType_->clone(), pointerCount_, NO_LINE);
    }

    std::string toString() const;
    bool analyze();

    static bool check(Type* t1, Type* t2);
    bool validate();
    bool isBool();
};

//------------------------------------------------------------------------------

#endif // SWIFT_TYPE_H
