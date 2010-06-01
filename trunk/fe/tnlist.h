#ifndef SWIFT_TNLIST_H
#define SWIFT_TNLIST_H

#include <vector>

#include <llvm/Support/IRBuilder.h>

#include "fe/typelist.h"

namespace llvm {
    class Value;
}

namespace swift {

class TypeNode;
template <class T> class TypeNodeVisitor;
typedef TypeNodeVisitor<class Analyzer> TypeNodeAnalyzer;
typedef TypeNodeVisitor<class  CodeGen> TypeNodeCodeGen;

//------------------------------------------------------------------------------

class TNList
{
public:

    ~TNList();

    void append(TypeNode* typeNode);
    void accept(TypeNodeAnalyzer* tna);
    void accept(TypeNodeCodeGen* tncg);

    TypeNode* getTypeNode(size_t i) const;
    bool isLValue(size_t i) const;
    bool isAddr(size_t i) const;
    llvm::Value* getLLVMValue(size_t i) const;
    llvm::Value* getScalar(size_t i, llvm::IRBuilder<>& builder) const;
    const TypeList& typeList() const;
    size_t size() const;

private:

    typedef std::vector<TypeNode*> TypeNodeVec;
    typedef std::vector<bool> BoolVec;
    typedef std::vector<llvm::Value*> ValueTNList;

    TypeNodeVec typeNodes_;
    BoolVec lvalueTNList_;
    BoolVec isAddrTNList_;
    TypeList typeList_;
    ValueTNList llvmValues_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TNLIST_H
