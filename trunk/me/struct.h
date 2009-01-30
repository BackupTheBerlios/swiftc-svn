#ifndef ME_STRUCT_H
#define ME_STRUCT_H

#include <map>
#include <stack>

#include "me/op.h"

namespace me {

struct Struct;

struct Member
{
    Struct* parent_;
    int nr_;
    int size_;
    int offset_;

    union
    {
        Op::Type type_;
        Struct* struct_;
    };

    bool simpleType_;

#ifdef SWIFT_DEBUG
    std::string id_;
#endif // SWIFT_DEBUG

    /*
    * constructors and destructor
    */

#ifdef SWIFT_DEBUG
    Member(Struct* parent, Struct* _struct, const std::string& id);
    Member(Struct* parent, Op::Type type, const std::string& id);
#else // SWIFT_DEBUG
    Member(Struct* parent, Struct* _struct);
    Member(Struct* parent, Op::Type type);
#endif // SWIFT_DEBUG
};

struct Struct
{
    static int nameCounter_;

    int nr_;
    int memberNameCounter_;
    int size_;

    enum
    {
        NOT_ANALYZED = -1
    };

#ifdef SWIFT_DEBUG
    std::string id_;
#endif // SWIFT_DEBUG

    typedef std::map<int, Member*> MemberMap;
    MemberMap members_;

    /*
     * constructor and destructor
     */

#ifdef SWIFT_DEBUG
    Struct(const std::string& id);
#else // SWIFT_DEBUG
    Struct();
#endif // SWIFT_DEBUG

    ~Struct();

    /*
     * further methods
     */

#ifdef SWIFT_DEBUG
    Member* append(Op::Type type, const std::string& id);
    Member* append(Struct* _struct, const std::string& id);
#else // SWIFT_DEBUG
    Member* append(Op::Type type);
    Member* append(Struct* _struct);
#endif // SWIFT_DEBUG

    Member* lookup(int nr);

    void analyze();
};

} // namespace me

#endif // ME_STRUCT_H
