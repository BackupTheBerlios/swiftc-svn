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

#include "utils/llvmhelper.h"

#include "fe/type.h"
#include "fe/context.h"

using llvm::Value;

namespace swift {

//------------------------------------------------------------------------------

Var::Var(const Location& loc, Type* type, std::string* id)
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

llvm::AllocaInst* Var::createEntryAlloca(Context* ctxt)
{
    int simdLength = 0;
    if ( type_->isSimd() ) // HACK
        simdLength = 4;
    const llvm::Type* llvmType = type_->getLLVMType(ctxt->module_, simdLength);
    alloca_ = ::createEntryAlloca( ctxt->builder_, llvmType, cid() );

    return alloca_;
}

//------------------------------------------------------------------------------

Local::Local(const Location& loc, Type* type, std::string* id)
    : Var(loc, type, id)
{}

Value* Local::getAddr(LLVMBuilder& /*builder*/) const
{
    return alloca_;
}

void Local::setAlloca(llvm::AllocaInst* alloca)
{
    alloca_ = alloca;
}

//------------------------------------------------------------------------------

InOut::InOut(const Location& loc, Type* type, std::string* id)
    : Var(loc, type, id)
{}

bool InOut::validate(Module* module) const
{
    return type_->validate(module);
}

Value* InOut::getAddr(LLVMBuilder& builder) const
{
    if ( !type_->isRef() )
        return alloca_;
    else
        return builder.CreateLoad( alloca_, cid() );
}

//------------------------------------------------------------------------------

Param::Param(const Location& loc, Type* type, std::string* id)
    : InOut(loc, type, id)
{}

const char* Param::kind() const
{
    return "parameter";
}

//------------------------------------------------------------------------------

RetVal::RetVal(const Location& loc, Type* type, std::string* id)
    : InOut(loc, type, id)
{}

const char* RetVal::kind() const
{
    return "return value";
}

//------------------------------------------------------------------------------

} // namespace swift
