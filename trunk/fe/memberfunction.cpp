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

MemberFunction::MemberFunction(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : ClassMember(id, parent, line)
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

    /*
     * build a function name for the me::functab consisting of the class name,
     * the method name and a counted number to prevent name clashes
     * due to overloading
     */
    std::ostringstream oss;

    oss << *symtab->class_->id_ << '$';

    //if (methodQualifier_ == OPERATOR)
        //oss << "operator";
    //else
        oss << *id_;

    static int counter = 0;

    oss << '$' << counter;
    ++counter;

    me::functab->insertFunction( new std::string(oss.str()), isTrivial() );

    bool result = true;
    result &= sig_->analyze();

    // insert the first label since every function must start with one
    me::functab->appendInstr( new me::LabelInstr() );

    if ( !specialAnalyze() )
        return false;;

    // build function entry
    me::SetParams* setParams = 0;
    
    // for each ingoing param
    for (size_t i = 0; i < sig_->getNumIn(); ++i) 
    {
        Param* param = sig_->getInParam(i);
        me::Var* var = param->getMeVar();

        if (setParams)
            setParams->res_.push_back( me::Res(var, var->varNr_) );
        else
        {
            setParams = new me::SetParams(0);
            setParams->res_.push_back( me::Res(var, var->varNr_) );
        }
    }

    me::AssignInstr* returnValues = 0;

    // for each result
    for (size_t i = 0; i < sig_->getNumOut(); ++i) 
    {
        Param* param = sig_->getOutParam(i);
        me::Var* var = param->getMeVar();

        // TODO default constructor call
        if (returnValues)
            returnValues->res_.push_back( me::Res(var, var->varNr_) );
        else
            returnValues = new me::AssignInstr( '=', var, me::functab->newUndef(var->type_) );
    }

    if (setParams)
        me::functab->appendInstr(setParams);

    if (returnValues)
        me::functab->appendInstr(returnValues);

    // analyze each statement
    for (Statement* iter = statements_; iter != 0; iter = iter->next_)
        result &= iter->analyze();

    // insert the last label since every function must end with one
    me::functab->appendInstrNode( me::functab->getLastLabelNode() );

    // is there at least one result?
    if ( sig_->getNumOut() > 0 )
    {
        // build function exit
        me::SetResults* setResults = new me::SetResults(0); // start with 0 args
        
        for (size_t i = 0; i < sig_->getNumOut(); ++i)
        {
            Param* param = sig_->getOutParam(i);

            me::Var* var = param->getMeVar();
            setResults->arg_.push_back( me::Arg(var) );
        }

        me::functab->appendInstr(setResults);
    }

    // insert the last label since every function must end with one
    me::functab->appendInstr( new me::LabelInstr() );

    symtab->leaveMemberFunction();

    return result;
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

Method::Method(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : MemberFunction(id, parent, line)
{}

bool Method::specialAnalyze()
{
    // the self pointer is the first (hidden) argument
    me::functab->appendArg(self_);

    // now append all other hidden args (non atomic types)

    //for ()
    //me::functab->appendArg();
    //

    return true;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Reader::Reader(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : Method(id, parent, line)
{}

/*
 * virtual methods
 */

bool Reader::specialAnalyze()
{
    return true;
}

std::string Reader::qualifierString() const
{
    return "reader";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Writer::Writer(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : Method(id, parent, line)
{}

/*
 * virtual methods
 */

bool Writer::specialAnalyze()
{
    return true;
}

std::string Writer::qualifierString() const
{
    return "writer";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Create::Create(Symbol* parent, int line /*= NO_LINE*/)
    : Method( new std::string("create"), parent, line )
{}

/*
 * virtual methods
 */

bool Create::specialAnalyze()
{
    return true;
}

std::string Create::qualifierString() const
{
    return "create";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Assign::Assign(Symbol* parent, int line /*= NO_LINE*/)
    : Method( new std::string("assign"), parent, line)
{}

/*
 * virtual methods
 */

bool Assign::specialAnalyze()
{
    const TypeList&  in = sig_->getIn();
    const TypeList& out = sig_->getOut();

    if ( in.empty() || !out.empty() )
    {
        errorf(line_, "an assignment must have exactly one or more "
                "incoming parameters and no outgoing parameters");

        return false;
    }
    else if (in[0]->modifier() != CONST_PARAM)
    {
        swiftAssert(in[0]->modifier() == INOUT, "must be inout here");
        errorf( line_, "the argument of an assignment "
            "must not be declared as 'inout'");

        return false;
    }

    return true;
}

std::string Assign::qualifierString() const
{
    return "assign";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

StaticMethod::StaticMethod(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : MemberFunction(id, parent, line)
{}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Routine::Routine(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : StaticMethod(id, parent, line)
{}

/*
 * virtual methods
 */

bool Routine::specialAnalyze()
{
    return true;
}

std::string Routine::qualifierString() const
{
    return "routine";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Operator::Operator(std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : StaticMethod(id, parent, line)
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

    return true;
}

std::string Operator::qualifierString() const
{
    return "operator";
}

//------------------------------------------------------------------------------

} // namespace swift
