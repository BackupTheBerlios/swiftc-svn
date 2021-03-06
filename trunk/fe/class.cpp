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

#include <memory>
#include <sstream>

#include "utils/assert.h"
#include "utils/cast.h"

#include "fe/context.h"
#include "fe/error.h"
#include "fe/scope.h"
#include "fe/sig.h"
#include "fe/stmnt.h"
#include "fe/type.h"

using llvm::Value;

namespace swift {

//------------------------------------------------------------------------------

Class::Class(const Location& loc, Module* parent, bool simd, std::string* id)
    : Node(loc, parent)
    , id_(id)
    , simd_(simd)
    , copyCreate_(NOT_ANALYZED)
    , defaultCreate_(NOT_ANALYZED)
    , copyAssign_(NOT_ANALYZED)
    , llvmType_(0)
    , vecType_(0)
    , simdLength_(-1)
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
    if ( ScalarType::isScalar(id_) )
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

bool Class::insert(MemberVar* m)
{
    memberVars_.push_back(m);

    MemberVarMap::iterator iter = memberVarMap_.find(m->id());

    if (iter != memberVarMap_.end())
    {
        errorf(m->loc(), "there is already a member variable '%s' defined in class '%s'", m->cid(), cid());
        SWIFT_PREV_ERROR(iter->second->loc());

        return false;
    }

    memberVarMap_[m->id()] = m;
    return true;
}

bool Class::insert(MemberFct* m)
{
    // TODO reader foo(int, int) <-> writer foo(int, int)
    memberFcts_.push_back(m);

    MemberFctMap::iterator iter = memberFctMap_.find(m->id());
    memberFctMap_.insert( std::make_pair(m->id(), m) );

    return true;
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

//void Class::addAssignCreate(Context* ctxt)
//{
    //typedef MemberFctMap::const_iterator CIter;

    //// create base type of this class
    //std::auto_ptr<BaseType> bt( 
            //BaseType::create( loc(), Token::CONST, new std::string(*id_), true ) );

    //// needed for signature check
    //TypeList empty;
    //TypeList in;
    //in.push_back( bt.get() );

    //{
        //// is there any user defined copy or default constructor?
        //std::string str = "create";
        //std::pair<CIter, CIter> p = memberFctMap_.equal_range(&str);

        //for (CIter iter = p.first; iter != p.second; ++iter)
        //{
            //MemberFct* m = iter->second;

            //if ( m->sig_.checkIn(ctxt->module_, in) )
            //{
                //copyCreate_ = USER;

                //if (defaultCreate_ != NOT_ANALYZED)
                    //break;
            //}
            //else if ( m->sig_.checkIn(ctxt->module_, empty) )
            //{
                //defaultCreate_ = USER;

                //if (copyCreate_ != NOT_ANALYZED)
                    //break;
            //}
        //}
    //}

    //{
        //// is there any user defined copy assignment?
        //std::string str = "=";
        //std::pair<CIter, CIter> p = memberFctMap_.equal_range(&str);

        //for (CIter iter = p.first; iter != p.second; ++iter)
        //{
            //MemberFct* m = iter->second;

            //if ( m->sig_.checkIn(ctxt->module_, in) )
            //{
                //copyAssign_ = USER;
                //break;
            //}
        //}
    //}

    //if (defaultCreate_ == NOT_ANALYZED)
    //{
        //// add default create
        //Scope* scope = ctxt->enterScope();
        //Create* create = new Create(loc_, simd_, scope);
        //ctxt->leaveScope();

        //memberFcts_.push_back(create);
        //memberFctMap_.insert( std::make_pair(create->id(), create) );

        //defaultCreate_ = DEFAULT;
        //create->isAutoDefault_ = true;
    //}

    //if (copyCreate_ == NOT_ANALYZED)
    //{
        //// add copy create
        //Scope* scope = ctxt->enterScope();
        //Create* create = new Create(loc_, simd_, scope);
        //ctxt->memberFct_ = create;
        //create->sig_.in_.push_back( new Param(loc_, bt->clone(), new std::string("arg")) );
        //create->sig_.buildTypeLists();
        //ctxt->leaveScope();

        //memberFcts_.push_back(create);
        //memberFctMap_.insert( std::make_pair(create->id(), create) );

        //copyCreate_ = DEFAULT;
        //create->isAutoCopy_ = true;
    //}

    //if (copyAssign_ == NOT_ANALYZED)
    //{
        //// add copy assign
        //Scope* scope = ctxt->enterScope();
        //Writer* assign = new Writer( loc_, simd_, new std::string("="), scope );
        //ctxt->memberFct_ = assign;
        //assign->sig_.in_.push_back( new Param(loc_, bt->clone(), new std::string("arg")) );
        //assign->sig_.buildTypeLists();
        //ctxt->leaveScope();

        //memberFcts_.push_back(assign);
        //memberFctMap_.insert( std::make_pair(assign->id(), assign) );

        //copyAssign_ = DEFAULT;
        //assign->isAutoAssign_ = true;
    //}
//}

const Class::MemberVars& Class::memberVars() const
{
    return memberVars_;
}

const Class::MemberFcts& Class::memberFcts() const
{
    return memberFcts_;
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

bool Class::isSimd() const
{
    return simd_;
}

const llvm::StructType* Class::getLLVMType() const
{
    return llvmType_;
}

const llvm::StructType* Class::getVecType() const
{
    return vecType_;
}

int Class::getSimdLength() const
{
    return simdLength_;
}

//------------------------------------------------------------------------------

ClassMember::ClassMember(const Location& loc, Class* parent, std::string* id)
    : Node(loc, parent)
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

std::string ClassMember::getLLVMName() const
{
    return llvmName_;
}

void ClassMember::setLLVMName(const std::string& name)
{
    llvmName_ = name;
}

//------------------------------------------------------------------------------

MemberFct::MemberFct(const Location& loc, 
                     Class* parent, 
                     const Qualifiers& qualifiers, 
                     std::string* id)
    : ClassMember(loc, parent, id)
    , scope_(new Scope(loc, this, 0) )
    , main_(false)
    , constructor_(false)
    , thisValue_(0)
{}

MemberFct::MemberFct(const Location& loc, 
                     Class* parent, 
                     const Qualifiers& qualifiers)
    : ClassMember(loc, parent, new std::string("this"))
    , scope_(new Scope(loc, this, 0) )
    , main_(false)
    , constructor_(true)
    , thisValue_(0)
{
    // constructors are always static
    qualifiers_.set(STATIC);
}

MemberFct::~MemberFct()
{
    delete scope_;
}

bool MemberFct::isEmpty() const
{
    return scope_->isEmpty();
}

bool MemberFct::isAutoGenerated() const
{
    // TODO
    return true;
}

void MemberFct::accept(ClassVisitorBase* c)
{
    c->visit(this);
}

bool MemberFct::isSimd() const
{
    return qualifiers_.test(SIMD);
}

bool MemberFct::isStatic() const
{
    return qualifiers_.test(STATIC);
}

bool MemberFct::hasThisArg() const
{
    return constructor_ || !qualifiers_.test(STATIC);
}

TokenType MemberFct::getVarOrConst() const
{
    swiftAssert(hasThisArg(), "must have a this argument");

    if ( qualifiers_.test(CONST) )
        return Token::CONST;
    else
        return Token::VAR;
}

Value* MemberFct::getThisValue() const
{
    return thisValue_;
}

Scope* MemberFct::scope()
{
    return scope_;
}

//----------------------------------------------------------------------

//Method::Method(const Location& loc, bool simd, std::string* id, Scope* scope)
    //: MemberFct(loc, simd, id, scope)
    //, isAutoAssign_(false)
//{}

//bool Method::isAutoGenerated() const
//{
    //return isAutoAssign_;
//}

////------------------------------------------------------------------------------

//Reader::Reader(const Location& loc, bool simd, std::string* id, Scope* scope)
    //: Method(loc, simd, id, scope )
//{}

//void Reader::accept(ClassVisitorBase* c)
//{
    //c->ctxt_->memberFct_ = this;
    //c->visit(this);
//}

//TokenType Reader::getModifier() const
//{
    //return Token::CONST;
//}

//const char* Reader::qualifierStr() const
//{
    //static const char* str = "reader";
    //return str;
//}

////------------------------------------------------------------------------------

//Writer::Writer(const Location& loc, bool simd, std::string* id, Scope* scope)
    //: Method(loc, simd, id, scope )
//{}

//void Writer::accept(ClassVisitorBase* c)
//{
    //c->ctxt_->memberFct_ = this;
    //c->visit(this);
//}

//TokenType Writer::getModifier() const
//{
    //return Token::VAR;
//}

//const char* Writer::qualifierStr() const
//{
    //static const char* str = "writer";
    //return str;
//}

////------------------------------------------------------------------------------

//Create::Create(const Location& loc, bool simd, Scope* scope)
    //: Method( loc, simd, new std::string("create"), scope )
    //, isAutoCopy_(false)
    //, isAutoDefault_(false)
//{}

//bool Create::isAutoGenerated() const
//{
    //return isAutoCopy_ || isAutoDefault_;
//}

//void Create::accept(ClassVisitorBase* c)
//{
    //c->ctxt_->memberFct_ = this;
    //c->visit(this);
//}

//TokenType Create::getModifier() const
//{
    //return Token::VAR;
//}

//const char* Create::qualifierStr() const
//{
    //static const char* str = "create";
    //return str;
//}

//bool Create::isAutoCopy() const 
//{ 
    //return isAutoCopy_;
//}

//bool Create::isAutoDefault() const 
//{ 
    //return isAutoDefault_;
//}

////------------------------------------------------------------------------------

//Routine::Routine(const Location& loc, bool simd, std::string* id, Scope* scope)
    //: MemberFct(loc, simd, id, scope)
//{}

//bool Routine::isAutoGenerated() const
//{
    //return false;
//}

//void Routine::accept(ClassVisitorBase* c)
//{
    //c->ctxt_->memberFct_ = this;
    //c->visit(this);
//}

//const char* Routine::qualifierStr() const
//{
    //static const char* str = "routine";
    //return str;
//}

//------------------------------------------------------------------------------

MemberVar::MemberVar(const Location& loc, Class* parent, Type* type, std::string* id)
    : ClassMember(loc, parent, id)
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

int MemberVar::getIndex() const
{
    return index_;
}

//------------------------------------------------------------------------------

ClassVisitorBase::ClassVisitorBase(Context* ctxt)
    : ctxt_(ctxt)
{}

} // namespace swift
