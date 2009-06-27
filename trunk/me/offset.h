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
    /// Only a root Offset instance is allowed to be  a RTOffset instance.
    Offset* next_;

    /*
     * constructor and destructor
     */

    Offset();
    virtual ~Offset();

    /*
     * virtual methods
     */

    virtual size_t getOffset() const = 0;
    virtual std::string toString() const = 0;
};

//------------------------------------------------------------------------------

class ArrayOffset : public Offset
{
public:

    /*
     * constructor 
     */

    ArrayOffset(size_t index);

    /*
     * further methods
     */

    virtual size_t getOffset() const;
    virtual std::string toString() const;

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
struct StructOffset : public Offset
{
    Struct* struct_;
    Member* member_;

    StructOffset(Struct* _struct, Member* member);

    virtual size_t getOffset() const;
    virtual std::string toString() const;
};

//------------------------------------------------------------------------------

} // namespace me

#endif // ME_OFFSET_H
