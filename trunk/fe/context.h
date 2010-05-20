#ifndef SWIFT_CONTEXT_H
#define SWIFT_CONTEXT_H

#include <stack>
#include <vector>

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

typedef std::vector<InOut*> IOs;

class Context
{
public:

    Context();

    Scope* enterScope();
    void enterScope(Scope* scope);
    void leaveScope();
    size_t scopeDepth() const;
    Scope* scope();

    bool result_;

    Module* module_;
    Class* class_;
    MemberFct* memberFct_;

    IOs* ios_;
    bool var_;

    llvm::Function* llvmFct_; ///< Current llvm function.
    llvm::BasicBlock* bb_;    ///< Current llvm basic block.

private:

    typedef std::stack<Scope*> Scopes;
    Scopes scopes_;
};

} // namespace swift

#endif // SWIFT_CONTEXT_H
