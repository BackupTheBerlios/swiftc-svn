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
struct CTOffset;

//------------------------------------------------------------------------------

struct Offset
{
    /// Only a root Offset instance is allowed to be  a RTOffset instance.
    CTOffset* next_;

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

    CTArrayOffset(size_t index);

    /*
     * further methods
     */

    virtual size_t getCTOffset() const;

private:

    /*
     * data
     */

    size_t index_;
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

    RTArrayOffset(Reg* index);

    virtual std::pair<Reg*, size_t> getRTOffset();
    virtual std::pair<const Reg*, size_t> getRTOffset() const;

    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

} // namespace me

#endif // ME_OFFSET_H
