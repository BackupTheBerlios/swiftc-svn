#include "me/offset.h"

#include <sstream>

#include "me/struct.h"

namespace me {

//------------------------------------------------------------------------------

Offset::Offset()
    : next_(0)
{}

Offset::~Offset()
{
    if (next_)
        delete next_;
}

//------------------------------------------------------------------------------

/*
 * constructor 
 */

ArrayOffset::ArrayOffset(size_t index)
    : index_( index ) 
{}

/*
 * virtual methods
 */

size_t ArrayOffset::getOffset() const
{
    return index_ + (next_ ? next_->getOffset() : 0 ); 
}

std::string ArrayOffset::toString() const
{
    std::ostringstream oss;
    oss << '[' << index_ << ']';

    if (next_) 
        oss << "->" << next_->toString();

    return oss.str();
}


//------------------------------------------------------------------------------

/*
 * constructor 
 */

StructOffset::StructOffset(Struct* _struct, Member* member)
    : struct_(_struct) 
    , member_(member)
{}

/*
 * virtual methods
 */

size_t StructOffset::getOffset() const
{
    return next_ ?  next_->getOffset() + member_->getOffset() 
                 : member_->getOffset();
}

std::string StructOffset::toString() const
{
    std::ostringstream oss;
    oss << struct_->toString() << '.' << member_->toString();

    if (next_) 
        oss << "->" << next_->toString();

    return oss.str();
}

//------------------------------------------------------------------------------

} // namespace me
