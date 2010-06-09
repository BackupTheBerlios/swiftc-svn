#ifndef SWIFT_TNLIST_H
#define SWIFT_TNLIST_H

#include <vector>

#include <llvm/Support/IRBuilder.h>

#include "fe/typelist.h"

namespace llvm {
    class Value;
}

namespace swift {

class Context;
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
    bool isInit(size_t i) const;
    bool isAddr(size_t i) const;
    llvm::Value* getValue(size_t i) const;
    llvm::Value* getScalar(size_t i, llvm::IRBuilder<>& builder) const;
    llvm::Value* getAddr(size_t i, Context* ctxt) const;
    const TypeList& typeList() const;
    size_t numItems() const;
    size_t numRetValues() const;

private:

    typedef std::vector<TypeNode*> TypeNodeVec;
    typedef std::vector<llvm::Value*> ValueVec;

    TypeNodeVec typeNodes_;
    BoolVec lvalues_;
    BoolVec inits_;
    BoolVec addresses_;
    TypeList types_;
    ValueVec values_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TNLIST_H
