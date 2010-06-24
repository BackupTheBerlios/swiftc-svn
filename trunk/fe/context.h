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

    enum
    {
        SIMD_WIDTH = 16
    };

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

    llvm::Value* createMalloc(llvm::Value* size, const llvm::PointerType* ptrType);
    void createMemCpy(llvm::Value* dst, llvm::Value* src, llvm::Value* size);
    llvm::Function* malloc_;
    llvm::Function* memcpy_;

    llvm::LLVMContext& lctxt();
    llvm::Module* lmodule();

private:

    typedef std::stack<Scope*> Scopes;
    Scopes scopes_;

    typedef std::stack<TNList*> TNLists;
    TNLists exprLists_;
};


#define SWIFT_CONFIRM_ERROR swiftAssert(!ctxt_->result_, "there must already be an error")

} // namespace swift

#endif // SWIFT_CONTEXT_H
