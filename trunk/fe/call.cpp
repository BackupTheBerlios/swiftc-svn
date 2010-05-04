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

#include "fe/class.h"
#include "fe/expr.h"
#include "fe/exprlist.h"
#include "fe/signature.h"
#include "fe/syntaxtree.h"
#include "fe/tuple.h"
#include "fe/type.h"
#include "fe/var.h"

#include "me/struct.h"

namespace swift {

/*
 * constructor
 */

Call::Call(ExprList* exprList, Tuple* tuple, Signature* sig, int simdLength)
    : exprList_(exprList)
    , tuple_(tuple)
    , sig_(sig)
    , simdLength_(simdLength)
    , place_(0)
{}

/*
 * further methods
 */

void Call::emitCall()
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
            if ( param->getType()->isAtomic() )
            {
                // -> this one is an ordinary out-param

                me::Op::Type type = param->getType()->toMeType();
                if (simdLength_)
                    type = me::Op::toSimd(type, simdLength_);

#ifdef SWIFT_DEBUG
                std::string resStr = std::string("res");
                me::Reg* res = me::functab->newReg(type, &resStr);
#else // SWIFT_DEBUG
                me::Reg* res = me::functab->newReg(type);
#endif // SWIFT_DEBUG

                me::AssignInstr* ai = new me::AssignInstr(
                        '=', res, me::functab->newUndef(res->type_) );
                me::functab->appendInstr(ai);

                out_.push_back(res);

                if (!place_)
                    place_ = res;
            }
            else
            {
                swiftAssert( typeid(*param->getType()) == typeid(BaseType), 
                        "must be a BaseType here"); // TODO container
                BaseType* bt = (BaseType*) param->getType();
                Class* _class = bt->lookupClass();
                me::Struct* _struct;

                if (simdLength_)
                    _struct = _class->meSimdStruct_;
                else
                    _struct = _class->meStruct_;

#ifdef SWIFT_DEBUG
                std::string resStr = std::string("res");
                me::MemVar* res = me::functab->newMemVar(_struct, &resStr );
#else // SWIFT_DEBUG
                me::MemVar* res = me::functab->newMemVar(_struct);
#endif // SWIFT_DEBUG

                me::AssignInstr* ai = new me::AssignInstr(
                        '=', res, me::functab->newUndef(res->type_) );
                me::functab->appendInstr(ai);

#ifdef SWIFT_DEBUG
                std::string tmpStr = std::string("p_res");
                me::Reg* tmp = me::functab->newReg(me::Op::R_PTR, &tmpStr);
#else // SWIFT_DEBUG
                me::Reg* tmp = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG

                me::functab->appendInstr( new me::LoadPtr(tmp, res, 0, 0) );

                // -> this one is a hidden in-param
                in_.push_back(tmp);

                if (!place_)
                    place_ = tmp;
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

    std::string id = sig_->getMeId();
    if (simdLength_)
        id += "simd";

    me::CallInstr* call = new me::CallInstr( out_.size(), in_.size(), id, false );

    for (size_t i = 0; i < in_.size(); ++i)
        call->arg_[i] = me::Arg( in_[i] );

    for (size_t i = 0; i < out_.size(); ++i)
        call->res_[i] = me::Res( out_[i] );

    me::functab->appendInstr(call); 

    return;
}

me::Var* Call::getPrimaryPlace()
{
    return place_;
}

Type* Call::getPrimaryType()
{
    Type* type = sig_->getNumOut() 
        ?  sig_->getOutParam(0)->getType()->varClone() 
        : 0;

    if ( type && !type->isAtomic() )
        type->modifier() = Parser::token::REF;

    return type;
}

void Call::addSelf(me::Reg* self)
{
    in_.push_back(self);
}

} // namespace swift
