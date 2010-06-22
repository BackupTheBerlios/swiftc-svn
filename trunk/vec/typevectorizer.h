#ifndef VEC_TYPE_VECTORIZER_H
#define VEC_TYPE_VECTORIZER_H

#include <map>
#include <stack>
#include <utility>

#include "utils/map.h"
#include "utils/types.h"

namespace llvm {
    class Module;
    class StructType;
    class Type;
}

namespace vec {

typedef llvm::Type Type;
typedef llvm::StructType Struct;

struct StructAndLength
{
    const Struct* struct_;
    int simdLength_;

    StructAndLength()
        : struct_(0)
        , simdLength_(0)
    {}
    StructAndLength(const Struct* st, int simdLength) 
        : struct_(st)
        , simdLength_(simdLength)
    {}
};

typedef Map<const Struct*, StructAndLength> StructMap;

class ErrorHandler
{
public:

    virtual void notInMap(const Struct* st, const Struct* parent) const = 0;
    virtual void notVectorizable(const Struct* st) const = 0;
};

class TypeVectorizer
{
public:

    TypeVectorizer(const ErrorHandler* errorHandler, 
                   StructMap& scalar2vec,
                   StructMap& vec2scalar,
                   llvm::Module*);

    static int lengthOfScalar(const Type* type, int simdWidth);
           int lengthOfScalar(const Type* type);
           int lengthOfStruct(const Struct* st);
           int lengthOfType  (const Type* type);

    static const Type*   vecScalar(const Type* type, int& n, int simdWidth);
           const Type*   vecScalar(const Type* type, int& n);
           const Struct* vecStruct(const Struct* st, int& n);
           const Type*   vecType  (const Type* type, int& n);

private:

    const ErrorHandler* errorHandler_;
    StructMap& scalar2vec_;
    StructMap& vec2scalar_;
    llvm::Module* module_;
    int simdWidth_;

    typedef std::map<const Type*, int> Lengths;
    Lengths lengths_;

    const Struct* parent_;
};

} // namespace vec

#endif // VEC_TYPE_VECTORIZER_H
