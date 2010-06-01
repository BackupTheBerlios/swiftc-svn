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
class InOut;
class MemberFct;
class Module;
class Scope;
class Stmnt;
class TNList;

class Context
{
public:

    Context(Module* module);
    ~Context();

    Scope* enterScope();
    void enterScope(Scope* scope);
    void leaveScope();
    size_t scopeDepth() const;
    Scope* scope();
    void newLists();
    void newTuple();
    void newExprList();

    bool result_;

    Module* module_;
    Class* class_;
    MemberFct* memberFct_;
    TNList* tuple_;
    TNList* exprList_;

    llvm::Function* llvmFct_; ///< Current llvm function.
    llvm::IRBuilder<> builder_;

private:

    typedef std::stack<Scope*> Scopes;
    Scopes scopes_;
};

} // namespace swift

#endif // SWIFT_CONTEXT_H
