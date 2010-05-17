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

#ifndef SWIFT_CLASS_H
#define SWIFT_CLASS_H

#include <map>
#include <set>

#include "utils/list.h"

#include "fe/auto.h"
#include "fe/context.h"
#include "fe/location.hh"
#include "fe/var.h"
#include "fe/sig.h"

namespace swift {

class ClassVisitor;
class Scope;
class Type;

//------------------------------------------------------------------------------

class Class : public Def
{
public:

    Class(location loc, bool simd, std::string* id);
    virtual ~Class();

    virtual void accept(ClassVisitor* c);
    const std::string* id() const;
    const char* cid() const;
    void insert(Context& ctxt, MemberVar* m);
    void insert(Context& ctxt, MemberFct* m);
    MemberFct* lookupMemberFct(Module* module, const std::string* id, const TypeList&) const;
    MemberVar* lookupMemberVar(const std::string* id) const;
    void addAssignCreate(Context& ctxt);

    enum Impl
    {
        NONE,
        USER,
        DEFAULT,
        NOT_ANALYZED
    };

    Impl getCopyCreate() const;
    Impl getDefaultCreate() const;
    Impl getAssign() const;


private:

    bool simd_;

    typedef std::vector<MemberVar*> MemberVars;
    typedef std::vector<MemberFct*> MemberFcts;
    typedef std::     map<const std::string*, MemberVar*, StringPtrCmp> MemberVarMap;
    typedef std::multimap<const std::string*, MemberFct*, StringPtrCmp> MemberFctMap;

    MemberVars memberVars_;
    MemberFcts memberFcts_;
    MemberFctMap memberFctMap_;
    MemberVarMap memberVarMap_;         

    Impl copyCreate_;
    Impl defaultCreate_;
    Impl copyAssign_;
};

//------------------------------------------------------------------------------

class ClassMember : public Node
{
public:

    ClassMember(location loc, std::string* id);
    virtual ~ClassMember();

    virtual void accept(ClassVisitor* c) = 0;
    const std::string* id() const;
    const char* cid() const;

protected:

    std::string* id_;
};

//------------------------------------------------------------------------------

class MemberFct : public ClassMember
{
public:

    MemberFct(location loc, bool simd, std::string* id, Scope* scope);
    virtual ~MemberFct();

    virtual const char* qualifierStr() const = 0;

protected:

    bool simd_;

public:

    Scope* scope_;
    Sig sig_;
};

//------------------------------------------------------------------------------

class Method : public MemberFct
{
public:

    Method(location loc, bool simd, std::string* id, Scope* scope);

    virtual TokenType getModifier() const = 0;
};

//------------------------------------------------------------------------------

class Create : public Method
{
public:

    Create(location loc, bool simd, Scope* scope);

    virtual void accept(ClassVisitor* c);
    virtual TokenType getModifier() const;
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class Reader : public Method
{
public:

    Reader(location loc, bool simd, std::string* id, Scope* scope);

    virtual void accept(ClassVisitor* c);
    virtual TokenType getModifier() const;
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class Writer : public Method
{
public:

    Writer(location loc, bool simd, std::string* id, Scope* scope);

    virtual void accept(ClassVisitor* c);
    virtual TokenType getModifier() const;
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class StaticMethod : public MemberFct
{
public:

    StaticMethod(location loc, bool simd, std::string* id, Scope* scope);
};

//------------------------------------------------------------------------------

class Assign : public StaticMethod
{
public:

    Assign(location loc, bool simd, int token, Scope* scope);

    virtual void accept(ClassVisitor* c);
    virtual TokenType getModifier() const;
    virtual const char* qualifierStr() const;
    int getToken() const;

    int token_;
};

//------------------------------------------------------------------------------

class Operator : public StaticMethod
{
public:

    Operator(location loc, bool simd, int token, Scope* scope);

    virtual void accept(ClassVisitor* c);
    virtual const char* qualifierStr() const;
    int getToken() const;

    enum NumIns
    {
        ONE,
        ONE_OR_TWO,
        TWO
    };

    NumIns getNumIns() const;

private:

    NumIns calcNumIns() const;

    int token_;
    NumIns numIns_;
};

//------------------------------------------------------------------------------

class Routine : public StaticMethod
{
public:

    Routine(location loc, bool simd, std::string *id, Scope* scope);

    virtual void accept(ClassVisitor* c);
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class MemberVar : public ClassMember
{
public:

    MemberVar(location loc, Type* type, std::string* id);
    virtual ~MemberVar();

    virtual void accept(ClassVisitor* c);

    Type* type_;
};

//------------------------------------------------------------------------------

class ClassVisitor
{
public:

    ClassVisitor(Context& ctxt);

    virtual void visit(Class* c) = 0;

    // ClassMember -> MemberFct -> Method
    virtual void visit(Create* c) = 0;
    virtual void visit(Reader* r) = 0;
    virtual void visit(Writer* w) = 0;

    // ClassMember -> MemberFct -> StaticMethod
    virtual void visit(Assign* a) = 0;
    virtual void visit(Operator* o) = 0;
    virtual void visit(Routine* r) = 0;

    // ClassMember -> MemberVar
    virtual void visit(MemberVar* m) = 0;

    friend void Class::accept(ClassVisitor* c);
    friend void Create::accept(ClassVisitor* c);
    friend void Reader::accept(ClassVisitor* c);
    friend void Writer::accept(ClassVisitor* c);
    friend void Assign::accept(ClassVisitor* c);
    friend void Operator::accept(ClassVisitor* c);
    friend void Routine::accept(ClassVisitor* c);

protected:

    Context& ctxt_;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_CLASS_H
