#ifndef SWIFT_TNLIST_H
#define SWIFT_TNLIST_H

#include <vector>

#include "utils/llvmhelper.h"
#include "utils/llvmplace.h"

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
    void accept(TypeNodeAnalyzer& tna);
    void accept(TypeNodeCodeGen& tncg);

    TypeNode* getTypeNode(size_t i) const;
    bool isLValue(size_t i) const;
    bool isInit(size_t i) const;
    Place* getPlace(size_t i) const;
    const TypeList& typeList() const;
    size_t numItems() const;
    size_t numRetValues() const;
    void getArgs(Values& args, LLVMBuilder& builder) const;
    llvm::Value* getArg(size_t i, LLVMBuilder& builder) const;

private:

    typedef std::vector<TypeNode*> TypeNodeVec;

    TypeNodeVec typeNodes_;
    BoolVec lvalues_;
    BoolVec inits_;
    TypeList types_;
    Places places_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TNLIST_H
