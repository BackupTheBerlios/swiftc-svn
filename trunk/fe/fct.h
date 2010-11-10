#ifndef SWIFT_ASSIGN_CREATE_H
#define SWIFT_ASSIGN_CREATE_H

#include "utils/types.h"

#include <string>

#include "fe/location.h"
#include "fe/type.h"

namespace swift {

class Context;
class TNList;
class TypeList;
class TypeNode;
class Type;

class AssignCreate
{
public:

    AssignCreate() {}
    AssignCreate(Context* ctxt,
                 const Location& loc, const std::string* id,
                 TypeNode* lhs, TNList* rhs,
                 size_t rBegin = 0, size_t rEnd = INTPTR_MAX);
         
    void check();
    void genCode();

    const MemberFctInfo& info() const { return info_; }

    bool initsRhs() const { return initsRhs_; }

private:

    bool isPairwise() const;

    Context* ctxt_;
    Location loc_;
    const std::string* id_;
    TypeNode* lhs_;
    TNList* rhs_;
    size_t rBegin_;
    size_t rEnd_;
    const Type* lType_;
    TypeList rTypes_;
    bool isDecl_;
    bool initsRhs_;

    MemberFctInfo info_;
};

} // namespace swift

#endif // SWIFT_FCT_H
