#include "fe/tnlist.h"

#include <algorithm>

#include "fe/context.h"
#include "fe/type.h"
#include "fe/typenodeanalyzer.h"
#include "fe/typenodecodegen.h"

using llvm::Value;

namespace swift {

TNList::~TNList()
{
    for (size_t i = 0; i < typeNodes_.size(); ++i)
        delete typeNodes_[i];
    for (size_t i = 0; i < places_.size(); ++i)
        delete places_[i];
}

void TNList::append(TypeNode* typeNode)
{
    typeNodes_.push_back(typeNode);
}

void TNList::accept(TypeNodeAnalyzer* tna)
{
    for (size_t i = 0; i < typeNodes_.size(); ++i)
    {
        TypeNode* tn = typeNodes_[i];
        tn->accept(tna);

        for (size_t j = 0; j < tn->size(); ++j)
        {
            types_.push_back( tn->getType(j) );
            inits_.push_back( tn->isInit(j) );
            lvalues_.push_back( tna->isLValue(j) );
        }
    }
}

void TNList::accept(TypeNodeCodeGen* tncg)
{
    for (size_t i = 0; i < typeNodes_.size(); ++i)
    {
        TypeNode* tn = typeNodes_[i];
        tn->accept(tncg);

        for (size_t j = 0; j < tn->size(); ++j)
            places_.push_back( tncg->getPlace(j) );
    }
}

TypeNode* TNList::getTypeNode(size_t i) const
{
    return typeNodes_[i];
}

bool TNList::isLValue(size_t i) const
{
    return lvalues_[i];
}

bool TNList::isInit(size_t i) const
{
    return inits_[i];
}

Place* TNList::getPlace(size_t i) const
{
    return places_[i];
}

const TypeList& TNList::typeList() const
{
    return types_;
}

size_t TNList::numItems() const
{
    return typeNodes_.size();
}

size_t TNList::numRetValues() const
{
    return types_.size();
}

Value* TNList::getArg(size_t i, LLVMBuilder& builder) const
{
    Place* p = places_[i];
    return types_[i]->perRef() ? p->getAddr(builder) : p->getScalar(builder);
}

void TNList::getArgs(Values& args, LLVMBuilder& builder) const
{
    for (size_t i = 0; i < places_.size(); ++i)
        args.push_back( getArg(i, builder) );
}

} // namespace swift
