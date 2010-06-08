#include "fe/tnlist.h"

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
    size_t size = typeNodes_.size();
    lvalueTNList_.resize(size);
    typeList_  .resize(size);

    for (size_t i = 0; i < size; ++i)
    {
        typeNodes_[i]->accept(tna);
        lvalueTNList_[i] = tna->isLValue();
        typeList_[i] = typeNodes_[i]->getType();
    }
}

void TNList::accept(TypeNodeCodeGen* tncg)
{
    size_t size = typeNodes_.size();
    isAddrTNList_.resize(size);
    llvmValues_.resize(size);

    for (size_t i = 0; i < size; ++i)
    {
        typeNodes_[i]->accept(tncg);
        isAddrTNList_[i] = tncg->isAddr();
        llvmValues_[i] = tncg->getLLVMValue();
    }
}

TypeNode* TNList::getTypeNode(size_t i) const
{
    return typeNodes_[i];
}

bool TNList::isLValue(size_t i) const
{
    return lvalueTNList_[i];
}

bool TNList::isAddr(size_t i) const
{
    return isAddrTNList_[i];
}

llvm::Value* TNList::getLLVMValue(size_t i) const
{
    return llvmValues_[i];
}

llvm::Value* TNList::getAddr(size_t i, Context* ctxt) const
{
    llvm::Value* val = llvmValues_[i];

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
