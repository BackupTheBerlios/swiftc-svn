#include "vec/typevectorizer.h"

#include <iostream>
#include <sstream>

#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Module.h>

#include "utils/assert.h"
#include "utils/cast.h"

using namespace llvm;

namespace vec {

TypeVectorizer::TypeVectorizer(const ErrorHandler* errorHandler, 
                               const VecStructs& vecStructs, 
                               llvm::Module* module)
    : errorHandler_(errorHandler)
    , vecStructs_(vecStructs)
    , module_(module)
    , simdWidth_(16)
{
    for (VecStructs::const_iterator iter = vecStructs_.begin(); iter != vecStructs_.end(); ++iter)
    {
        const StructType* st = iter->first;

        int n = lengthOf(st);

        if (n == -1)
            errorHandler_->notVectorizable(st);
        else
            vecStructType(st, n);
    }
}

int TypeVectorizer::lengthOfScalar(const Type* type, int simdWidth)
{
    swiftAssert( type->isSized(), "must be a sized type" );

    int size;

    if ( type->isIntegerTy() )
    {
        size = type->getPrimitiveSizeInBits();
        if (size == 1) // this is a bool
            return 0;
        else
            return  simdWidth / (type->getPrimitiveSizeInBits() / 8);
    }
    else if ( type->isFloatTy() || type->isDoubleTy() || type->isVectorTy() )
        return  simdWidth / (type->getPrimitiveSizeInBits() / 8);
    else if ( type->isPointerTy() )
    {
        swiftAssert(false, "ignore atm");
        return simdWidth / 8;
    }

    // else
    swiftAssert(false, "unreachable");
    return -1;
}

int TypeVectorizer::lengthOfScalar(const Type* type)
{
    return lengthOfScalar(type, simdWidth_);
}
    
int TypeVectorizer::lengthOfStruct(const StructType* st)
{
    if ( !vecStructs_.contains(st) )
    {
        errorHandler_->notInMap(st, parent_);
        return -1;
    }

    int simdLength = 0;
    typedef StructType::element_iterator EIter;
    for (EIter iter = st->element_begin(); iter != st->element_end(); ++iter)
    {
        const Type* attr = iter->get();
        parent_ = st;
        int tmp = lengthOf(attr);

        if (tmp == -1)
            return -1;
        else if (simdLength == 0)
            simdLength = tmp;
        else if (tmp != 0 && tmp != simdLength)
            return -1;
    }

    return simdLength;
}

int TypeVectorizer::lengthOf(const Type* type)
{
    Lengths::iterator iter = lengths_.find(type);
    if ( iter != lengths_.end() )
        return iter->second; // already calculated

    int simdLength;
    if ( const StructType* st = dynamic_cast<const StructType*>(type) )
        simdLength = lengthOfStruct(st);
    else
        simdLength = lengthOfScalar(type, simdWidth_);

    lengths_[type] = simdLength;

    return simdLength;
}

const Type* TypeVectorizer::vecScalarType(const Type* type, int n, int simdWidth)
{
    int simdLength = n == 0 ? simdWidth : n;

    if ( type->isIntegerTy() && type->getPrimitiveSizeInBits() == 1 )
    {
        // -> this is a bool
        int numBits = (simdWidth / simdLength) * 8;
        const IntegerType* intType = IntegerType::get(type->getContext(), numBits);

        return VectorType::get(intType, simdLength);
    }
    else if ( type->isIntegerTy() || type->isFloatTy() || type->isDoubleTy() )
        return VectorType::get(type, simdLength);
    else if ( type->isPointerTy() )
    {
        swiftAssert(false, "ignore atm");
        return 0;
    }
    else if ( type->isVectorTy() )
    {
        swiftAssert(false, "TODO");
        return 0;
    }

    swiftAssert(false, "unreachable");
    return 0;
}

const Type* TypeVectorizer::vecScalarType(const Type* type, int n)
{
    return vecScalarType(type, n, simdWidth_);
}

const StructType* TypeVectorizer::vecStructType(const StructType* st, int n)
{
    swiftAssert( vecStructs_.contains(st), "must contain st" );

    StructMap::iterator iter = structMap_.find(st);
    if ( iter != structMap_.end() )
        return iter->second;

    int simdLength = n == 0 ? simdWidth_ : n;

    typedef std::vector<const Type*> LLVMTypes;
    LLVMTypes vecTypes;

    typedef StructType::element_iterator EIter;
    for (EIter iter = st->element_begin(); iter != st->element_end(); ++iter)
        vecTypes.push_back( vecType( iter->get(), simdLength ) );

    const StructType* vecStruct = StructType::get( st->getContext(), vecTypes );
    structMap_[st] = vecStruct;

    std::ostringstream oss; 
    oss << "simd." << module_->getTypeName(st);

    module_->addTypeName( oss.str().c_str(), vecStruct );

    return vecStruct;
}

const Type* TypeVectorizer::vecType(const Type* type, int n)
{
    if ( const StructType* st = dynamic_cast<const StructType*>(type) )
        return vecStructType(st, n);
    else
        return vecScalarType(type, n);
}

} // namespace vec
