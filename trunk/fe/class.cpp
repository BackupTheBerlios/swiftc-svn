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

#include "fe/class.h"

#include <sstream>

#include "utils/assert.h"

#include "fe/context.h"
#include "fe/error.h"
#include "fe/scope.h"
#include "fe/sig.h"
#include "fe/token2str.h"
#include "fe/stmnt.h"
#include "fe/type.h"

namespace swift {

//------------------------------------------------------------------------------

Class::Class(location loc, bool simd, std::string* id)
    : Def(loc, id)
    , simd_(simd)
    , copyCreate_(NOT_ANALYZED)
    , defaultCreate_(NOT_ANALYZED)
    , copyAssign_(NOT_ANALYZED)
    , llvmType_(0)
{}

Class::~Class()
{
    for (size_t i = 0; i < memberVars_.size(); ++i)
        delete memberVars_[i];

    for (size_t i = 0; i < memberFcts_.size(); ++i)
        delete memberFcts_[i];
}

void Class::accept(ClassVisitorBase* c)
{
    // omit builtin types here
    if ( BaseType::isBuiltin(id_) )
        return;

    c->ctxt_->class_ = this;

    for (size_t i = 0; i < memberVars_.size(); ++i)
        memberVars_[i]->accept(c);

    for (size_t i = 0; i < memberFcts_.size(); ++i)
        memberFcts_[i]->accept(c);
}

const std::string* Class::id() const
{
    return id_;
}

const char* Class::cid() const
{
    return id_->c_str();
}

void Class::insert(Context* ctxt, MemberVar* m)
{
    memberVars_.push_back(m);

    MemberVarMap::iterator iter = memberVarMap_.find(m->id());

    if (iter != memberVarMap_.end())
    {
        errorf(m->loc(), "there is already a member variable '%s' defined in class '%s'", m->cid(), cid());
        SWIFT_PREV_ERROR(iter->second->loc());

        ctxt->result_ = false;

        return;
    }

    memberVarMap_[m->id()] = m;

    return;
}

void Class::insert(Context* ctxt, MemberFct* m)
{
    // TODO reader foo(int, int) <-> writer foo(int, int)
    memberFcts_.push_back(m);

    MemberFctMap::iterator iter = memberFctMap_.find(m->id());
    memberFctMap_.insert( std::make_pair(m->id(), m) );

    ctxt->memberFct_ = m;
}

MemberVar* Class::lookupMemberVar(const std::string* id) const
{
    MemberVarMap::const_iterator iter = memberVarMap_.find(id);

    if ( iter == memberVarMap_.end() )
        return 0;

    return iter->second;
}

MemberFct* Class::lookupMemberFct(Module* module, const std::string* id, const TypeList& inTypes) const
{
    typedef MemberFctMap::const_iterator CIter;
    std::pair<CIter, CIter> p = memberFctMap_.equal_range(id);

    for (CIter iter = p.first; iter != p.second; ++iter)
    {
        MemberFct* m = iter->second;

        if ( m->sig_.checkIn(module, inTypes) )
            return m;
    }

    return 0;
}

void Class::addAssignCreate(Context* ctxt)
{
    typedef MemberFctMap::const_iterator CIter;

    // create base type of this class
    BaseType bt( loc(), Token::CONST, new std::string(*id_) );

    // needed for signature check
    TypeList empty;
    TypeList in;
    in.push_back(&bt);

    {
        // is there any user defined copy or default constructor?
        std::string str = "create";
        std::pair<CIter, CIter> p = memberFctMap_.equal_range(&str);

        for (CIter iter = p.first; iter != p.second; ++iter)
        {
            MemberFct* m = iter->second;

            if ( m->sig_.checkIn(ctxt->module_, in) )
            {
                copyCreate_ = USER;

                if (defaultCreate_ != NOT_ANALYZED)
                    break;
            }
            else if ( m->sig_.checkIn(ctxt->module_, empty) )
            {
                defaultCreate_ = USER;

                if (copyCreate_ != NOT_ANALYZED)
                    break;
            }
        }
    }

    {
        // is there any user defined copy assignment?
        std::string str = "assign";
        std::pair<CIter, CIter> p = memberFctMap_.equal_range(&str);

        for (CIter iter = p.first; iter != p.second; ++iter)
        {
            MemberFct* m = iter->second;

            if ( m->sig_.checkIn(ctxt->module_, in) )
            {
                copyAssign_ = USER;
                break;
            }
        }
    }

    if (defaultCreate_ == NOT_ANALYZED)
    {
        // add default create
        Scope* scope = ctxt->enterScope();
        Create* create = new Create(loc_, false, scope);
        ctxt->leaveScope();

        memberFcts_.push_back(create);
        memberFctMap_.insert( std::make_pair(create->id(), create) );

        defaultCreate_ = DEFAULT;
    }

    if (copyCreate_ == NOT_ANALYZED)
    {
        // add copy create
        Scope* scope = ctxt->enterScope();
        Create* create = new Create(loc_, false, scope);
        ctxt->memberFct_ = create;
        create->sig_.in_.push_back( new InOut(loc_, bt.clone(), new std::string("arg")) );
        create->sig_.buildTypeLists();
        ctxt->leaveScope();

        memberFcts_.push_back(create);
        memberFctMap_.insert( std::make_pair(create->id(), create) );

        copyCreate_ = DEFAULT;
    }

    if (copyAssign_ == NOT_ANALYZED)
    {
        // add copy assign
        Scope* scope = ctxt->enterScope();
        Assign* assign = new Assign(loc_, false, Token::ASGN, scope);
        ctxt->memberFct_ = assign;
        assign->sig_.in_.push_back( new InOut(loc_, bt.clone(), new std::string("arg")) );
        assign->sig_.buildTypeLists();
        ctxt->leaveScope();

        memberFcts_.push_back(assign);
        memberFctMap_.insert( std::make_pair(assign->id(), assign) );

        copyAssign_ = DEFAULT;
    }
}

const Class::MemberVars& Class::memberVars() const
{
    return memberVars_;
}

Class::Impl Class::getCopyCreate() const
{
    return copyCreate_;
}

Class::Impl Class::getDefaultCreate() const
{
    return defaultCreate_;
}

Class::Impl Class::getAssign() const
{
    return copyAssign_;
}

llvm::StructType*& Class::llvmType()
{
    return llvmType_;
}

const Class::LLVMStructTypePtr& Class::llvmType() const
{
    return llvmType_;
}

//------------------------------------------------------------------------------

ClassMember::ClassMember(location loc, std::string* id)
    : Node(loc)
    , id_(id)
{}

ClassMember::~ClassMember()
{
    delete id_;
}

const std::string* ClassMember::id() const
{
    return id_;
}


const char* ClassMember::cid() const
{
    return id_->c_str();
}
//------------------------------------------------------------------------------

MemberFct::MemberFct(location loc, bool simd, std::string* id, Scope* scope)
    : ClassMember(loc, id)
    , simd_(simd)
    , scope_(scope)
{}

MemberFct::~MemberFct()
{
    delete scope_;
}

//------------------------------------------------------------------------------

Method::Method(location loc, bool simd, std::string* id, Scope* scope)
    : MemberFct(loc, simd, id, scope)
{}

//------------------------------------------------------------------------------

Reader::Reader(location loc, bool simd, std::string* id, Scope* scope)
    : Method(loc, simd, id, scope )
{}

void Reader::accept(ClassVisitorBase* c)
{
    c->ctxt_->memberFct_ = this;
    c->visit(this);
}

TokenType Reader::getModifier() const
{
    return Token::CONST;
}

const char* Reader::qualifierStr() const
{
    static const char* str = "reader";
    return str;
}

//------------------------------------------------------------------------------

Writer::Writer(location loc, bool simd, std::string* id, Scope* scope)
    : Method(loc, simd, id, scope )
{}

void Writer::accept(ClassVisitorBase* c)
{
    c->ctxt_->memberFct_ = this;
    c->visit(this);
}

TokenType Writer::getModifier() const
{
    return Token::VAR;
}

const char* Writer::qualifierStr() const
{
    static const char* str = "writer";
    return str;
}

//------------------------------------------------------------------------------

Create::Create(location loc, bool simd, Scope* scope)
    : Method( loc, simd, new std::string("create"), scope )
{}

void Create::accept(ClassVisitorBase* c)
{
    c->ctxt_->memberFct_ = this;
    c->visit(this);
}

TokenType Create::getModifier() const
{
    return Token::VAR;
}

const char* Create::qualifierStr() const
{
    static const char* str = "create";
    return str;
}

//------------------------------------------------------------------------------

Assign::Assign(location loc, bool simd, int token, Scope* scope)
    : StaticMethod( loc, simd, token2str(token), scope )
    , token_(token)
{}

void Assign::accept(ClassVisitorBase* c)
{
    c->ctxt_->memberFct_ = this;
    c->visit(this);
}

TokenType Assign::getModifier() const
{
    return Token::VAR;
}

const char* Assign::qualifierStr() const
{
    static const char* str = "assign";
    return str;
}

int Assign::getToken() const
{
    return token_;
}

//------------------------------------------------------------------------------

StaticMethod::StaticMethod(location loc, bool simd, std::string* id, Scope* scope)
    : MemberFct(loc, simd, id, scope)
{}

//------------------------------------------------------------------------------

Routine::Routine(location loc, bool simd, std::string* id, Scope* scope)
    : StaticMethod(loc, simd, id, scope)
{}

void Routine::accept(ClassVisitorBase* c)
{
    c->ctxt_->memberFct_ = this;
    c->visit(this);
}

const char* Routine::qualifierStr() const
{
    static const char* str = "routine";
    return str;
}

//------------------------------------------------------------------------------

Operator::Operator(location loc, bool simd, int token, Scope* scope)
    : StaticMethod(loc, simd, token2str(token), scope)
    , token_(token)
    , numIns_( calcNumIns() )
{}

void Operator::accept(ClassVisitorBase* c)
{
    c->ctxt_->memberFct_ = this;
    c->visit(this);
}

int Operator::getToken() const
{
    return token_;
}

Operator::NumIns Operator::getNumIns() const
{
    return numIns_;
}

Operator::NumIns Operator::calcNumIns() const
{
    switch (token_)
    {
        case Token::SUB:
            return ONE_OR_TWO;
        case Token::INC:
        case Token::DEC:
        case Token::NOT:
        case Token::L_NOT:
            return ONE;
        default:
            return TWO;
    }
}

const char* Operator::qualifierStr() const
{
    static const char* str = "operator";
    return str;
}

//------------------------------------------------------------------------------

MemberVar::MemberVar(location loc, Type* type, std::string* id)
    : ClassMember(loc, id)
    , type_(type)
{}

MemberVar::~MemberVar()
{
    delete type_;
}

void MemberVar::accept(ClassVisitorBase* c)
{
    c->visit(this);
}

const Type* MemberVar::getType() const
{
    return type_;
}

//------------------------------------------------------------------------------

ClassVisitorBase::ClassVisitorBase(Context* ctxt)
    : ctxt_(ctxt)
{}

} // namespace swift
