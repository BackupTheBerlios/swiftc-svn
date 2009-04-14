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

#ifndef SWIFT_MEMBERFUNCTION_H
#define SWIFT_MEMBERFUNCTION_H

#include <map>
#include <string>

#include "fe/class.h"
#include "fe/typelist.h"

namespace swift {

/*
 * forward declarations
 */
class Param;
class Signature;
class Scope;

//------------------------------------------------------------------------------

/**
 * This class represents a Method of a Class. 
 */
class MemberFunction : public ClassMember
{
public:

    /*
     * constructor and destructor
     */

    MemberFunction(std::string* id, Symbol* parent, int line = NO_LINE);
    virtual ~MemberFunction();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual bool specialAnalyze(const TypeList& in, const TypeList& out) = 0;
    virtual std::string qualifierString() const = 0;

    /*
     * further methods
     */

    bool isTrivial() const;

//protected: TODO

    Statement* statements_; ///< The statements_ inside this Proc.
    Scope* rootScope_;      ///< The root Scope where vars of this Proc are stored.
    Signature* sig_;        ///< The signature of this Method.
};

//------------------------------------------------------------------------------

class Method : public MemberFunction
{
public:

    /*
     * constructor
     */

    Method(std::string* id, Symbol* parent, int line = NO_LINE);
};

//------------------------------------------------------------------------------

class Reader : public Method
{
public:

    /*
     * constructor
     */

    Reader(std::string* id, Symbol* parent, int line = NO_LINE);

    /*
     * virtual methods
     */

    virtual bool specialAnalyze(const TypeList& in, const TypeList& out);
    virtual std::string qualifierString() const;
};

//------------------------------------------------------------------------------

class Writer : public Method
{
public:

    /*
     * constructor
     */

    Writer(std::string* id, Symbol* parent, int line = NO_LINE);

    /*
     * virtual methods
     */

    virtual bool specialAnalyze(const TypeList& in, const TypeList& out);
    virtual std::string qualifierString() const;
};

//------------------------------------------------------------------------------

class Create : public Method
{
public:

    /*
     * constructor
     */

    Create(Symbol* parent, int line = NO_LINE);

    /*
     * virtual methods
     */

    virtual bool specialAnalyze(const TypeList& in, const TypeList& out);
    virtual std::string qualifierString() const;
};

//------------------------------------------------------------------------------

class Assign : public Method
{
public:

    /*
     * constructor 
     */

    Assign(Symbol* parent, int line = NO_LINE);

    /*
     * virtual methods
     */

    virtual bool specialAnalyze(const TypeList& in, const TypeList& out);
    virtual std::string qualifierString() const;
};

//------------------------------------------------------------------------------

class StaticMethod : public MemberFunction
{
public:

    /*
     * constructor
     */

    StaticMethod(std::string* id, Symbol* parent, int line = NO_LINE);
};

//------------------------------------------------------------------------------

class Routine : public StaticMethod
{
public:

    /*
     * constructor
     */

    Routine(std::string *id, Symbol* parent, int line = NO_LINE);

    /*
     * virtual methods
     */

    virtual bool specialAnalyze(const TypeList& in, const TypeList& out);
    virtual std::string qualifierString() const;
};

//------------------------------------------------------------------------------

class Operator : public StaticMethod
{
public:

    /*
     * constructor 
     */

    Operator(std::string* id, Symbol* parent, int line = NO_LINE);

    /*
     * virtual methods
     */

    virtual bool specialAnalyze(const TypeList& in, const TypeList& out);
    virtual std::string qualifierString() const;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_MEMBERFUNCTION_H
