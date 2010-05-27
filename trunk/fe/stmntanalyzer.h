#ifndef SWIFT_STMNTANALYZER_H
#define SWIFT_STMNTANALYZER_H

#include <memory>

#include "fe/stmnt.h"

namespace swift {

//------------------------------------------------------------------------------

template <class T> class TypeNodeVisitor;
typedef TypeNodeVisitor<class Analyzer> TypeNodeAnalyzer;

//------------------------------------------------------------------------------

template <>
class StmntVisitor<class Analyzer> : public StmntVisitorBase
{
public:

    StmntVisitor(Context* ctxt);

    virtual void visit(ErrorStmnt* s);
    virtual void visit(CFStmnt* s);
    virtual void visit(DeclStmnt* s);
    virtual void visit(IfElStmnt* s);
    virtual void visit(RepeatUntilStmnt* s);
    virtual void visit(ScopeStmnt* s);
    virtual void visit(WhileStmnt* s);

    // Stmnt -> ActionStmnt
    virtual void visit(AssignStmnt* s);
    virtual void visit(ExprStmnt* s);

private:

    std::auto_ptr<TypeNodeAnalyzer> tna_;
};

typedef StmntVisitor<class Analyzer> StmntAnalyzer;

} // namespace swift

#endif // SWIFT_STMNTANALYZER_H
