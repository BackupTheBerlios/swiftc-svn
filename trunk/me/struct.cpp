#include "struct.h"

namespace me {

/*
 * globals
 */

int Struct::nameCounter_ = 0;

//------------------------------------------------------------------------------

#ifdef SWIFT_DEBUG

Member::Member(Struct* parent, Struct* _struct, const std::string& id)
    : parent_(parent)
    , nr_(parent->memberNameCounter_++)
    , struct_(_struct)
    , simpleType_(false)
    , id_(id)
{}

Member::Member(Struct* parent, Op::Type type, const std::string& id)
    : parent_(parent)
    , nr_(parent->memberNameCounter_++)
    , type_(type)
    , simpleType_(true)
    , id_(id)
{}

#else // SWIFT_DEBUG

Member::Member(Struct* parent, Struct* _struct)
    : parent_(parent)
    , nr_(parent->memberNameCounter_++)
    , struct_(_struct)
    , simpleType_(false)
{}

Member::Member(Struct* parent, Op::Type type)
    : parent_(parent)
    , nr_(parent->memberNameCounter_++)
    , type_(type)
    , simpleType_(true)
{}

#endif // SWIFT_DEBUG

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

#ifdef SWIFT_DEBUG

Struct::Struct(const std::string& id)
    : nr_(nameCounter_++)
    , memberNameCounter_(0)
    , size_(0)
    , id_(id)
{}

#else // SWIFT_DEBUG

Struct::Struct()
    : nr_(nameCounter_++)
    , memberNameCounter_(0)
    , size_(0)
{}

#endif // SWIFT_DEBUG

Struct::~Struct()
{
    for (MemberMap::iterator iter = members_.begin(); iter != members_.end(); ++iter)
        delete iter->second;
}

/*
 * further methods
 */

#ifdef SWIFT_DEBUG

Member* Struct::append(Op::Type type, const std::string& id)
{
    Member* member = new Member(this, type, id);
    members_[member->nr_] = member;

    return member;
}

Member* Struct::append(Struct* _struct, const std::string& id)
{
    Member* member = new Member(this, _struct, id);
    members_[member->nr_] = member;

    return member;
}

#else // SWIFT_DEBUG

Member* Struct::append(Op::Type type)
{
    Member* member = new Member(this, type);
    members_[member->nr_] = member;

    return member;
}

Member* Struct::append(Struct* _struct)
{
    Member* member = new Member(this, _struct);
    members_[member->nr_] = member;

    return member;
}

#endif // SWIFT_DEBUG

Member* Struct::lookup(int nr)
{
    MemberMap::iterator iter = members_.find(nr);

    if ( iter == members_.end() )
        return 0;
    else
        return iter->second;
}

} // namespace me
