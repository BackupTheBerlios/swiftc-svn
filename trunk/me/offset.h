#ifndef ME_OFFSET_H
#define ME_OFFSET_H

#include <string>
#include <utility>

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
     * virtual methods
     */

    virtual std::pair<Reg*, size_t> getOffset() = 0;
    virtual std::pair<const Reg*, size_t> getOffset() const = 0;

    virtual std::string toString() const = 0;
};

//------------------------------------------------------------------------------

class CTOffset : public Offset
{
public:

    /*
     * virtual methods
     */

    virtual std::pair<Reg*, size_t> getOffset();
    virtual std::pair<const Reg*, size_t> getOffset() const;

    virtual size_t getCTOffset() const = 0;
};

//------------------------------------------------------------------------------

class CTArrayOffset : public CTOffset
{
public:

    /*
     * constructor 
     */

    CTArrayOffset(size_t index, Member* member);

    /*
     * further methods
     */

    virtual size_t getCTOffset() const;

private:

    /*
     * data
     */
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

    virtual size_t getCTOffset() const;
    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

class RTOffset : public Offset
{
public:

    /*
     * virtual methods
     */

    virtual std::pair<Reg*, size_t> getOffset();
    virtual std::pair<const Reg*, size_t> getOffset() const;

    virtual std::pair<Reg*, size_t> getRTOffset() = 0;
    virtual std::pair<const Reg*, size_t> getRTOffset() const = 0;
};

//------------------------------------------------------------------------------

struct RTArrayOffset : public RTOffset
{
    Reg* index_;
    Member* member_;

    RTArrayOffset(Reg* index, Member* member);

    virtual std::pair<Reg*, size_t> getRTOffset();
    virtual std::pair<const Reg*, size_t> getRTOffset() const;

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

} // namespace me

#endif // ME_OFFSET_H
