#ifndef SWIFT_FCT_VECTORIZER_H
#define SWIFT_FCT_VECTORIZER_H

#include "fe/stmnt.h"

namespace Packetizer {
    class Packetizer;
}

namespace swift {

class Class;
class Context;
class MemberFct;

//------------------------------------------------------------------------------

template <>
class StmntVisitor<class SimdLoopProcessor> : public StmntVisitorBase
{
public:
    StmntVisitor(Context* ctxt, Packetizer::Packetizer* packetizer);

    virtual void visit(ErrorStmnt* s) {}
    virtual void visit(CFStmnt* s) {}
    virtual void visit(DeclStmnt* s) {}
    virtual void visit(IfElStmnt* s);
    virtual void visit(RepeatUntilLoop* l);
    virtual void visit(WhileLoop* l);
    virtual void visit(SimdLoop* l);
    virtual void visit(ScopeStmnt* s);

    // Stmnt -> ActionStmnt
    virtual void visit(AssignStmnt* s) {}
    virtual void visit(ExprStmnt* s) {}

private:

    Packetizer::Packetizer* packetizer_;

    static void enumBBs(SimdLoop* l, llvm::BasicBlock* bb, std::vector<llvm::BasicBlock*>& bbs);
};

typedef StmntVisitor<class SimdLoopProcessor> StmntSimdLoopProcessor;

//------------------------------------------------------------------------------

class FctVectorizer
{
public:

    FctVectorizer(Context* ctxt);

private:

    void processSimd(Class* c, MemberFct* m);
    void processScalar(Class* c, MemberFct* m);

    Context* ctxt_;
    Packetizer::Packetizer* packetizer_;
};

} // namespace swift

#endif // SWIFT_FCT_VECTORIZER_H
