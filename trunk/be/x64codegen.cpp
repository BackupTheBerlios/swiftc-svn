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

#include "be/x64codegen.h"

#include <iostream>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"
#include "me/stacklayout.h"

#include "be/x64codegenhelpers.h"
#include "be/x64lexer.h"
#include "be/x64parser.h"
#include "be/x64phiimpl.h"

//------------------------------------------------------------------------------

namespace be {

/*
 * constructor
 */

X64CodeGen::X64CodeGen(me::Function* function, std::ofstream& ofs)
    : CodeGen(function, ofs)
{
    x64_ofs = &ofs;
    x64_stacklayout = function_->stackLayout_;
}

/*
 * further methods
 */

void X64CodeGen::process()
{
    static int counter = 1;
    std::string id;

    if ( function_->isMain() )
        id = "main";
    else
        id = *function_->id_;

    function_->stackLayout_->arangeStackLayout();

    // function prologue
    ofs_ //<< "\t.p2align 4,,15\n"
         << "\t.globl\t" << id << '\n'
         << "\t.type\t" << id << ", @function\n"
         << id << ":\n"
         << ".LFB" << counter << ":\n";

    const me::Colors& usedColors = function_->usedColors_;

    // count number of pushes
    int numPushes = 0;
    if ( usedColors.contains(X64RegAlloc::RBX) ) ++numPushes;
    if ( usedColors.contains(X64RegAlloc::RBP) ) ++numPushes;
    if ( usedColors.contains(X64RegAlloc::R12) ) ++numPushes;
    if ( usedColors.contains(X64RegAlloc::R13) ) ++numPushes;
    if ( usedColors.contains(X64RegAlloc::R14) ) ++numPushes;
    if ( usedColors.contains(X64RegAlloc::R15) ) ++numPushes;

    // align stack
    numPushes = (numPushes & 0x00000001) ? 8 : 0; // 8 if odd, 0 if even
    numPushes = 8 - numPushes;

    int stackSize = function_->stackLayout_->size_;
    if ( (stackSize + numPushes) & 0x0000000F ) // (stackSize + numPushes) % 16
        stackSize = ((stackSize  + numPushes + 15) & 0xFFFFFFF0) - numPushes;

    if (stackSize)
        ofs_ << "\tsubq\t$" << stackSize << ", %rsp\n";

    // save registers which should be preserved during function calls
    if ( usedColors.contains(X64RegAlloc::RBX) ) ofs_ << "\tpushq\t%rbx\n";
    if ( usedColors.contains(X64RegAlloc::RBP) ) ofs_ << "\tpushq\t%rbp\n";
    if ( usedColors.contains(X64RegAlloc::R12) ) ofs_ << "\tpushq\t%r12\n";
    if ( usedColors.contains(X64RegAlloc::R13) ) ofs_ << "\tpushq\t%r13\n";
    if ( usedColors.contains(X64RegAlloc::R14) ) ofs_ << "\tpushq\t%r14\n";
    if ( usedColors.contains(X64RegAlloc::R15) ) ofs_ << "\tpushq\t%r15\n";

    me::BBNode* currentNode = 0;
    bool phisInserted = false;

    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        me::InstrBase* instr = iter->value_;

        // check whether we have a new basic block
        if ( typeid(*instr) == typeid(me::LabelInstr) )
        {
            me::BBNode* oldNode = currentNode;
            currentNode = cfg_->labelNode2BBNode_[iter];

            if (!phisInserted)
                genPhiInstr(oldNode, currentNode);

            currentNode = cfg_->labelNode2BBNode_[iter];
            phisInserted = false;
        }
        else if ( dynamic_cast<me::JumpInstr*>(instr) )
        {
            me::JumpInstr* jump = (me::JumpInstr*) instr;

            // generate phi stuff already here
            swiftAssert( typeid(*iter->next()->value_) == typeid(me::LabelInstr),
                    "must be a LabelInstr here");

            // there must be exactly one successor
            if ( currentNode->succ_.size() == 1 )
            {
                me::BBNode* target = currentNode->succ_.first()->value_;
                swiftAssert( target == jump->bbTargets_[0], 
                        "must be the same" );
                genPhiInstr(currentNode, target);
                phisInserted = true;

                me::InstrNode* labelIter = iter->next();
                while ( labelIter != cfg_->instrList_.sentinel() 
                        && typeid(*labelIter) == typeid(me::LabelInstr) )
                {
                    if (labelIter == jump->instrTargets_[0])
                    {
                        std::cout << labelIter->value_->toString() << std::endl;
                        goto outer_loop;
                    }

                    labelIter = labelIter->next();
                }
            }
        }
        else if ( typeid(*instr) == typeid(me::AssignInstr) )
        {
            me::AssignInstr* ai = (me::AssignInstr*) instr;

            if (ai->kind_ == '=')
            {
                if ( typeid(*ai->arg_[0].op_) == typeid(me::Undef) )
                    continue; // ignore defition of undefined vars
            }
        }
        else if ( typeid(*instr) == typeid(me::Cast) )
        {
            me::Cast* cast = (me::Cast*) instr;
            ofs_ << cast2str(cast);
            continue;
        }
        else if ( typeid(*instr) == typeid(me::PhiInstr) )
            continue;
        else if ( typeid(*instr) == typeid(me::SetParams) )
        {
            // TODO
            continue;
        }
        else if ( typeid(*instr) == typeid(me::SetResults) )
        {
            // TODO
            continue;
        }

        // update globals for x64lex
        currentInstrNode = iter;

        x64parse();

outer_loop:
        continue;
    }

    /*
     * function epilogue
     */

    // restore saved registers
    if ( usedColors.contains(X64RegAlloc::R15) ) ofs_ << "\tpopq\t%r15\n";
    if ( usedColors.contains(X64RegAlloc::R14) ) ofs_ << "\tpopq\t%r14\n";
    if ( usedColors.contains(X64RegAlloc::R13) ) ofs_ << "\tpopq\t%r13\n";
    if ( usedColors.contains(X64RegAlloc::R12) ) ofs_ << "\tpopq\t%r12\n";
    if ( usedColors.contains(X64RegAlloc::RBP) ) ofs_ << "\tpopq\t%rbp\n";
    if ( usedColors.contains(X64RegAlloc::RBX) ) ofs_ << "\tpopq\t%rbx\n";

    if (stackSize)
        ofs_ << "\taddq\t$" << stackSize << ", %rsp\n";

    ofs_ << "\tret\n";

    ofs_ << ".LFE" << counter << ":\n"
         << "\t.size\t" << id << ", .-" << id << '\n';

    ++counter;
    ofs_ << '\n';
}

/*
 * helpers
 */

void X64CodeGen::genPhiInstr(me::BBNode* prevNode, me::BBNode* nextNode)
{
    if ( !nextNode->value_->hasPhiInstr() )
        return; // nothing to do

#ifdef SWIFT_DEBUG
    ofs_ << "\t /* phi functions */\n";
#endif // SWIFT_DEBUG

    /*
     * phi-spilled first
     */

    // quadword spill slots -> general purpose registers are used
    X64PhiImpl(X64PhiImpl::QUADWORD_SPILL_SLOTS,
            prevNode, nextNode, function_->usedColors_, ofs_).genPhiInstr();

    // octword spill slots -> xmm registers are used 
    X64PhiImpl(X64PhiImpl::OCTWORD_SPILL_SLOTS,
            prevNode, nextNode, function_->usedColors_, ofs_).genPhiInstr();

    /*
     * now non-phi-spilled
     */

    // integer registers
    X64PhiImpl(X64PhiImpl::INT_REG,
            prevNode, nextNode, function_->usedColors_, ofs_).genPhiInstr();

    // xmm registers
    X64PhiImpl(X64PhiImpl::XMM_REG,
            prevNode, nextNode, function_->usedColors_, ofs_).genPhiInstr();

#ifdef SWIFT_DEBUG
    ofs_ << "\t /* end phi functions */\n";
#endif // SWIFT_DEBUG
}

} // namespace be
