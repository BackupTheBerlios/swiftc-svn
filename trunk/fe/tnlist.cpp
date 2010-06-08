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
            typeList_.push_back( tn->getType(j) );
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
            values_.push_back( tncg->getLLVMValue(j) );
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

bool TNList::isAddr(size_t i) const
{
    return addresses_[i];
}

llvm::Value* TNList::getLLVMValue(size_t i) const
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
        return builder.CreateLoad( getLLVMValue(i), getLLVMValue(i)->getName() );
    else
        return getLLVMValue(i);
}

const TypeList& TNList::typeList() const
{
    return typeList_;
}

size_t TNList::size() const
{
    return typeNodes_.size();
}

} // namespace swift
