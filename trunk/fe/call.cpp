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

#include "fe/call.h"

#include "fe/expr.h"
#include "fe/exprlist.h"
#include "fe/signature.h"
#include "fe/syntaxtree.h"
#include "fe/tuple.h"
#include "fe/type.h"
#include "fe/var.h"

namespace swift {

/*
 * constructor
 */

Call::Call(ExprList* exprList, Tuple* tuple, Signature* sig)
    : exprList_(exprList)
    , tuple_(tuple)
    , sig_(sig)
{}

/*
 * further methods
 */

bool Call::emitCall()
{
    /*
     * fill in_ and out_
     */

    // are there locations to put the results or do we have to create them? 
    if (tuple_)
    {
        PlaceList resPlaceList = tuple_->getPlaceList();

        // examine results
        Tuple* tupleIter = tuple_;
        for (size_t i = 0; i < sig_->getNumOut(); ++i)
        {
            const Param* param = sig_->getOutParam(i);

            if ( param->getType()->isAtomic() )
            {
                // -> this one is an ordinary out-param
                out_.push_back( (me::Var*) resPlaceList[i] );
            }
            else
            {
                // -> this one is a hidden in-param
                swiftAssert( param->getType()->isInternalAtomic(), 
                        "must actually be a pointer" );

                in_.push_back( resPlaceList[i] );
            }

            tupleIter = tupleIter->next();
        }
    }
    else
    {
        // no tuple given -> create results

        for (size_t i = 0; i < sig_->getNumOut(); ++i)
        {
            const Param* param = sig_->getOutParam(i);

            // create place to hold the result and init with undef
#ifdef SWIFT_DEBUG
            std::string resStr = std::string("res");
            me::Reg* res = me::functab->newReg( param->getType()->toMeType(), &resStr );
#else // SWIFT_DEBUG
            me::Reg* res = me::functab->newReg( param->getType()->toMeType() );
#endif // SWIFT_DEBUG

            me::AssignInstr* ai = new me::AssignInstr(
                    '=', res, me::functab->newUndef(res->type_) );
            me::functab->appendInstr(ai);

            if ( param->getType()->isAtomic() )
            {
                // -> this one is an ordinary out-param
                out_.push_back(res);
            }
            else
            {
#ifdef SWIFT_DEBUG
                std::string tmpStr = std::string("p_res");
                me::Reg* tmp = me::functab->newReg(me::Op::R_PTR, &tmpStr);
#else // SWIFT_DEBUG
                me::Reg* tmp = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG

                me::functab->appendInstr( new me::LoadPtr(tmp, res, 0) );

                // -> this one is a hidden in-param
                in_.push_back(tmp);
            }
        }
    }

    PlaceList argPlaceList = exprList_ 
        ? exprList_->getPlaceList() 
        : PlaceList(); // use empty PlaceList when there is no ExprList

    // now append ordinary in-params
    for (size_t i = 0; i < argPlaceList.size(); ++i)
        in_.push_back( argPlaceList[i] );

    /*
     * create actual call
     */

    me::CallInstr* call = new me::CallInstr( 
            out_.size(), in_.size(), sig_->getMeId(), false);

    for (size_t i = 0; i < in_.size(); ++i)
        call->arg_[i] = me::Arg( in_[i] );

    for (size_t i = 0; i < out_.size(); ++i)
        call->res_[i] = me::Res( out_[i] );

    me::functab->appendInstr(call); 

    return true;
}

me::Var* Call::getPrimaryPlace()
{
    return out_.empty() ? 0 : (me::Var*) out_[0];
}

Type* Call::getPrimaryType()
{
    return out_.empty() ? 0 : sig_->getOutParam(0)->getType()->varClone();
}

void Call::addSelf(me::Reg* self)
{
    in_.push_back(self);
}

} // namespace swift
