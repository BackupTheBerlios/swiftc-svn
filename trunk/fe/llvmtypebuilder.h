#ifndef SWIFT_LLVM_TYPE_BUILDER_H
#define SWIFT_LLVM_TYPE_BUILDER_H

#include "utils/set.h"

namespace swift {

class Class;
class Module;

class LLVMTypebuilder
{
public:

    LLVMTypebuilder(Module* m);

    bool getResult() const;

private:

    bool process(Class* c);

    typedef Set<Class*> ClassSet;

    Module* module_;
    bool result_;

    ClassSet cycle_;
};

} // namespace swift

#endif // SWIFT_LLVM_TYPE_BUILDER_H
