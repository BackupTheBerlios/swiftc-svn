#include "fe/tnlist.h"

#include <algorithm>

#include "fe/context.h"
#include "fe/type.h"
#include "fe/typenodeanalyzer.h"
#include "fe/typenodecodegen.h"

namespace swift {

TNList::~TNList()
{
    for (size_t i = 0; i < typeNodes_.size(); ++i)
        delete typeNodes_[i];
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
        {
            addresses_.push_back( tncg->isAddr(j) );
            values_.push_back( tncg->getValue(j) );
        }
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

bool TNList::isAddr(size_t i) const
{
    return addresses_[i];
}

llvm::Value* TNList::getValue(size_t i) const
{
    return values_[i];
}

llvm::Value* TNList::getAddr(size_t i, Context* ctxt) const
{
    llvm::Value* val = values_[i];

    if ( !isAddr(i) )
    {
        llvm::AllocaInst* alloca = ctxt->createEntryAlloca( 
                val->getType(), val->getName() );
        ctxt->builder_.CreateStore( val, alloca);

        return alloca;
    }
    else
        return val;
}

llvm::Value* TNList::getScalar(size_t i, llvm::IRBuilder<>& builder) const
{
    if ( isAddr(i) )
        return builder.CreateLoad( getValue(i), getValue(i)->getName() );
    else
        return getValue(i);
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

llvm::Value* TNList::getArg(size_t i, Context* ctxt) const
{
    return types_[i]->perRef() ? getAddr(i, ctxt) : getScalar(i, ctxt->builder_);
}

void TNList::getArgs(Values& args, Context* ctxt) const
{
    for (size_t i = 0; i < values_.size(); ++i)
        args.push_back( getArg(i, ctxt) );
}

} // namespace swift
