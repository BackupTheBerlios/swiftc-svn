#ifndef SWIFT_STMNTANALYZER_H
#define SWIFT_STMNTANALYZER_H

#include "fe/stmnt.h"

namespace swift {

class StmntAnalyzer : public StmntVisitor
{
public:

    StmntAnalyzer(Context& ctxt);

    virtual void visit(CFStmnt* s);
    virtual void visit(DeclStmnt* s);
    virtual void visit(IfElStmnt* s);
    virtual void visit(RepeatUntilStmnt* s);
    virtual void visit(ScopeStmnt* s);
    virtual void visit(WhileStmnt* s);

    // Stmnt -> ActionStmnt
    virtual void visit(AssignStmnt* s);
    virtual void visit(ExprStmnt* s);
};

} // namespace swift

#endif // SWIFT_STMNTANALYZER_H
