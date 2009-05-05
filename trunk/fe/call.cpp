#include "fe/call.h"

#include "fe/exprlist.h"
#include "fe/signature.h"
#include "fe/tupel.h"
#include "fe/type.h"
#include "fe/var.h"

namespace swift {

/*
 * constructor
 */

Call::Call(ExprList* exprList, Tupel* tupel, Signature* sig)
    : exprList_(exprList)
    , tupel_(tupel)
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
    if (tupel_)
    {
        PlaceList resPlaceList = tupel_->getPlaceList();

        swiftAssert( sig_->getNumOut() == resPlaceList.size(),
                "sizes must match here" );

        // examine results
        Tupel* tupelIter = tupel_;
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

            tupelIter = tupelIter->next();
        }
    }
    else
    {
        // no tupel given -> create results

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

    PlaceList argPlaceList = exprList_->getPlaceList();

    // now append ordinary in-params
    for (size_t i = 0; i < argPlaceList.size(); ++i)
        in_.push_back( argPlaceList[i] );

    /*
     * create actual call
     */

    me::CallInstr* call = new me::CallInstr( 
            out_.size(), in_.size(), sig_->getMeId(), false);
            //out_.size(), in_.size(), *id_, kind_ == 'v' ? true : false );

    for (size_t i = 0; i < in_.size(); ++i)
        call->arg_[i] = me::Arg( in_[i] );

    for (size_t i = 0; i < out_.size(); ++i)
        call->res_[i] = me::Res( out_[i], out_[i]->varNr_ );

    me::functab->appendInstr(call); 

    return true;
}

void Call::emitStores()
{
    if (tupel_)
    {
        // TODO
    }
}

me::Var* Call::getPrimaryPlace()
{
    return out_.empty() ? 0 : (me::Var*) out_[0];
}

Type* Call::getPrimaryType()
{
    return out_.empty() ? 0 : sig_->getOutParam(0)->getType()->varClone();
}

} // namespace swift
