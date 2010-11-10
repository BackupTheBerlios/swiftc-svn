#include "fe/tnlist.h"

#include <algorithm>

#include "fe/context.h"
#include "fe/type.h"
#include "fe/typenodeanalyzer.h"
#include "fe/typenodecodegen.h"

using llvm::Value;

namespace swift {


TNList::TNList()
    : typeList_(0)
{}

TNList::~TNList()
{
    delete typeList_;
}

void TNList::append(TypeNode* typeNode)
{
    swiftAssert( indexMap_.empty(), "indexMap_ has already been built" );
    typeNodes_.push_back(typeNode);
}

void TNList::buildIndexMap()
{
    if ( !indexMap_.empty() )
        return; // has already been built or building is not necessary at all

#ifdef SWIFT_DEBUG
    size_t numAllResults = 0;
#endif

    for (size_t first = 0; first < typeNodes_.size(); ++first)
    {
        TypeNode* tn = typeNodes_[first];
        size_t numResults = tn->numResults();

#ifdef SWIFT_DEBUG
        numAllResults += numResults;
#endif

        for (size_t second = 0; second < numResults; ++second)
            indexMap_.push_back( std::make_pair(first, second) );
    }

    swiftAssert(numAllResults == indexMap_.size(), "sizes must match");
}

const TNResult& TNList::getResult(size_t i) const
{
    size_t first  = indexMap_[i].first;
    size_t second = indexMap_[i].second;

    return typeNodes_[first]->get(second);
}

TNResult& TNList::setResult(size_t i)
{
    size_t first  = indexMap_[i].first;
    size_t second = indexMap_[i].second;

    return typeNodes_[first]->set(second);
}

void TNList::accept(TypeNodeVisitorBase* visitor)
{
    for (size_t i = 0; i < typeNodes_.size(); ++i)
        typeNodes_[i]->accept(visitor);

    buildIndexMap();
}

Value* TNList::getArg(LLVMBuilder& builder, size_t i) const
{
    const TNResult& result = getResult(i);
    Place* p = result.place_;

    return result.type_->perRef() ? p->getAddr(builder) : p->getScalar(builder);
}

void TNList::getArgs(LLVMBuilder& builder, Values& args, size_t begin /*= 0*/, size_t end /*= INTPTR_MAX*/) const
{
    end = (end == INTPTR_MAX) ? numResults() : end;

    for (size_t i = begin; i < end; ++i)
        args.push_back( getArg(builder, i) );
}

const TypeList& TNList::typeList() const
{
    if (!typeList_)
    {
        typeList_ = new TypeList();

        for (size_t first = 0; first < typeNodes_.size(); ++first)
        {
            TypeNode* tn = typeNodes_[first];

            for (size_t second = 0; second < tn->numResults(); ++second)
                typeList_->push_back( tn->get(second).type_ );
        }
    }

    return *typeList_;
}

} // namespace swift
