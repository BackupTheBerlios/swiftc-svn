#include "vec/vectype.h"

#include <iostream>
#include <sstream>

#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Module.h>

#include "utils/assert.h"
#include "utils/cast.h"

#include "utils/llvmhelper.h"

using namespace llvm;

typedef std::vector<const Type*> LLVMTypes;
typedef std::vector<bool> bvector;

namespace {

LLVMTypes getSubTypes(const Type* type)
{
    LLVMTypes result;

    typedef Type::subtype_iterator Iter;
    for (Iter i = type->subtype_begin(), e = type->subtype_end(); i != e; ++i)
        result.push_back(*i);

    return result;
}

const Type* vecTypeRec(Module* module, int simdWidth, const Type* type, int& simdLength);

const Type* vecTypeRec(Module* module, int simdWidth, const Type* type, const bvector& uniforms, int& simdLength)
{
    // calculate the actual simd length
    simdLength = (simdLength == vec::CONTAINS_BOOL) ? simdWidth : simdLength;

    if ( type->isIntegerTy() && type->getPrimitiveSizeInBits() == 1 )
    {
        // -> this is a bool
        int numBits = (simdWidth / simdLength) * 8;
        const IntegerType* intType = IntegerType::get(type->getContext(), numBits);

        return VectorType::get(intType, simdLength);
    }
    else if ( type->isIntegerTy() || type->isFloatTy() || type->isDoubleTy() )
        return VectorType::get(type, simdLength);
    else if ( type->isVoidTy() )
        return createVoid( type->getContext() );

    // vectorize all sub types which are not uniforms
    LLVMTypes sSubTypes = getSubTypes(type);
    size_t numSubTypes = sSubTypes.size();
    swiftAssert(uniforms.size() == numSubTypes, "sizes must match");

    LLVMTypes vSubTypes(numSubTypes);
    for (size_t i = 0; i < numSubTypes; ++i)
    {
        const llvm::Type* subType_i = sSubTypes[i];
        vSubTypes[i] = uniforms[i] 
                     ? subType_i
                     : vecTypeRec(module, simdWidth, subType_i, simdLength);
    }

    if ( const StructType* st = dynamic<StructType>(type) )
    {
        const StructType* vt = StructType::get( st->getContext(), vSubTypes );

        std::ostringstream oss; 
        oss << "simd_" << module->getTypeName(st);
        module->addTypeName( oss.str().c_str(), vt );

        return vt;
    }
    else if ( const FunctionType* ft = dynamic<FunctionType>(type) )
    {
        // vec return type
        const Type* vRetType = vSubTypes.front();

        // vec params
        LLVMTypes vParams( vSubTypes.begin()+1, vSubTypes.end() );
        const FunctionType* vt = FunctionType::get( vRetType, vParams, ft->isVarArg() );

        return vt;
    }

    swiftAssert(numSubTypes == 1, "must exactly have one sub type");
    const Type* vSubType = vSubTypes[0];

    if ( type->isPointerTy() )
        return PointerType::getUnqual(vSubType);
    else if ( const ArrayType* at = dynamic<ArrayType>(type) )
        return ArrayType::get( vSubType, at->getNumElements() );
    else if ( const VectorType* vt = dynamic<VectorType>(type) )
    {
        // merge if vSubType is also a vector type
        if ( const VectorType* vec = dynamic<VectorType>(vSubType) )
        {
            size_t numElems = vec->getNumElements() * simdLength;
            return VectorType::get( vec->getElementType(), numElems );
        }
        else
            return VectorType::get( vSubType, vt->getNumElements() );
    }

    swiftAssert(false, "unreachable");
    return 0;
}

const Type* vecTypeRec(Module* module, int simdWidth, const Type* type, int& simdLength)
{
    // build dummy uniform vector
    bvector uniforms; 
    uniforms.assign( type->getNumContainedTypes(), false );

    return vecTypeRec(module, simdWidth, type, uniforms, simdLength);
}

} // anonymous namespace

namespace vec {

int lengthOf(int simdWidth, const Type* type)
{
    if ( type->isIntegerTy() )
    {
        int size = type->getPrimitiveSizeInBits();
        if (size == 1) // this is a bool
            return 0;
        else
            return  simdWidth / (type->getPrimitiveSizeInBits() / 8);
    }
    else if ( type->isFloatTy() || type->isDoubleTy() )
        return  simdWidth / (type->getPrimitiveSizeInBits() / 8);
    else if (type->isVoidTy() )
        return 0; // let other types decide

    int n = 0;

    LLVMTypes subTypes = getSubTypes(type);
    for (size_t i = 0; i < subTypes.size(); ++i)
    {
        const Type* attr = subTypes[i];
        int tmp = vec::lengthOf(simdWidth, attr);

        if (tmp == -1)
            return -1;
        else if (n == 0)
            n = tmp;
        else if (tmp != 0 && tmp != n)
            return -1;
    }

    return n;
}

const Type* vecType(Module* module, int simdWidth, const Type* type, int& simdLength)
{
    simdLength = lengthOf(simdWidth, type);

    if (simdLength == ERROR)
        return 0;

    return vecTypeRec(module, simdWidth, type, simdLength);
}

const FunctionType* vecFunctionType(Module* module, 
                                    int simdWidth, 
                                    const FunctionType* type, 
                                    const bvector& uniforms, 
                                    int& simdLength)
{
    simdLength = lengthOf(simdWidth, type);

    if (simdLength == ERROR)
        return 0;

    return cast<FunctionType>( vecTypeRec(module, simdWidth, type, uniforms, simdLength) );
}

} // namespace vec
