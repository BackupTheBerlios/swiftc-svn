#ifndef SWIFT_CLASSANALYZER_H
#define SWIFT_CLASSANALYZER_H

#include "fe/class.h"

namespace swift {

class ClassAnalyzer : public ClassVisitor
{
public:

    ClassAnalyzer(Context& ctxt);

    virtual void visit(Class* c);

    // ClassMember -> MemberFct -> Method
    virtual void visit(Assign* a);
    virtual void visit(Create* c);
    virtual void visit(Reader* r);
    virtual void visit(Writer* w);

    // ClassMember -> MemberFct -> StaticMethod
    virtual void visit(Operator* o);
    virtual void visit(Routine* r);

    // ClassMember -> MemberVar
    virtual void visit(MemberVar* m);

    void checkSig(MemberFct* m);
    void checkStmnts(MemberFct* m);
};

} // namespace swift

#endif // SWIFT_CLASSANALYZER_H
