#include "me/offset.h"

#include <sstream>

#include "me/struct.h"

namespace me {

//------------------------------------------------------------------------------

StructOffset::StructOffset(Struct* root, Member* member)
    : root_(root)
    , member_(member)
{}

int StructOffset::getOffset() const
{
    return 99;
}

std::string StructOffset::toString() const
{
    std::ostringstream oss;
    oss << '(';

#ifdef SWIFT_DEBUG
    oss << root_->id_ << ", " << member_->id_ << ", " << getOffset();
#else // SWIFT_DEBUG
    oss << root_->nr_ << ", " << getOffset();
#endif // SWIFT_DEBUG

    oss << ')';
    return oss.str();
}

} // namespace me
