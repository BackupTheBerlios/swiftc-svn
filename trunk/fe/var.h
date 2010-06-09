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

#ifndef SWIFT_VAR_H
#define SWIFT_VAR_H

#include <string>
#include <vector>

#include "fe/node.h"

namespace llvm {
    class AllocaInst;
    class Value;
}

namespace swift {

class Context;
class Type;

//------------------------------------------------------------------------------

class Var : public Node
{
protected:

    Var(location loc, Type* type, std::string* id);
    virtual ~Var();

public:

    const Type* getType() const;
    const std::string* id() const;
    const char* cid() const;
    llvm::AllocaInst* createEntryAlloca(Context* ctxt);
    virtual llvm::Value* getAddr(Context* ctxt) const = 0;

protected:

    Type* type_;
    std::string* id_;
    llvm::AllocaInst* alloca_;
};

//------------------------------------------------------------------------------

class Local : public Var
{
public:

    Local(location loc, Type* type, std::string* id);
    virtual llvm::Value* getAddr(Context* ctxt) const;

    void setAlloca(llvm::AllocaInst* alloca);
};

//------------------------------------------------------------------------------

class InOut : public Var
{
protected:

    InOut(location loc, Type* type, std::string* id);

public:

    bool validate(Module* module) const;
    virtual llvm::Value* getAddr(Context* ctxt) const;
    virtual const char* kind() const = 0;
};

//------------------------------------------------------------------------------

typedef std::vector<InOut*> IOs;

//------------------------------------------------------------------------------

class Param : public InOut
{
public:

    Param(location loc, Type* type, std::string* id);

    virtual const char* kind() const;
};

//------------------------------------------------------------------------------

class RetVal : public InOut
{
public:

    RetVal(location loc, Type* type, std::string* id);

    virtual const char* kind() const;

private:

    int retIndex_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_VAR_H
