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

class ClassVisitorBase;
class Scope;
class Type;

//------------------------------------------------------------------------------

class Class : public Def
{
public:

    Class(location loc, bool simd, std::string* id);
    virtual ~Class();

    virtual void accept(ClassVisitorBase* c);
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

protected:

    bool simd_;

    typedef std::vector<MemberVar*> MemberVars;
    typedef std::vector<MemberFct*> MemberFcts;
    typedef std::     map<const std::string*, MemberVar*, StringPtrCmp> MemberVarMap;
    typedef std::multimap<const std::string*, MemberFct*, StringPtrCmp> MemberFctMap;

    MemberVars memberVars_;
    MemberFcts memberFcts_;
    MemberFctMap memberFctMap_;
    MemberVarMap memberVarMap_;         

private:

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

    virtual void accept(ClassVisitorBase* c) = 0;
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
    Scope* scope_;

public:

    Sig sig_;

    template<class T> friend class ClassVisitor;
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

    virtual void accept(ClassVisitorBase* c);
    virtual TokenType getModifier() const;
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class Reader : public Method
{
public:

    Reader(location loc, bool simd, std::string* id, Scope* scope);

    virtual void accept(ClassVisitorBase* c);
    virtual TokenType getModifier() const;
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class Writer : public Method
{
public:

    Writer(location loc, bool simd, std::string* id, Scope* scope);

    virtual void accept(ClassVisitorBase* c);
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

    virtual void accept(ClassVisitorBase* c);
    virtual TokenType getModifier() const;
    virtual const char* qualifierStr() const;
    int getToken() const;

protected:

    int token_;

    template<class T> friend class ClassVisitor;
};

//------------------------------------------------------------------------------

class Operator : public StaticMethod
{
public:

    Operator(location loc, bool simd, int token, Scope* scope);

    virtual void accept(ClassVisitorBase* c);
    virtual const char* qualifierStr() const;
    int getToken() const;

    enum NumIns
    {
        ONE,
        ONE_OR_TWO,
        TWO
    };

    NumIns getNumIns() const;

protected:

    NumIns calcNumIns() const;

    int token_;
    NumIns numIns_;

    template<class T> friend class ClassVisitor;
};

//------------------------------------------------------------------------------

class Routine : public StaticMethod
{
public:

    Routine(location loc, bool simd, std::string *id, Scope* scope);

    virtual void accept(ClassVisitorBase* c);
    virtual const char* qualifierStr() const;
};

//------------------------------------------------------------------------------

class MemberVar : public ClassMember
{
public:

    MemberVar(location loc, Type* type, std::string* id);
    virtual ~MemberVar();

    virtual void accept(ClassVisitorBase* c);
    const Type* getType() const;

protected:

    Type* type_;

    template<class T> friend class ClassVisitor;
};

//------------------------------------------------------------------------------

class ClassVisitorBase
{
public:

    ClassVisitorBase(Context& ctxt);

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

    friend void Class::accept(ClassVisitorBase* c);
    friend void Create::accept(ClassVisitorBase* c);
    friend void Reader::accept(ClassVisitorBase* c);
    friend void Writer::accept(ClassVisitorBase* c);
    friend void Assign::accept(ClassVisitorBase* c);
    friend void Operator::accept(ClassVisitorBase* c);
    friend void Routine::accept(ClassVisitorBase* c);

protected:

    Context& ctxt_;
};

//------------------------------------------------------------------------------

template<class T> class ClassVisitor; 

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_CLASS_H
