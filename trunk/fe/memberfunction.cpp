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

#include "fe/memberfunction.h"

#include <sstream>

#include "fe/error.h"
#include "fe/scope.h"
#include "fe/signature.h"
#include "fe/statement.h"
#include "fe/symtab.h"
#include "fe/type.h"
#include "fe/var.h"

#include "me/functab.h"
#include "me/ssa.h"

namespace swift {

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

MemberFunction::MemberFunction(bool simd, std::string* id, Symbol* parent, int line)
    : ClassMember(id, parent, line)
    , simd_(simd)
    , rootScope_( new Scope(0) )
    , sig_( new Signature() )
{}

MemberFunction::~MemberFunction()
{
    delete statements_;
    delete rootScope_;
    delete sig_;
}

/*
 * virtual methods
 */

bool MemberFunction::analyze()
{
    symtab->enterMemberFunction(this);

    me::functab->insertFunction( new std::string(sig_->getMeId()), isTrivial(), simd_ );

    bool result = true;
    result &= sig_->analyze();

    // insert the first label since every function must start with one
    me::functab->appendInstr( new me::LabelInstr() );

    if ( !specialAnalyze() )
        return false;;

    me::functab->buildFunctionEntry();

    // analyze each statement
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    // insert the last label since every function must end with one
    me::functab->appendInstrNode( me::functab->getFunctionEpilogue() );

    me::functab->buildFunctionExit();

    // insert the last label since every function must end with one
    me::functab->appendInstr( new me::LabelInstr() );

    symtab->leaveMemberFunction();

    return result;
}

bool MemberFunction::specialAnalyze()
{
    /* 
     * append all other hidden args (non atomic return values) 
     * or append regular results if applicable
     */
    for (size_t i = 0; i < sig_->getNumOut(); ++i)
    {
        Param* param = sig_->getOutParam(i);
        me::Var* var = param->getMeVar();

        if ( !param->getType()->isAtomic() )
            me::functab->appendArg(var); // hidden arg
        else
            me::functab->appendRes(var); // regular res
    }

    // append regular arguments
    for (size_t i = 0; i < sig_->getNumIn(); ++i)
    {
        me::Var* param = sig_->getInParam(i)->getMeVar();
        me::functab->appendArg(param); // hidden arg
    }

    return true;
}

/*
 * further methods
 */

bool MemberFunction::isTrivial() const
{
    return !statements_;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Method::Method(bool simd, std::string* id, Symbol* parent, int line)
    : MemberFunction(simd, id, parent, line)
{}

/*
 * virtual methods
 */

bool Method::specialAnalyze()
{
    BaseType* selfType = createSelfType();
    std::string* selfStr = new std::string("self");
    self_ = (me::Reg*) selfType->createVar(selfStr); 
    delete selfType;
    delete selfStr;

    // the self pointer is the first (hidden) argument
    me::functab->appendArg(self_);

    return MemberFunction::specialAnalyze();
}

/*
 * further methods
 */

BaseType* Method::createSelfType() const
{
    return new BaseType( getSelfModifier(), symtab->currentClass() );
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Reader::Reader(bool simd, std::string* id, Symbol* parent, int line)
    : Method(simd, id, parent, line)
{}

/*
 * virtual methods
 */

int Reader::getSelfModifier() const
{
    return Token::CONST_REF;
}

std::string Reader::qualifierString() const
{
    return "reader";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Writer::Writer(bool simd, std::string* id, Symbol* parent, int line)
    : Method(simd, id, parent, line)
{}

/*
 * virtual methods
 */

int Writer::getSelfModifier() const
{
    return Token::REF;
}

std::string Writer::qualifierString() const
{
    return "writer";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Create::Create(bool simd, Symbol* parent, int line /*= NO_LINE*/)
    : Method( simd, new std::string("create"), parent, line )
{}

/*
 * virtual methods
 */

int Create::getSelfModifier() const
{
    return Token::REF;
}

std::string Create::qualifierString() const
{
    return "create";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Assign::Assign(bool simd, Symbol* parent, int line /*= NO_LINE*/)
    : Method( simd, new std::string("assign"), parent, line)
{}

/*
 * virtual methods
 */

bool Assign::specialAnalyze()
{
    if ( !Method::specialAnalyze() )
        return false;

    const TypeList&  in = sig_->getIn();
    const TypeList& out = sig_->getOut();

    if ( in.empty() || !out.empty() )
    {
        errorf(line_, "an assignment must have exactly one or more "
                "incoming parameters and no outgoing parameters");

        return false;
    }

    return true;
}

int Assign::getSelfModifier() const
{
    return Token::REF;
}

std::string Assign::qualifierString() const
{
    return "assign";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

StaticMethod::StaticMethod(bool simd, std::string* id, Symbol* parent, int line)
    : MemberFunction(simd, id, parent, line)
{}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Routine::Routine(bool simd, std::string* id, Symbol* parent, int line)
    : StaticMethod(simd, id, parent, line)
{}

/*
 * virtual methods
 */

bool Routine::specialAnalyze()
{
    if ( *id_ == "main" 
            && sig_->getNumIn() == 0 
            && sig_->getNumOut() == 1 
            && sig_->getOutParam(0)->getType()->isInt() ) 
    {
        me::functab->setAsMain();
    }

    return MemberFunction::specialAnalyze();
}

std::string Routine::qualifierString() const
{
    return "routine";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Operator::Operator(bool simd, std::string* id, Symbol* parent, int line)
    : StaticMethod(simd, id, parent, line)
{}

/*
 * virtual methods
 */

bool Operator::specialAnalyze()
{
    const TypeList&  in = sig_->getIn();
    const TypeList& out = sig_->getOut();

    /*
     * check signature
     */
    if ( !in.empty() )
    {
        // check whether the first type matches the type of the current class
        const BaseType* bt = dynamic_cast<const BaseType*>( in[0] );
        if ( !bt || *symtab->class_->id_ != *bt->getId() )
        {
            errorf( line_, "the first parameter of the '%s'-operator "
                    "must be of type %s",
                    id_->c_str(),
                    symtab->class_->id_->c_str() );

            return false;
        }
    }

    // minus needs special handling -> can be unary or binary
    bool unaryMinus = false;

    if (   *id_ == "+"
        || *id_ == "-"
        || *id_ == "*"
        || *id_ == "/"
        || *id_ == "mod"
        || *id_ == "div"
        || *id_ == "=="
        || *id_ == "<>"
        || *id_ == "<"
        || *id_ == ">"
        || *id_ == "<="
        || *id_ == ">="
        || *id_ == "and"
        || *id_ == "or"
        || *id_ == "xor")
    {
        if ( in.size() != 2 || out.size() != 1 )
        {
            if (*id_ == "-")
                unaryMinus = true;
            else
            {
                errorf( line_, "the '%s'-operator must exactly have two "
                        "incoming and one outgoing parameter", 
                        id_->c_str() );

                return false;
            }
        }

    }

    if (*id_ == "not" || unaryMinus)
    {

        if ( in.size() != 1 || out.size() != 1 )
        {
            if (*id_ == "-")
            {
                errorf(line_, "the '-'-operator must either have exactly "
                        "two incoming and one outgoing or one incoming " 
                        "and one outgoing parameter");
            }
            else
            {
                errorf( line_, "the '%s'-operator must exactly have "
                        "one incoming and one outgoing parameter",
                        id_->c_str() );
            }

            return false;
        }
    }

    return MemberFunction::specialAnalyze();
}

std::string Operator::qualifierString() const
{
    return "operator";
}

//------------------------------------------------------------------------------

} // namespace swift
