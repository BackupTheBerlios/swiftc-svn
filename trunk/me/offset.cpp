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
 * virtual methods
 */

std::pair<Reg*, size_t> CTOffset::getOffset()
{
    return std::make_pair( (Reg*) 0, getCTOffset() );
}

std::pair<const Reg*, size_t> CTOffset::getOffset() const
{
    return std::make_pair( (const Reg*) 0, getCTOffset() );
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

size_t StructOffset::getCTOffset() const
{
    return next_ ?  next_->getCTOffset() + member_->offset_ 
                 : member_->offset_;
}

std::string StructOffset::toString() const
{
    std::ostringstream oss;
    oss << '(';

#ifdef SWIFT_DEBUG
    oss << struct_->id_ << ", " << member_->id_ << ", " << getCTOffset();
#else // SWIFT_DEBUG
    oss << struct_->nr_ << ", " << getCTOffset();
#endif // SWIFT_DEBUG

    oss << ')';
    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * virtual methods
 */

std::pair<Reg*, size_t> RTOffset::getOffset()
{
    return getRTOffset();
}

std::pair<const Reg*, size_t> RTOffset::getOffset() const
{
    return getRTOffset();
}

//------------------------------------------------------------------------------

/*
 * constructor 
 */

RTArrayOffset::RTArrayOffset(Reg* index, Member* member)
    : index_(index) 
    , member_(member)
{}

/*
 * virtual methods
 */

std::pair<Reg*, size_t> RTArrayOffset::getRTOffset() 
{
    return std::make_pair( index_,
            next_ ?  next_->getCTOffset() + member_->offset_ 
                  : member_->offset_ );
}

std::pair<const Reg*, size_t> RTArrayOffset::getRTOffset() const
{
    return std::make_pair( index_,
            next_ ? next_->getCTOffset() + member_->offset_ 
                  : member_->offset_ );
}

std::string RTArrayOffset::toString() const
{
    std::ostringstream oss;
    oss << '(';

    // TODO
#ifdef SWIFT_DEBUG
    //oss << member_->id_ << "[" << getRTOffset()->toString();
#else // SWIFT_DEBUG
    //oss << member_->nr_ << "[" << getRTOffset()->toString();
#endif // SWIFT_DEBUG

    oss << "])";
    return oss.str();
}

} // namespace me
