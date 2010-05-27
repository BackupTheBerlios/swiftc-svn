#ifndef SWIFT_CONTEXT_H
#define SWIFT_CONTEXT_H

#include <stack>
#include <vector>

#include <llvm/Support/IRBuilder.h>

namespace llvm {
    class BasicBlock;
    class Function;
}

namespace swift {

class Class;
class MemberFct;
class Module;
class InOut;
class Scope;
class Stmnt;

class Context
{
public:

    Context(Module* module);

    Scope* enterScope();
    void enterScope(Scope* scope);
    void leaveScope();
    size_t scopeDepth() const;
    Scope* scope();

    bool result_;

    Module* module_;
    Class* class_;
    MemberFct* memberFct_;

    bool var_;

    llvm::Function* llvmFct_; ///< Current llvm function.
    llvm::IRBuilder<> builder_;

private:

    typedef std::stack<Scope*> Scopes;
    Scopes scopes_;
};

} // namespace swift

#endif // SWIFT_CONTEXT_H
