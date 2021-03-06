#ifndef SWIFT_STMNTANALYZER_H
#define SWIFT_STMNTANALYZER_H

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
    ~StmntVisitor();

    virtual void visit(ErrorStmnt* s);
    virtual void visit(CFStmnt* s);
    virtual void visit(DeclStmnt* s);
    virtual void visit(IfElStmnt* s);
    virtual void visit(WhileLoop* l);
    virtual void visit(RepeatUntilLoop* l);
    virtual void visit(SimdLoop* l);
    virtual void visit(ScopeStmnt* s);
    virtual void visit(AssignStmnt* s);
    virtual void visit(ExprStmnt* s);

private:

    void checkAssignCreate(const Location& loc, 
                           TypeNode* left, 
                           TNList& right, 
                           const std::string* id, 
                           size_t r_begin, 
                           size_t r_end);

    TypeNodeAnalyzer* tna_;
};

typedef StmntVisitor<class Analyzer> StmntAnalyzer;

} // namespace swift

#endif // SWIFT_STMNTANALYZER_H
