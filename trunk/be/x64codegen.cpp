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

#include "be/x64parser.h"
#include "be/x64phiimpl.h"
#include "be/x64codegenhelpers.h"

//------------------------------------------------------------------------------

/*
 * globals
 */

enum Location
{
    INSTRUCTION,
    TYPE,
    OP1,
    OP2,
    OP3,
    END
};

me::InstrNode* currentInstrNode;
Location location;
std::ofstream* x64_ofs = 0;
me::StackLayout* x64_stacklayout = 0;
int lastOp;

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

    //ofs_ << "\tpushq\t%rbp\n"
         //<< "\tmovq\t%rsp, %rbp\n";

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
            // generate phi stuff already here
            swiftAssert( typeid(*iter->next()->value_) == typeid(me::LabelInstr),
                    "must be a LabelInstr here");

            // there must be exactly one successor
            if ( currentNode->succ_.size() == 1 )
            {
                genPhiInstr( currentNode, currentNode->succ_.first()->value_ );
                phisInserted = true;
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
        location = INSTRUCTION;

        x64parse();
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

    // clean up
    //ofs_ << "\tmovq\t%rbp, %rsp\n"
         //<< "\tpopq\t%rbp\n"

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

//------------------------------------------------------------------------------

/*
 * lexer to parser interface
 */

void x64error(const char *s)
{
    printf( "%s: could not parse '%s'\n", s, currentInstrNode->value_->toString().c_str() );
}

int x64lex()
{
    me::InstrBase* currentInstr = currentInstrNode->value_;

    switch (location)
    {
        case INSTRUCTION:
        {
            // set new location
            location = TYPE;

            const std::type_info& instrTypeId = typeid(*currentInstr);

            if ( instrTypeId == typeid(me::LabelInstr) )
            {
                x64lval.label_ = (me::LabelInstr*) currentInstr;
                location = END;
                return X64_LABEL;
            }
            else if (instrTypeId == typeid(me::GotoInstr) )
            {
                x64lval.goto_ = (me::GotoInstr*) currentInstr;
                return X64_GOTO;
            }
            else if (instrTypeId == typeid(me::BranchInstr) )
            {
                me::BranchInstr* bi = (me::BranchInstr*) currentInstr;
                x64lval.branch_ = (me::BranchInstr*) currentInstr;

                me::InstrNode* nextNode = currentInstrNode->next();
                swiftAssert( typeid(*nextNode->value_) == typeid(me::LabelInstr),
                        "must be a LabelInstr" );
                me::LabelInstr* nextLabel = (me::LabelInstr*) nextNode->value_;

                if (bi->trueLabel() == nextLabel)
                    return X64_BRANCH_FALSE;
                else if (bi->falseLabel() == nextLabel)
                    return X64_BRANCH_TRUE;
                else
                    return X64_BRANCH;
            }
            else if ( instrTypeId == typeid(me::AssignInstr) )
            {
                me::AssignInstr* ai = (me::AssignInstr*) currentInstr;
                x64lval.assign_ = ai;

                switch (ai->kind_)
                {
                    case '=': return X64_MOV;
                    case '+': return X64_ADD;
                    case '*': return X64_MUL;
                    case '-': return X64_SUB;
                    case '/': return X64_DIV;
                    case me::AssignInstr::EQ: return X64_EQ;
                    case me::AssignInstr::NE: return X64_NE;
                    case '<': return X64_L;
                    case '>': return X64_G;
                    case me::AssignInstr::LE: return X64_LE;
                    case me::AssignInstr::GE: return X64_GE;
                    case '^': return X64_DEREF;
                    case '&': return X64_AND;
                    case me::AssignInstr::UNARY_MINUS: return X64_UN_MINUS;

                    default:
                        swiftAssert(false, "unreachable code");
                }
            }
            else if ( instrTypeId == typeid(me::Spill) )
            {
                me::Spill* spill = (me::Spill*) currentInstr;
                x64lval.spill_ = spill;
                return X64_SPILL;
            }
            else if ( instrTypeId == typeid(me::Reload) )
            {
                me::Reload* reload = (me::Reload*) currentInstr;
                x64lval.reload_ = reload;
                return X64_RELOAD;
            }
            else if ( instrTypeId == typeid(me::Load) )
            {
                me::Load* load = (me::Load*) currentInstr;
                x64lval.load_ = load;
                return X64_LOAD;
            }
            else if ( instrTypeId == typeid(me::Store) )
            {
                me::Store* store = (me::Store*) currentInstr;
                x64lval.store_ = store;
                return X64_STORE;
            }
            else if ( instrTypeId == typeid(me::LoadPtr) )
            {
                me::LoadPtr* loadPtr = (me::LoadPtr*) currentInstr;
                x64lval.loadPtr_ = loadPtr;
                location = OP1;
                return X64_LOAD_PTR;
            }
            else if ( dynamic_cast<me::CallInstr*>(currentInstr) )
            {
                me::CallInstr* call = (me::CallInstr*) currentInstr;
                x64lval.call_ = call;
                location = END;
                return X64_CALL;
            }
            else
            {
                swiftAssert( instrTypeId == typeid(me::NOP), "must be a NOP" );
                location = END;
                return X64_NOP;
            }
        }
        case TYPE:
        {
            if ( currentInstr->arg_.empty() )
                return 0;

            // set new location
            location = OP1;

            me::AssignInstr* ai = dynamic_cast<me::AssignInstr*>(currentInstr);
            me::Op::Type type;
            if ( typeid(*currentInstr) == typeid(me::Load) )
                type = currentInstr->res_[0].var_->type_;
            else if ( ai && ai->isUnary() )
                type = currentInstr->res_[0].var_->type_;
            else
                type = currentInstr->arg_[0].op_->type_;

            switch (type)
            {
                case me::Op::R_BOOL:   return X64_BOOL;

                case me::Op::R_INT8:   return X64_INT8;
                case me::Op::R_INT16:  return X64_INT16;
                case me::Op::R_INT32:  return X64_INT32;
                case me::Op::R_INT64:  return X64_INT64;
                case me::Op::R_SAT8:   return X64_SAT8;
                case me::Op::R_SAT16:  return X64_SAT16;

                case me::Op::R_UINT8:  return X64_UINT8;
                case me::Op::R_UINT16: return X64_UINT16;
                case me::Op::R_UINT32: return X64_UINT32;
                case me::Op::R_UINT64: return X64_UINT64;
                case me::Op::R_USAT8:  return X64_USAT8;
                case me::Op::R_USAT16: return X64_USAT16;

                case me::Op::R_REAL32: return X64_REAL32;
                case me::Op::R_REAL64: return X64_REAL64;

                case me::Op::R_PTR:    return X64_UINT64;
                case me::Op::R_MEM:    return X64_STACK;

                case me::Op::S_INT8:   return X64_S_INT8;
                case me::Op::S_INT16:  return X64_S_INT16;
                case me::Op::S_INT32:  return X64_S_INT32;
                case me::Op::S_INT64:  return X64_S_INT64;
                case me::Op::S_SAT8:   return X64_S_SAT8;
                case me::Op::S_SAT16:  return X64_S_SAT16;

                case me::Op::S_UINT8:  return X64_S_UINT8;
                case me::Op::S_UINT16: return X64_S_UINT16;
                case me::Op::S_UINT32: return X64_S_UINT32;
                case me::Op::S_UINT64: return X64_S_UINT64;
                case me::Op::S_USAT8:  return X64_S_USAT8;
                case me::Op::S_USAT16: return X64_S_USAT16;

                case me::Op::S_REAL32: return X64_S_REAL32;
                case me::Op::S_REAL64: return X64_S_REAL64;
            }
        }
        case OP1:
        {
            if ( currentInstr->arg_.empty() )
                return 0;

            location = OP2;

            me::Op* op = currentInstr->arg_[0].op_;
            const std::type_info& opTypeId = typeid(*op);

            if ( opTypeId == typeid(me::Undef) )
            {
                x64lval.undef_ = (me::Undef*) op;
                lastOp = X64_UNDEF;
            }
            else if ( opTypeId == typeid(me::Const) )
            {
                x64lval.const_ = (me::Const*) op;
                lastOp = X64_CONST;
            }
            else if ( opTypeId == typeid(me::MemVar) )
            {
                x64lval.memVar_ = (me::MemVar*) op;
                lastOp = X64_MEM_VAR;
            }
            else 
            {
                swiftAssert( typeid(*op) == typeid(me::Reg), "must be a Reg" );
                me::Reg* reg = (me::Reg*) op;
                x64lval.reg_ = reg;

                if ( reg->isSpilled() )
                    lastOp = X64_REG_1;
                else if ( currentInstr->res_.empty() )
                    lastOp = X64_REG_1;
                else if ( typeid(*currentInstr->res_[0].var_) != typeid(me::Reg) )
                    lastOp = X64_REG_1;
                else if ( currentInstr->res_[0].var_->isSpilled() )
                    lastOp = X64_REG_1;
                else if ( currentInstr->res_[0].var_->color_ == reg->color_ )
                    lastOp = X64_REG_1;
                else
                    lastOp = X64_REG_2;
            }
            return lastOp;
        }
        case OP2:
        {
            location = END;

            if ( currentInstr->arg_.size() < 2 )
                return 0;

            me::Op* op = currentInstr->arg_[1].op_;
            const std::type_info& opTypeId = typeid(*op);

            if ( opTypeId == typeid(me::Undef) )
            {
                x64lval.undef_ = (me::Undef*) op;
                lastOp = X64_UNDEF;
            }
            else if ( opTypeId == typeid(me::Const) )
            {
                x64lval.const_ = (me::Const*) op;
                lastOp = X64_CONST;
            }
            else 
            {
                if ( typeid(*currentInstr) == typeid(me::Store) )
                {
                    if (currentInstr->arg_.size() == 3)
                        location = OP3;

                    me::MemVar* memVar = dynamic_cast<me::MemVar*>(op);
                    if (memVar)
                    {
                        x64lval.memVar_ = memVar;
                        lastOp = X64_MEM_VAR;
                    }
                    else
                    {
                        swiftAssert( typeid(*op) == typeid(me::Reg), 
                                "must be a Reg here");
                        me::Reg* reg = (me::Reg*) op;
                        x64lval.reg_ = reg;
                        lastOp = X64_REG_2;
                    }
                }
                else
                {
                    swiftAssert( typeid(*op) == typeid(me::Reg), "must be a Reg here");
                    me::Reg* reg = (me::Reg*) op;
                    x64lval.reg_ = reg;

                    /*
                    * The following cases should be considered:
                    * - res.color == op2.color                 -> reg1
                    * - op1 == reg, op1.color == op2.color_    -> reg2
                    * - op1 == reg1                            -> reg2
                    * - op1 == const                           -> reg2
                    * - else                                   -> reg3
                    */
                    if ( currentInstr->res_[0].var_->color_ == reg->color_ )
                        lastOp = X64_REG_1;
                    else if ( lastOp == X64_REG_2 && ((me::Reg*) currentInstr->arg_[0].op_)->color_ == reg->color_ )
                        lastOp = X64_REG_2;
                    else if (lastOp == X64_REG_1)
                        lastOp = X64_REG_2;
                    else if (lastOp == X64_CONST)
                        lastOp = X64_REG_2;
                    else
                        return X64_REG_3;
                }
            }

            return lastOp;
        }
        case OP3:
        {
            location = END;
            me::Op* op = currentInstr->arg_[2].op_;
            swiftAssert( typeid(*op) == typeid(me::Reg), 
                    "must be a Reg here");
            me::Reg* reg = (me::Reg*) op;
            x64lval.reg_ = reg;
            lastOp = X64_REG_4;
            return lastOp;
        }
        case END:
            return 0;
    }

    return 0;
}
