#ifndef ME_STRUCT_H
#define ME_STRUCT_H

#include <map>

#include "me/op.h"

namespace me {

struct Struct
{
    static int nameCounter_;

    int nr_;
    int size_;

    struct Member
    {
        int nr_;
        int size_;
        int offset_;

        union
        {
            Op::Type type_;
            Struct* struct_;
        };

        bool simpleType_;

        /*
         * constructors and destructor
         */

        Member(Struct* str);
        Member(Op::Type type);
    };

    typedef std::map<int, Member*> MemberMap;
    MemberMap members_;

    /*
     * constructor and destructor
     */

    Struct();
    ~Struct();

    /*
     * further methods
     */

    void append(Op::Type type);
    void append(Struct* str);
};

} // namespace me

#endif // ME_STRUCT_H
