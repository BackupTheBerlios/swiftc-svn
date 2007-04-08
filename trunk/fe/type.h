#ifndef SWIFT_TYPE_H
#define SWIFT_TYPE_H

#include "fe/syntaxtree.h"

#include "me/pseudoreg.h"

// forward declarations
struct Expr;

//------------------------------------------------------------------------------

struct BaseType : public Node
{
    BaseType(int line)
        : Node(line)
    {}

    virtual BaseType* clone() const = 0;
};

//------------------------------------------------------------------------------

struct Type : public Node
{
    int         typeQualifier_;
    BaseType*   baseType_;
    int         pointerCount_;

    Type(int typeQualifier, BaseType* baseType, int pointerCount, int line = NO_LINE)
        : Node(line)
        , typeQualifier_(typeQualifier)
        , baseType_(baseType)
        , pointerCount_(pointerCount)
    {}
    ~Type()
    {
        delete baseType_;
    }

    virtual Type* clone() const
    {
        return new Type(typeQualifier_, baseType_->clone(), pointerCount_, line_);
    }

    std::string toString() const;
    bool analyze();

    static bool check(Type* t1, Type* t2);
    static int fitQualifier(Type* t1, Type* t2);
    bool validate();
    bool isBool();
};

//------------------------------------------------------------------------------

struct SimpleType : public BaseType
{
    int kind_;

    SimpleType(int kind, int line = NO_LINE)
        : BaseType(line)
        , kind_(kind)
    {}

    BaseType* clone() const
    {
        return new SimpleType(kind_, line_);
    }

    PseudoReg::RegType toRegType() const;
    static PseudoReg::RegType int2RegType(int i);

    std::string toString() const;
};

//------------------------------------------------------------------------------

struct Container : public BaseType
{
    int     kind_;
    Type*   type_;
    Expr*   expr_;

    Container(int kind, Type* type, Expr* expr, int line = NO_LINE)
        : BaseType(line)
        , kind_(kind)
        , type_(type)
        , expr_(expr)
    {}
    ~Container();

    std::string getContainerType() const;
    std::string toString() const;

    BaseType* clone() const
    {
        return new Container(kind_, type_, 0, line_); // TODO
    }
};

//------------------------------------------------------------------------------

struct UserType : public BaseType
{
    std::string* id_;

    UserType(std::string* id, int line = NO_LINE)
        : BaseType(line)
        , id_(id)
    {}
    ~UserType()
    {
        delete id_;
    }

    BaseType* clone() const
    {
        return new UserType(id_, line_);
    }

    std::string toString() const
    {
        return *id_;
    }
};

#endif // SWIFT_TYPE_H
