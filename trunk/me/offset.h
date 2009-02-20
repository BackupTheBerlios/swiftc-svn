#ifndef ME_OFFSET_H
#define ME_OFFSET_H

#include <string>

#include "me/op.h"

namespace me {

/*
 * forward declarations
 */
struct Member;

//------------------------------------------------------------------------------

struct Offset
{
    Offset* next_;

    /*
     * constructor and destructor
     */

    Offset();
    virtual ~Offset();

    /*
     * further methods
     */

    virtual int getOffset() const = 0;
    virtual std::string toString() const = 0;
};

//------------------------------------------------------------------------------

struct CTOffset : public Offset
{
};

//------------------------------------------------------------------------------

struct RTOffset : public Offset
{
};

//------------------------------------------------------------------------------

struct SimpleTypeOffset : public CTOffset
{
    /*
     * constructor 
     */

    SimpleTypeOffset(Op::Type type);

    /*
     * further methods
     */

    virtual int getOffset() const;
};

//------------------------------------------------------------------------------

struct CTArrayOffset : public CTOffset
{
    /*
     * constructor 
     */

    CTArrayOffset();

    /*
     * further methods
     */

    virtual int getOffset() const;
};

//------------------------------------------------------------------------------

/** 
 * @brief Points to an arbitrary direct or indirect member of \a root_.
 */
struct StructOffset : public CTOffset
{
    Struct* struct_;
    Member* member_;

    StructOffset(Struct* _struct, Member* member);

    virtual int getOffset() const;
    virtual std::string toString() const;
};

//------------------------------------------------------------------------------



} // namespace me

#endif // ME_OFFSET_H
