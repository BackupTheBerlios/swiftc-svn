#ifndef SWIFT_LLVM_FCT_DECLARER_H
#define SWIFT_LLVM_FCT_DECLARER_H

namespace swift {

class Context;
class Class;
class MemberFct;

class LLVMFctDeclarer
{
public:

    LLVMFctDeclarer(Context* ctxt);

private:

    void process(Class* c, MemberFct* m);

    Context* ctxt_;
};

} // namespace swift

#endif // SWIFT_LLVM_FCT_DECLARER_H
