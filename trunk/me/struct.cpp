#include "struct.h"

namespace me {

/*
 * globals
 */

int Struct::nameCounter_ = 0;

//------------------------------------------------------------------------------

Struct::Member::Member(Struct* str)
    : nr_(nameCounter_++)
    , struct_(str)
    , simpleType_(false)
{}

Struct::Member::Member(Op::Type type)
    : nr_(nameCounter_++)
    , type_(type)
    , simpleType_(true)
{}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Struct::Struct()
    : nr_(nameCounter_++)
{}

Struct::~Struct()
{
    for (MemberMap::iterator iter = members_.begin(); iter != members_.end(); ++iter)
        delete iter->second;
}

/*
 * further methods
 */

void Struct::append(Op::Type type)
{
    Member* member = new Member(type);
    members_[member->nr_] = member;
}

void Struct::append(Struct* str)
{
    Member* member = new Member(str);
    members_[member->nr_] = member;
}

} // namespace me
