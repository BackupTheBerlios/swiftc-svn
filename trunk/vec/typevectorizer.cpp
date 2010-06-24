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
                               StructMap& scalar2vec,
                               StructMap& vec2scalar,
                               llvm::Module* module)
    : errorHandler_(errorHandler)
    , scalar2vec_(scalar2vec)
    , vec2scalar_(vec2scalar)
    , module_(module)
    , simdWidth_(16)
{
    for (StructMap::const_iterator iter = scalar2vec.begin(); iter != scalar2vec.end(); ++iter)
    {
        const Struct* st = iter->first;

        int n = lengthOfType(st);

        if (n == -1)
            errorHandler_->notVectorizable(st);
        else
            vecStruct(st, n);
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
    
int TypeVectorizer::lengthOfStruct(const Struct* st)
{
    if ( !scalar2vec_.contains(st) )
    {
        errorHandler_->notInMap(st, parent_);
        return -1;
    }

    int simdLength = 0;
    typedef Struct::element_iterator EIter;
    for (EIter iter = st->element_begin(); iter != st->element_end(); ++iter)
    {
        const Type* attr = iter->get();
        parent_ = st;
        int tmp = lengthOfType(attr);

        if (tmp == -1)
            return -1;
        else if (simdLength == 0)
            simdLength = tmp;
        else if (tmp != 0 && tmp != simdLength)
            return -1;
    }

    return simdLength;
}

int TypeVectorizer::lengthOfType(const Type* type)
{
    Lengths::iterator iter = lengths_.find(type);
    if ( iter != lengths_.end() )
        return iter->second; // already calculated

    int simdLength;
    if ( const Struct* st = dynamic<Struct>(type) )
        simdLength = lengthOfStruct(st);
    else
        simdLength = lengthOfScalar(type, simdWidth_);

    lengths_[type] = simdLength;

    return simdLength;
}

const Type* TypeVectorizer::vecScalar(const Type* type, int& n, int simdWidth)
{
    n = n == 0 ? simdWidth : n;

    if ( type->isIntegerTy() && type->getPrimitiveSizeInBits() == 1 )
    {
        // -> this is a bool
        int numBits = (simdWidth / n) * 8;
        const IntegerType* intType = IntegerType::get(type->getContext(), numBits);

        return VectorType::get(intType, n);
    }
    else if ( type->isIntegerTy() || type->isFloatTy() || type->isDoubleTy() )
        return VectorType::get(type, n);
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

const Type* TypeVectorizer::vecScalar(const Type* type, int& n)
{
    return vecScalar(type, n, simdWidth_);
}

const Struct* TypeVectorizer::vecStruct(const Struct* st, int& n)
{
    StructMap::iterator iter = scalar2vec_.find(st);
    swiftAssert( iter != scalar2vec_.end(), "must contain st" );

    // has this struct already been processed?
    if ( iter->second.struct_ != 0 )
    {
        n = iter->second.simdLength_;
        return iter->second.struct_;
    }

    // calculate the actual simd length
    n = n == 0 ? simdWidth_ : n;

    typedef std::vector<const Type*> LLVMTypes;
    LLVMTypes vecTypes;

    typedef Struct::element_iterator EIter;
    for (EIter iter = st->element_begin(); iter != st->element_end(); ++iter)
        vecTypes.push_back( vecType(iter->get(), n) );

    const Struct* vt = Struct::get( st->getContext(), vecTypes );
    scalar2vec_[st] = StructAndLength(vt, n);
    vec2scalar_[vt] = StructAndLength(st, n);

    std::ostringstream oss; 
    oss << "simd." << module_->getTypeName(st);
    module_->addTypeName( oss.str().c_str(), vt );

    return vt;
}

const Type* TypeVectorizer::vecType(const Type* type, int& n)
{
    if ( const Struct* st = dynamic<Struct>(type) )
        return vecStruct(st, n);
    else
        return vecScalar(type, n);
}

} // namespace vec
