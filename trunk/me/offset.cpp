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

StructOffset::StructOffset(Struct* _struct, Member* member)
    : struct_(_struct) 
    , member_(member)
{}

int StructOffset::getOffset() const
{
    return next_ ? next_->getOffset() + member_->offset_ : member_->offset_;
}

std::string StructOffset::toString() const
{
    std::ostringstream oss;
    oss << '(';

#ifdef SWIFT_DEBUG
    oss << struct_->id_ << ", " << member_->id_ << ", " << getOffset();
#else // SWIFT_DEBUG
    oss << struct_->nr_ << ", " << getOffset();
#endif // SWIFT_DEBUG

    oss << ')';
    return oss.str();
}

} // namespace me
