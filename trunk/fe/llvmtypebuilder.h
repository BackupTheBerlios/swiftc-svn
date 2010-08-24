#ifndef SWIFT_LLVM_TYPE_BUILDER_H
#define SWIFT_LLVM_TYPE_BUILDER_H

#include "utils/map.h"
#include "utils/set.h"

#include "vec/typevectorizer.h"

namespace llvm {
    class OpaqueType;
    class StructType;
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

    static const llvm::Type* scalar2vec(const llvm::Type* scalar, int& simdLength);
    static const llvm::Type* vec2scalar(const llvm::Type* vec, int& simdLength);

private:

    bool buildTypeFromClass(Class* c);

    Context* ctxt_;
    bool result_;

    typedef Set<Class*> ClassSet;
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
