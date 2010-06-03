#ifndef SWIFT_LLVM_TYPE_BUILDER_H
#define SWIFT_LLVM_TYPE_BUILDER_H

#include "utils/set.h"

namespace llvm {
    class OpaqueType;
}

namespace swift {

class Context;
class Class;
class Module;
class Type;

class LLVMTypebuilder
{
public:

    LLVMTypebuilder(Context* ctxt);

    bool getResult() const;

private:

    bool process(Class* c);

    typedef Set<Class*> ClassSet;

    Context* ctxt_;
    bool result_;

    ClassSet cycle_;

    struct Refine
    {
        Refine(llvm::OpaqueType* opaque, const Type* type)
            : opaque_(opaque)
            , type_(type)
        {}

        llvm::OpaqueType* opaque_;
        const Type* type_;
    };

    typedef std::vector<Refine> Refinements;
    Refinements refinements;
};

} // namespace swift

#endif // SWIFT_LLVM_TYPE_BUILDER_H
