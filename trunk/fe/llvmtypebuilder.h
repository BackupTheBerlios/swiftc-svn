#ifndef SWIFT_LLVM_TYPE_BUILDER_H
#define SWIFT_LLVM_TYPE_BUILDER_H

#include "utils/set.h"

#include "vec/typevectorizer.h"

namespace llvm {
    class OpaqueType;
}

namespace swift {

class Context;
class Class;
class Module;
class Type;

class LLVMTypebuilder : public vec::ErrorHandler
{
public:

    LLVMTypebuilder(Context* ctxt);

    bool getResult() const;
    virtual void notInMap(const llvm::StructType* st, const llvm::StructType* parent) const;
    virtual void notVectorizable(const llvm::StructType* st) const;

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

    vec::VecStructs vecStructs_;

    typedef std::map<const llvm::StructType*, Class*> Struct2Class;
    Struct2Class struct2Class_;
};

} // namespace swift

#endif // SWIFT_LLVM_TYPE_BUILDER_H
