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

#include "fe/simdprefix.h"

#include "fe/error.h"
#include "fe/expr.h"
#include "fe/type.h"

#include "me/functab.h"

namespace swift {

/*
 * constructor and destructor
 */

SimdPrefix::SimdPrefix(Expr* leftExpr, Expr* rightExpr, int line)
    : Node(line)
    , leftExpr_(leftExpr)
    , rightExpr_(rightExpr)
{
}

SimdPrefix::~SimdPrefix()
{
    delete leftExpr_;
    delete rightExpr_;
}

/*
 * further methods
 */

/*
 * generate this SSA code:
 *
 *     $simd_counter = left
 * simdLabelNode:
 *     $simd_check = left < rigt
 *     IF simd_check THEN trueLabelNode ELSE nextLabelNode
 * trueLabelNode:
 *     //...
 *     GOTO whileLabelNode
 * nextLabelNode:
 *     //...
 */

bool SimdPrefix::analyze()
{
    bool result = true;

    // TODO get bounds for 0 left/rigt expressions
    swiftAssert(leftExpr_ && rightExpr_, "TODO");

    if (leftExpr_)
        result &= leftExpr_->analyze();

    if (rightExpr_)
        result &= rightExpr_->analyze();

    if (!result)
        return false;

    if ( leftExpr_ && !leftExpr_->getType()->isIndex() )
    {
        errorf(line_, "type of the left expression must be of type 'index'");
        return false;
    }

    if ( rightExpr_ && !rightExpr_->getType()->isIndex() )
    {
        errorf(line_, "type of the right expression must be of type 'index'");
        return false;
    }

    return true;
}

void SimdPrefix::genPreSSA()
{
    /*
     * generate this SSA code:
     *
     *     $simd_counter = left
     * simdLabelNode:
     *     $SIMd_check = $simd_counter < rigt
     *     IF simd_check THEN trueLabelNode ELSE nextLabelNode
     * trueLabelNode:
     *     //...                             <- will be created by the parent statement
     *     $simd_counter = $simd_counter + 1 <- will be created by genSSA
     *     GOTO simdLabelNode                <- will be created by genSSA
     * nextLabelNode:                        <- will be created by genSSA
     *     //...
     */

    // create regs
#ifdef SWIFT_DEBUG
    std::string counterStr = "$simd_counter";
    std::string checkStr   = "$simd_check";
    counter_ = me::functab->newReg(me::Op::R_UINT64, &counterStr);
    check_   = me::functab->newReg(me::Op::R_BOOL,   &checkStr);
#else // SWIFT_DEBUG
    counter_ = me::functab->newReg(me::Op::R_UINT64);
    check_   = me::functab->newReg(me::Op::R_BOOL);
#endif // SWIFT_DEBUG

    // create labels
    simdLabelNode_ = new me::InstrList::Node( new me::LabelInstr() );
    nextLabelNode_ = new me::InstrList::Node( new me::LabelInstr() );
    me::InstrNode* trueLabelNode = 
                     new me::InstrList::Node( new me::LabelInstr() );

    // $simd_counter = left
    me::functab->appendInstr( new me::AssignInstr(
                '=', counter_, leftExpr_->getPlace()) );

    // simdLabelNode:
    me::functab->appendInstrNode(simdLabelNode_);

    // $simd_check = $simd_counter < right
    me::functab->appendInstr( new me::AssignInstr(
                '<', check_, counter_, rightExpr_->getPlace()) );

    // IF $simd_check THEN trueLabelNode ELSE nextLabelNode_
    me::functab->appendInstr( new me::BranchInstr(
                check_, trueLabelNode, nextLabelNode_) );

    // trueLabelNode:
    me::functab->appendInstrNode(trueLabelNode);
}

void SimdPrefix::genPostSSA(SimdAnalyses& simdAnalyzes)
{
    /*
     * generate this SSA code:
     *
     *     $simd_counter = left                                 <- already created
     * simdLabelNode:                                           <- already created
     *     $simd_check = left < rigt                            <- already created
     *     IF simd_check THEN trueLabelNode ELSE nextLabelNode  <- already created
     * trueLabelNode:                                           <- already created
     *     //...                                                <- already created
     *     $simd_counter = $simd_counter + 1
     *     GOTO simdLabelNode
     * nextLabelNode:
     *     //...
     */

    // $simd_counter = $simd_counter + simdLength_
    me::Const* cst = me::functab->newConst(me::Op::R_UINT64);
    cst->box().uint64_ = simdAnalyzes[0].simdLength_;
    me::functab->appendInstr( new me::AssignInstr(
                '+', counter_, counter_, cst) );

    for (size_t i = 0; i < simdAnalyzes.size(); ++i)
    {
        SimdAnalysis& simd = simdAnalyzes[i];

        me::functab->appendInstr( new me::AssignInstr( 
                    '+', simd.ptr_, simd.ptr_, cst) );
    }

    // GOTO simdLabelNode_
    me::functab->appendInstr( new me::GotoInstr(simdLabelNode_) );
    me::functab->appendInstrNode(nextLabelNode_);
}

/*
 * virtual methods
 */

std::string SimdPrefix::toString() const
{
    return "TODO";
}

} // namespace swift
