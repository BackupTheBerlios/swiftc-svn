#ifndef SWIFT_TNLIST_H
#define SWIFT_TNLIST_H

#include <memory>
#include <vector>

#include "utils/llvmhelper.h"

namespace llvm {
    class Value;
}

namespace swift {

class Context;
class TNResult;
class TypeNode; 
class TypeNodeVisitorBase;
class TypeList;

//------------------------------------------------------------------------------

class TNList
{
public:

    TNList();
    ~TNList();

    void append(TypeNode* typeNode);
    void accept(TypeNodeVisitorBase* visitor);

    size_t numTypeNodes() const { return typeNodes_.size(); }
    size_t numResults() const   { return indexMap_.size(); }

    void getArgs(LLVMBuilder& builder, Values& args, size_t begin = 0, size_t end = INTPTR_MAX) const;
    llvm::Value* getArg(LLVMBuilder& builder, size_t i) const;

    TypeNode* getTypeNode(size_t i) const { return typeNodes_[i]; }
    const TNResult& getResult(size_t i) const;

    const TypeList& typeList() const;

private:

    TNResult& setResult(size_t i);

    void buildIndexMap();

    mutable TypeList* typeList_;

    typedef std::vector<TypeNode*> TypeNodes;
    TypeNodes typeNodes_;

    typedef std::vector< std::pair<size_t, size_t> > IndexMap;
    IndexMap indexMap_;

    friend class AssignCreate;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_TNLIST_H
