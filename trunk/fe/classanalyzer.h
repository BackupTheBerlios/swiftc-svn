#ifndef SWIFT_CLASSANALYZER_H
#define SWIFT_CLASSANALYZER_H

#include <memory>

#include "fe/class.h"

namespace swift {

//------------------------------------------------------------------------------

template <class T> class StmntVisitor;
typedef StmntVisitor<class Analyzer> StmntAnalyzer;

//------------------------------------------------------------------------------

template<>
class ClassVisitor<class Analyzer> : public ClassVisitorBase
{
public:

    ClassVisitor(Context* ctxt);
    virtual ~ClassVisitor();

    virtual void visit(Class* c);

    // ClassMember -> MemberFct -> Method
    virtual void visit(Create* c);
    virtual void visit(Reader* r);
    virtual void visit(Writer* w);

    // ClassMember -> MemberFct -> StaticMethod
    virtual void visit(Assign* a);
    virtual void visit(Operator* o);
    virtual void visit(Routine* r);

    // ClassMember -> MemberVar
    virtual void visit(MemberVar* m);

private:

    void checkSig(MemberFct* m);
    void checkStmnts(MemberFct* m);

    std::auto_ptr<StmntAnalyzer> sa_;
};

typedef ClassVisitor<class Analyzer> ClassAnalyzer;

} // namespace swift

#endif // SWIFT_CLASSANALYZER_H
