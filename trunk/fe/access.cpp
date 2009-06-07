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

#include "fe/access.h"

#include "fe/error.h"
#include "fe/memberfunction.h"
#include "fe/symtab.h"
#include "fe/type.h"

#include "me/offset.h"
#include "me/struct.h"

namespace swift {

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Access::Access(Expr* postfixExpr, int line)
    : Expr(line)
    , postfixExpr_(postfixExpr)
    , right_(true) // assume that we are right in the beginning until proven wrong
{}

Access::~Access()
{
    delete postfixExpr_;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

MemberAccess::MemberAccess(Expr* postfixExpr, std::string* id, int line)
    : Access(postfixExpr, line)
    , id_(id)
{}

MemberAccess::~MemberAccess()
{
    delete id_;
}

/*
 * further methods
 */

bool MemberAccess::analyze()
{
    MemberAccess* ma = dynamic_cast<MemberAccess*>(postfixExpr_);

    // obviously ma is not right
    if (ma)
        ma->right_ = false; 

    /*
     * The accesses are traversed from right to left to this point and from
     * left to right beyond this point.
     */
    if (postfixExpr_) 
    {
        // work with the original var
        postfixExpr_->doNotLoadPtr();
        
        if ( !postfixExpr_->analyze() )
            return false;
    }

    Class* _class; // class of the access

    /*
     * pass-through places
     */
    if (!postfixExpr_)
    {
        // -> access via implicit 'self'

        MemberFunction* mf = symtab->currentMemberFunction();
        Method* method = dynamic_cast<Method*>(mf);

        if (!method)
        {
            errorf( line_, "%ss do not have a 'self' argument", 
                    mf->qualifierString().c_str() );
            return false;
        }

        rootVar_ = method->self_;
        place_   = method->self_;
        _class   = symtab->currentClass();
    }
    else
    {
        if (!ma)
        {
            const Type* exprType = postfixExpr_->getType();
            me::Var* exprPlace = dynamic_cast<me::Var*>( postfixExpr_->getPlace() );
            if (!exprPlace)
            {
                errorf(line_, "trying to access a literal");
                return false;
            }

            if ( exprType->isActuallyPtr() )
                rootVar_ = exprType->derefToInnerstPtr(exprPlace);
            else
                rootVar_ = exprPlace;
        }
        else
        {
            // -> access via another MemberAccess
            rootVar_ = ma->rootVar_;
        }

        place_ = postfixExpr_->getPlace();
        _class = postfixExpr_->getType()->unnestPtr()->lookupClass();
    }

    /*
     * In a chain of member accesses there are three special accesses:
     * - the left most one -> ma = 0
     * - the right most one -> right = true
     * - one access is an indirect access
     */

    // get type and member var
    Class::MemberVarMap::const_iterator iter = _class->memberVars_.find(id_);

    if ( iter == _class->memberVars_.end() )
    {
        errorf( line_, "class '%s' does not have a member named %s", 
                _class->id_->c_str(), id_->c_str() );

        return false;
    }

    MemberVar* member = iter->second;
    type_ = member->type_->clone();

    structOffset_ = new me::StructOffset(_class->meStruct_, member->meMember_);

    if (ma)
    {
        ma->structOffset_->next_ = structOffset_;
        rootStructOffset_ = ma->rootStructOffset_;
        // TODO new chain
    }
    else
        rootStructOffset_ = structOffset_; // we are left

    if ( right_ )
    {
        if ( neededAsLValue_ && type_->isAtomic() )
        {
            storeNecessary_ = true;
            return true;
        }

        /*
         * create new place for the right most access 
         */
        if ( !neededAsLValue_ && type_->isAtomic() )
            place_ = type_->createVar();
        else 
        {
#ifdef SWIFT_DEBUG
            std::string tmpStr = std::string("p_") + *id_;
            place_ = me::functab->newReg(me::Op::R_PTR, &tmpStr);
#else // SWIFT_DEBUG
            place_ = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG
        }

        me::Load* load = new me::Load( (me::Var*) place_, rootVar_, rootStructOffset_ );
        me::functab->appendInstr(load); 
    }

    return true;
}

void MemberAccess::genSSA()
{
}

void MemberAccess::emitStoreIfApplicable(Expr* expr)
{
    MemberAccess* ma = dynamic_cast<MemberAccess*>(postfixExpr_);
    if (ma)
        ma->emitStoreIfApplicable(expr);
    else
    {
        // this is the root
        if (!storeNecessary_)
            return;

        me::Store* store = new me::Store( 
                rootVar_,           // memory variable
                expr->getPlace(),   // argument 
                rootStructOffset_); // offset 
        me::functab->appendInstr(store);
    }
}

std::string MemberAccess::toString() const
{
    return postfixExpr_->toString() + "." + *id_;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

IndexExpr::IndexExpr(Expr* postfixExpr, Expr* indexExpr, int line)
    : Access(postfixExpr, line)
    , indexExpr_(indexExpr)
{}

IndexExpr::~IndexExpr()
{
    delete indexExpr_;
}

/*
 * further methods
 */

bool IndexExpr::analyze()
{
    bool result = true;

    result &= postfixExpr_->analyze();
    result &= indexExpr_->analyze();

    if (!result)
        return false;

    const Container* container = 
        dynamic_cast<const Container*>( postfixExpr_->getType() );

    if (!container)
    {
        errorf(line_, "an index expression must only be used "
                "with an 'array' or 'simd' type");

        return false;
    }

    if ( !indexExpr_->getType()->isIndex() )
    {
        errorf(line_, 
               "indexing expression must be of type 'index' but '%s' is given",
               indexExpr_->getType()->toString().c_str() );

        return false;
    }

    return true;
}

void IndexExpr::genSSA()
{}

std::string IndexExpr::toString() const
{
    std::ostringstream oss;
    oss << postfixExpr_->toString() << '[' << indexExpr_->toString() << ']';

    return oss.str();
}

//------------------------------------------------------------------------------

} // namespace swift
