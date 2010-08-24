#ifndef VEC_TYPE_VECTORIZER_H
#define VEC_TYPE_VECTORIZER_H

#include <vector>

namespace llvm {
    class FunctionType;
    class Module;
    class Type;
}

namespace vec {

enum 
{
    ERROR = -1,
    JUST_VOID = -2,
    CONTAINS_BOOL = 0
};

int lengthOf(int simdWidth, const llvm::Type* type);

const llvm::Type* vecType(llvm::Module* module, 
                          int simdWidth, 
                          const llvm::Type* type, 
                          int& simdLength);

const llvm::FunctionType* vecFunctionType(llvm::Module* module, 
                                          int simdWidth, 
                                          const llvm::FunctionType* ft, 
                                          const std::vector<bool>& uniforms, 
                                          int& simdLength);
} // namespace vec

#endif // VEC_TYPE_VECTORIZER_H
