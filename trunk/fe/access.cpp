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
    , nextAccess_(0)
    , firstInAChain_(false)
    , lastInAChain_(true)
{
    place_ = 0; // only last ones in a chain get a valid place
}

Access::~Access()
{
    delete postfixExpr_;
}

/*
 * virtual methods
 */

bool Access::analyze()
{
    Access* prevAccess = dynamic_cast<Access*>(postfixExpr_);

    /*
     * link and mark first and last access properly
     */
    if (prevAccess)
    {
        prevAccess->nextAccess_ = this;
        prevAccess->lastInAChain_ = false;
    }
    else
        firstInAChain_ = true;

    /*
     * The accesses are traversed from right to left to this point and from
     * left to right beyond this point.
     */
    if (postfixExpr_) 
    {
        // work with the original var if appropriate
        postfixExpr_->doNotLoadPtr();
        
        if ( !postfixExpr_->analyze() )
            return false;
    }

    /*
     * check whether we have to split here
     */
    if ( nextAccess_ && nextAccess_->needsNewChain() )
    {
        lastInAChain_ = true;
        nextAccess_->firstInAChain_ = true;
    }

    if ( !analyzeAccess() )
        return false;

    /*
     * check whether next access is a MemberAccess which automatically dereferences
     * a pointer
     */
    bool derefPtr = false;
    MemberAccess* nextMemberAccess = dynamic_cast<MemberAccess*>(nextAccess_);
    if (nextMemberAccess)
    {
        if ( type_->isActuallyPtr() )
        {
            derefPtr = true;

            // split here
            lastInAChain_ = true;
            nextMemberAccess->firstInAChain_ = true;
        }
    }

    if (prevAccess && prevAccess->lastInAChain_)
        firstInAChain_ = true;

    /*
     * set rootVar_ and rootOffset_
     */

    if (!postfixExpr_)
    {
        /*
         * -> very first access via implicit 'self'
         */

        MemberFunction* mf = symtab->currentMemberFunction();
        Method* method = dynamic_cast<Method*>(mf);

        // TODO const check
        if (!method)
        {
            errorf( line_, "%ss do not have a 'self' argument", 
                    mf->qualifierString().c_str() );
            return false;
        }

        rootVar_ = method->self_;
        rootOffset_ = offset_;
    }
    else if (!prevAccess)
    {
        /*
         * -> very first access
         */

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

        rootOffset_ = offset_;
    }
    else if (firstInAChain_)
    {
        /*
         * -> first access in a chain but not the very first one
         */

        rootVar_ = (me::Var*) prevAccess->place_;
        rootOffset_ = offset_;
    }
    else
    {
        /*
         * -> normal access which is not the first in a chain
         */

        // pass-through
        rootVar_ = prevAccess->rootVar_;       
        rootOffset_ = prevAccess->rootOffset_;

        // link previous' offset with this one's
        prevAccess->offset_->next_ = offset_;
    }

    if (lastInAChain_)
    {
        if ( dynamic_cast<IndexExpr*>(nextAccess_) )
            offset_->next_ = Container::createContainerPtrOffset();

        genSSA();
    }

    if (derefPtr)
    {
        swiftAssert(typeid(*place_) == typeid(me::Reg), "must be a Reg");
        place_ = type_->derefToInnerstPtr( (me::Reg*) place_);
        Type* type = type_->unnestInnerstPtr()->clone();
        delete type_;
        type_ = type;
    }

    return true;
}

void Access::genSSA()
{
    if ( neededAsLValue_ && type_->isAtomic() )
    {
        storeNecessary_ = true;
        return;
    }

    /*
     * create new place for the right most access 
     */

    if ( !neededAsLValue_ && type_->isAtomic() )
        place_ = type_->createVar();
    else 
    {
#ifdef SWIFT_DEBUG
        std::string tmpStr = std::string("p_"); // TODO + *id_;
        place_ = me::functab->newReg(me::Op::R_PTR, &tmpStr);
#else // SWIFT_DEBUG
        place_ = me::functab->newReg(me::Op::R_PTR);
#endif // SWIFT_DEBUG
    }

    me::InstrBase* instr;
    instr = new me::Load( (me::Var*) place_, rootVar_, rootOffset_);

    me::functab->appendInstr(instr); 
}

/*
 * further methods
 */

void Access::emitStoreIfApplicable(Expr* expr)
{
    Access* prevAccess = dynamic_cast<Access*>(postfixExpr_);
    if (prevAccess)
        prevAccess->emitStoreIfApplicable(expr);
    else
    {
        // this is the root
        if (!storeNecessary_)
            return;

        me::Store* store = new me::Store( 
                expr->getPlace(), // argument 
                rootVar_,         // memory variable
                rootOffset_);     // offset 
        me::functab->appendInstr(store);
    }
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

bool MemberAccess::analyzeAccess()
{
    Class* _class; // class of the access

    if (!postfixExpr_)
        _class = symtab->currentClass();
    else
        _class = postfixExpr_->getType()->unnestPtr()->lookupClass();

    // get type and member var
    Class::MemberVarMap::const_iterator iter = _class->memberVars_.find(id_);

    if ( iter == _class->memberVars_.end() )
    {
        errorf( line_, "class '%s' does not have a member named %s", 
                _class->id_->c_str(), id_->c_str() );

        return false;
    }

    MemberVar* member = iter->second;
    offset_ = new me::StructOffset(_class->meStruct_, member->meMember_);
    type_ = member->type_->clone();

    return true;
}

bool MemberAccess::needsNewChain() const
{
    return false;
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
 * virtual methods
 */

bool IndexExpr::analyzeAccess()
{
    if ( !indexExpr_->analyze() )
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

    type_ = container->getInnerType()->clone();

    // create temporaries
#ifdef SWIFT_DEBUG
    std::string indexStr = "index";
    me::Reg* realIndex = me::functab->newReg(me::Op::R_UINT64, &indexStr);
#else // SWIFT_DEBUG
    me::Reg* realIndex = me::functab->newReg(me::Op::R_UINT64);
#endif // SWIFT_DEBUG

    me::Const* elemSize = me::functab->newConst(me::Op::R_UINT64);
    elemSize->box_.uint64_ = container->getInnerType()->sizeOf();
    me::AssignInstr* mul = 
        new me::AssignInstr( '*', realIndex, indexExpr_->getPlace(), elemSize );
    me::functab->appendInstr(mul);
    //offset_ = new me::RTArrayOffset(realIndex);
    // TODO

    return true;
}

bool IndexExpr::needsNewChain() const
{
    return true;
}

std::string IndexExpr::toString() const
{
    std::ostringstream oss;
    oss << postfixExpr_->toString() << '[' << indexExpr_->toString() << ']';

    return oss.str();
}

//------------------------------------------------------------------------------

} // namespace swift
