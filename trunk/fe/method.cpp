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

#include "fe/method.h"

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

/*
 * constructor and destructor
 */

Method::Method(int methodQualifier, std::string* id, Symbol* parent, int line /*= NO_LINE*/)
    : ClassMember(id, parent, line)
    , methodQualifier_(methodQualifier)
    , rootScope_( new Scope(0) )
    , sig_( new Signature() )
{}

Method::~Method()
{
    delete statements_;
    delete rootScope_;
    delete sig_;
}

/*
 * further methods
 */

bool Method::analyze()
{
    symtab->enterMethod(this);

    /*
     * build a function name for the me::functab consisting of the class name,
     * the method name and a counted number to prevent name clashes
     * due to overloading
     */
    std::ostringstream oss;

    oss << *symtab->class_->id_ << '$';

    if (methodQualifier_ == OPERATOR)
        oss << "operator";
    else
        oss << *id_;

    static int counter = 0;

    oss << '$' << counter;
    ++counter;

    me::functab->insertFunction( new std::string(oss.str()), isTrivial() );

    bool result = true;
    result &= sig_->analyze();

    // insert the first label since every function must start with one
    me::functab->appendInstr( new me::LabelInstr() );

    const TypeList&  in = sig_->getIn();
    const TypeList& out = sig_->getOut();

    // is it an operator?
    if (methodQualifier_ == OPERATOR)
    {
        /*
         * check signature
         */
        if ( !in.empty() )
        {
            // check whether the first type matches the type of the current class
            const BaseType* bt = dynamic_cast<const BaseType*>( in[0] );
            if ( !bt || *symtab->class_->id_ != *bt->getId() )
            {
                errorf( line_, "the first parameter of the '%s'-operator must be of type %s",
                    id_->c_str(),
                    symtab->class_->id_->c_str() );
                result = false;
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
                    errorf( line_, "the '%s'-operator must exactly have two incoming and one outgoing parameter",
                        id_->c_str() );
                    result = false;
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
                result = false;
            }
        }
    }
    else if (methodQualifier_ == ASSIGN)
    {
        if ( in.empty() || !out.empty() )
        {
            errorf(line_, "an assignment must have exactly one or more "
                    "incoming parameters and no outgoing parameters");
            result = false;
        }
        else if (in[0]->modifier() != CONST_PARAM)
        {
            swiftAssert(in[0]->modifier() == INOUT, "must be inout here");
            errorf( line_, "the argument of an assignment "
                "must not be declared as 'inout'");
            result = false;
        }
    }

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

    symtab->leaveMethod();

    return result;
}

// std::string Method::toString() const
// {
//     std::ostringstream oss;
//
//     switch (methodQualifier_)
//     {
//         case CREATE:                        break;
//         case READER:    oss << "reader ";   break;
//         case WRITER:    oss << "writer ";   break;
//         case ROUTINE:   oss << "routine ";  break;
//         case OPERATOR:  oss << "operator "; break;
//         default:
//             swiftAssert(false, "illegal case value");
//             return "";
//     }
//
//     return oss.str() + sig_->toString();
// }

bool Method::isTrivial() const
{
    return !statements_;
}

} // namespace swift
