/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "fe/var.h"

#include <llvm/BasicBlock.h>
#include <llvm/Function.h>

#include "fe/type.h"
#include "fe/context.h"

namespace swift {

//------------------------------------------------------------------------------

Var::Var(location loc, Type* type, std::string* id)
    : Node(loc) 
    , type_(type)
    , id_(id)
    , alloca_(0)
{}

Var::~Var()
{
    delete type_;
    delete id_;
}

const Type* Var::getType() const
{
    return type_;
}

const std::string* Var::id() const
{
    return id_;
}

const char* Var::cid() const
{
    return id_->c_str();
}

llvm::Value* Var::getAddr(Context* /*ctxt*/) const
{
    return alloca_;
}

void Var::createEntryAlloca(Context* ctxt)
{
    llvm::BasicBlock* entry = &ctxt->llvmFct_->getEntryBlock();
    llvm::IRBuilder<> tmpBuilder( entry, entry->begin() );

    const llvm::Type* llvmType = type_->getLLVMType(ctxt->module_);
    alloca_ = tmpBuilder.CreateAlloca( llvmType, 0, cid() );
}

//------------------------------------------------------------------------------

Local::Local(location loc, Type* type, std::string* id)
    : Var(loc, type, id)
{}

//------------------------------------------------------------------------------

InOut::InOut(location loc, Type* type, std::string* id)
    : Var(loc, type, id)
{}

bool InOut::validate(Module* module) const
{
    return type_->validate(module);
}

//------------------------------------------------------------------------------

Param::Param(location loc, Type* type, std::string* id)
    : InOut(loc, type, id)
{}

//------------------------------------------------------------------------------

RetVal::RetVal(location loc, Type* type, std::string* id)
    : InOut(loc, type, id)
{}

void RetVal::setAlloca(llvm::AllocaInst* alloca, int retIndex)
{
    alloca_ = alloca;
    retIndex_ = retIndex;
}

llvm::Value* RetVal::getAddr(Context* ctxt) const
{
    return ctxt->builder_.CreateGEP(alloca_, llvm::ConstantInt::get(
                *ctxt->module_->llvmCtxt_, 
                llvm::APInt(32, 0)) );
}

} // namespace swift
