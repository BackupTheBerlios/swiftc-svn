#ifndef VEC_TYPE_VECTORIZER_H
#define VEC_TYPE_VECTORIZER_H

#include <map>
#include <stack>

#include "utils/map.h"
#include "utils/types.h"

namespace llvm {
    class Module;
    class StructType;
    class Type;
}

namespace vec {

typedef Map<const llvm::StructType*, const llvm::StructType*> VecStructs;

class ErrorHandler
{
public:

    virtual void notInMap(const llvm::StructType* st, const llvm::StructType* parent) const = 0;
    virtual void notVectorizable(const llvm::StructType* st) const = 0;
};

class TypeVectorizer
{
public:

    TypeVectorizer(const ErrorHandler* errorHandler, 
                   const VecStructs& vecStructs, 
                   llvm::Module*);

    static int lengthOfScalar(const llvm::Type* type, int simdWidth);
           int lengthOfScalar(const llvm::Type* type);
           int lengthOfStruct(const llvm::StructType* st);
           int lengthOf(const llvm::Type* type);

    static const llvm::Type* vecScalarType(const llvm::Type* type, int n, int simdWidth);
           const llvm::Type* vecScalarType(const llvm::Type* type, int n);
           const llvm::StructType* vecStructType(const llvm::StructType* st, int n);
           const llvm::Type* vecType(const llvm::Type* type, int n);

private:

    const ErrorHandler* errorHandler_;
    const VecStructs& vecStructs_;
    llvm::Module* module_;
    int simdWidth_;

    typedef std::map<const llvm::Type*, int> Lengths;
    Lengths lengths_;

    typedef std::map<const llvm::StructType*, const llvm::StructType*> StructMap;
    StructMap structMap_;

    const llvm::StructType* parent_;
};

} // namespace vec

#endif // VEC_TYPE_VECTORIZER_H
