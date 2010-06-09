#ifndef SWIFT_CONTEXT_H
#define SWIFT_CONTEXT_H

#include <stack>
#include <vector>

#include <llvm/Support/IRBuilder.h>

namespace llvm {
    class BasicBlock;
    class Function;
    class Type;
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
    void newTuple();
    void pushExprList();
    TNList* popExprList();
    TNList* topExprList() const;

    bool result_;

    Module* module_;
    Class* class_;
    MemberFct* memberFct_;
    TNList* tuple_;

    llvm::Function* llvmFct_; ///< Current llvm function.
    llvm::IRBuilder<> builder_;

    llvm::AllocaInst* createEntryAlloca(
            const llvm::Type* llvmType, 
            const llvm::Twine& name = "") const;

private:

    typedef std::stack<Scope*> Scopes;
    Scopes scopes_;

    typedef std::stack<TNList*> TNLists;
    TNLists exprLists_;
};

} // namespace swift

#endif // SWIFT_CONTEXT_H
