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

#include "be/x64regalloc.h"

#include <sstream>
#include <typeinfo>

#include "me/coloring.h"
#include "me/constpool.h"
#include "me/copyinsertion.h"
#include "me/defusecalc.h"
#include "me/functab.h"
#include "me/livenessanalysis.h"
#include "me/liverangesplitting.h"
#include "me/spiller.h"
#include "me/stacklayout.h"

#include "be/x64.h"

namespace be {

/*
 * constructor
 */

X64RegAlloc::X64RegAlloc(me::Function* function)
    : me::RegAlloc(function)
    , omitFramePointer_(false)
{}

/*
 * methods
 */

/*
 *  register targeting
 *          |
 *          v
 *  spill general purpose registers
 *          |
 *          v
 *  spill XMM registers
 *          |
 *          v
 *  live range splitting
 *          |
 *          v
 *  color general purpose registers
 *          |
 *          v
 *  color XMM registers
 */
void X64RegAlloc::process()
{
    /*
     * do the register targeting
     */

    registerTargeting();
    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * spill general purpose registers
     */

    me::Colors rColors;
    for (int i = R0; i <= R15; ++i)
        rColors.insert(i);

    // do not use stack pointer as a free register
    rColors.erase(RSP);

    // check whether the frame pointer can be used
    if (!omitFramePointer_)
        rColors.erase(RBP);

    // use this to test better the spilling
#if 0
    rColors.erase(RCX);
    rColors.erase(RSI);
    rColors.erase(RDI);
    rColors.erase(R8 );
    rColors.erase(R9 );
    rColors.erase(R10);
    rColors.erase(R11);
    rColors.erase(R12);
    rColors.erase(R13);
    rColors.erase(R14);
    rColors.erase(R15);
#endif

    me::Spiller( function_, rColors.size(), R_TYPE_MASK ).process();

    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * spill XMM registers
     */

    me::Colors fColors;
    for (int i = XMM0; i <= XMM15; ++i)
        fColors.insert(i);

    me::Spiller( function_, fColors.size(), F_TYPE_MASK ).process();

    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * copy insertion -> faithful fixing -> live range splitting
     */

    me::CopyInsertion(function_).process();
    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * TODO FaithFulFixing is ignored at the moment !!!!!!
     */
    //me::FaithFulFixing(function_).process();
    // recalulate def-use and liveness stuff
    //me::DefUseCalc(function_).process();
    //me::LivenessAnalysis(function_).process();

    me::LiveRangeSplitting(function_).process();
    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * coloring
     */

    // general purpose registers
    me::Coloring(function_, R_TYPE_MASK, rColors).process();

    // XMM registers
    me::Coloring(function_, F_TYPE_MASK, fColors).process(); 

    // color spill slots
    me::Coloring(function_, R_TYPE_MASK | F_TYPE_MASK, X64::QUADWORDS).process();
    //me::Coloring(function_, V_TYPE_MASK, X64::OCTWORDS).process();

    // calculate all offsets and the like
    function_->stackLayout_->arangeStackLayout();
}

void X64RegAlloc::registerTargeting()
{
    me::BBNode* currentBB;

    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        me::InstrBase* instr = iter->value_;

        static const size_t NUM_INT_REGS = 6;
        static const size_t NUM_REAL_REGS = 8;
        static int  intRegs[NUM_INT_REGS] = {RDI, RSI, RDX, RCX, R8, R9};
        static int realRegs[NUM_REAL_REGS] = {XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7};
        
        if ( typeid(*instr) == typeid(me::LabelInstr) )
        {
            currentBB = cfg_->labelNode2BBNode_[iter];
            continue;
        }
        else if ( typeid(*instr) == typeid(me::BranchInstr) )
        {
            me::BranchInstr* bi = (me::BranchInstr*) instr;

            /* 
             * check whether the preceding instruction is the definition
             * of bi->getOp()
             */

            me::InstrNode* preNode = iter->prev();

            if ( dynamic_cast<me::Var*>(bi->getOp()) )
            {
                me::Var* var = (me::Var*) bi->getOp();

                if (var->def_.instrNode_ == preNode)
                {
                    // is it even an AssignInstr?
                    if ( typeid(*preNode->value_) == typeid(me::AssignInstr) )
                    {
                        me::AssignInstr* ai = (me::AssignInstr*) preNode->value_;

                        if ( ai->isComparison() )
                        {
                            // do nothing when both are Consts,
                            // this should be optimized away
                            if ( typeid(*ai->arg_[0].op_) == typeid(me::Const)
                                    && typeid(*ai->arg_[0].op_) == typeid(me::Const))
                                continue;

                            // check whether this is the only use
                            if (var->uses_.size() == 1)
                            {
                                // do not color this var
                                var->type_ = me::Op::R_SPECIAL;
                            }

                            me::Op::Type type = ai->arg_[0].op_->type_;

                            if (       type == me::Op::R_INT8  || type == me::Op::R_INT16 
                                    || type == me::Op::R_INT32 || type == me::Op::R_INT64
                                    || type == me::Op::R_SAT8  || type == me::Op::R_SAT16)
                            {
                                switch (ai->kind_)
                                {
                                    case me::AssignInstr::EQ: bi->cc_ = C_EQ; break;
                                    case me::AssignInstr::NE: bi->cc_ = C_NE; break;
                                    case '<':                 bi->cc_ = C_L ; break;
                                    case '>':                 bi->cc_ = C_G ; break;
                                    case me::AssignInstr::LE: bi->cc_ = C_LE; break;
                                    case me::AssignInstr::GE: bi->cc_ = C_GE; break;
                                    default:
                                        swiftAssert(false, "unreachable code");
                                }
                            }
                            else
                            {
                                switch (ai->kind_)
                                {
                                    case me::AssignInstr::EQ: bi->cc_ = C_EQ; break;
                                    case me::AssignInstr::NE: bi->cc_ = C_NE; break;
                                    case '<':                 bi->cc_ = C_B ; break;
                                    case '>':                 bi->cc_ = C_A ; break;
                                    case me::AssignInstr::LE: bi->cc_ = C_BE; break;
                                    case me::AssignInstr::GE: bi->cc_ = C_AE; break;
                                    default:
                                        swiftAssert(false, "unreachable code");
                                }
                            }
                        } // if comparison
                    } // if AssignInstr
                    continue;
                } // if var defined in previous instruction
            } // if bi->getOP() is a Var
            else 
            {
                swiftAssert( typeid(*bi->getOp()) == typeid(me::Const),
                        "must be a me::Const here" );
            }

            continue;
        }
        else if ( typeid(*instr) == typeid(me::SetParams) )
        {
            me::SetParams* sp = (me::SetParams*) instr;

            if ( !sp->res_.empty() )
                sp->constrain();

            int intCounter = 0;
            int realCounter = 0;

            for (size_t i = 0; i < sp->res_.size(); ++i)
            {
                me::Op::Type type = sp->res_[i].var_->type_;

                switch (type)
                {
                    case me::Op::R_BOOL:
                    case me::Op::R_INT8:
                    case me::Op::R_INT16:
                    case me::Op::R_INT32:
                    case me::Op::R_INT64:
                    case me::Op::R_SAT8:
                    case me::Op::R_SAT16:
                    case me::Op::R_UINT8:
                    case me::Op::R_UINT16:
                    case me::Op::R_UINT32:
                    case me::Op::R_UINT64:
                    case me::Op::R_USAT8:
                    case me::Op::R_USAT16:
                        sp->res_[i].constraint_ = intRegs[intCounter++];
                        break;

                    case me::Op::R_REAL32:
                    case me::Op::R_REAL64:
                        sp->res_[i].constraint_ = realRegs[realCounter++];
                        break;

                    default:
                        swiftAssert( false, "unreachable code" );
                }
            }

            continue;
        }
        else if ( typeid(*instr) == typeid(me::SetResults) )
        {
            me::SetResults* sr = (me::SetResults*) instr;
            swiftAssert( !sr->arg_.empty(), "must not be empty" );
            sr->constrain();

            // TODO only one result supported
            me::Op::Type type = sr->arg_[0].op_->type_;
            switch (type)
            {
                case me::Op::R_BOOL:
                case me::Op::R_INT8:
                case me::Op::R_INT16:
                case me::Op::R_INT32:
                case me::Op::R_INT64:
                case me::Op::R_SAT8:
                case me::Op::R_SAT16:
                case me::Op::R_UINT8:
                case me::Op::R_UINT16:
                case me::Op::R_UINT32:
                case me::Op::R_UINT64:
                case me::Op::R_USAT8:
                case me::Op::R_USAT16:
                    sr->arg_[0].constraint_ = RAX;
                    break;

                case me::Op::R_REAL32:
                case me::Op::R_REAL64:
                    sr->arg_[0].constraint_ = XMM0;
                    break;

                default:
                    swiftAssert( false, "unreachable code" );
            }

            continue;
        }
        else if ( typeid(*instr) == typeid(me::AssignInstr) )
        {
            me::AssignInstr* ai = (me::AssignInstr*) instr;
            swiftAssert( ai->res_.size() >= 1, "one or more results must be here" );
            swiftAssert( ai->arg_.size() == 1 || ai->arg_.size() == 2,
                    "one or two args must be here" );

            me::Op::Type type = ai->res_[0].var_->type_;

            if (       ai->kind_ == me::AssignInstr::EQ
                    || ai->kind_ == me::AssignInstr::NE
                    || ai->kind_ == me::AssignInstr::LE
                    || ai->kind_ == me::AssignInstr::GE
                    || ai->kind_ == '<'
                    || ai->kind_ == '>')
            {
                // TODO
                continue;
            }

            /*
             * forbidden instructions:
             * r1 =  c / r1     ( reg = c / reg_dead )
             *      rewrite as:
             *      tmp = c
             *      a = tmp / b
             *
             * r1 = r2 / r1     ( reg = reg / reg_dead)
             *      rewrite as:
             *      a = b / c
             *      NOP(c)
             *      
             */

            if (ai->kind_ == '/')
            {
                swiftAssert( ai->arg_.size() == 2, "must have exactly two args" );
            
                me::InstrBase::OpType opType1 = ai->getOpType(0);
                me::InstrBase::OpType opType2 = ai->getOpType(1);

                if (opType2 == me::InstrBase::VAR_DEAD)
                {
                    if (opType1 == me::InstrBase::CONST)
                    {
                        swiftAssert( typeid(*ai->arg_[0].op_) == typeid(me::Const), 
                                "must be a Const here");
                        me::Const* cst = (me::Const*) ai->arg_[0].op_;

                        // create var which will hold the constant, the assignment and insert
                        me::Var* newVar = function_->newSSAReg(type);
                        me::AssignInstr* newCopy = new me::AssignInstr('=', newVar, cst);
                        cfg_->instrList_.insert( iter->prev(), newCopy );

                        // substitute operand with newVar
                        instr->arg_[0].op_ = newVar;

                        currentBB->value_->fixPointers();
                    }
                    else if (opType1 == me::InstrBase::VAR)
                    {
                        // insert artificial use for all critical cases
                        me::Var* var = (me::Var*) ai->arg_[1].op_;
                        cfg_->instrList_.insert( iter, new me::NOP(var) );

                        currentBB->value_->fixPointers();
                    }
                } // if op2 VAR_DEAD
            } // if div

            /*
             * do register targeting on mul, div and idiv instructions
             */

            if (type == me::Op::R_REAL32 || type == me::Op::R_REAL64)
                continue;

            if (ai->kind_ == '*' || ai->kind_ == '/')
            {
                if ( ai->kind_ == '*' && 
                        (type == me::Op::R_INT8  || type == me::Op::R_INT16 || 
                         type == me::Op::R_INT32 || type == me::Op::R_INT64) )
                {
                    // this is signed int = signed int * signed int
                    continue; // -> everything's fine
                }
                
                /*
                 * constrain properly
                 */
                ai->constrain();
                ai->arg_[0].constraint_ = RAX;
                ai->res_[0].constraint_ = RAX;

                // int8 and uint8 just go to ax/al/ah and not dl:al or something
                if (type != me::Op::R_INT8 && type != me::Op::R_UINT8)
                {
                    // RDX is destroyed in all cases
                    ai->res_.push_back( me::Res(function_->newSSAReg(type), 0, RDX) );

                    if (ai->kind_ == '/')
                    {
                        // no input reg may be RDX

                        // define a new Var which is initialized with undef
                        me::Var* dummy = function_->newSSAReg(type);
                        cfg_->instrList_.insert( iter->prev(), 
                            new me::AssignInstr('=', dummy, function_->newUndef(type)) );

                        // avoid that RDX is spilled here
                        ai->arg_.push_back( me::Arg(dummy, RDX) );

                        currentBB->value_->fixPointers();
                    }
                }
            }
        } // if AssignInstr
        else if ( typeid(*instr) == typeid(me::Store) )
        {
            me::Store* store = (me::Store*) instr;
            if ( typeid(*store->arg_[0].op_) != typeid(me::Reg) )
            {
                swiftAssert( typeid(*store->arg_[0].op_) == typeid(me::Const), 
                        "must be a Const here");
                me::Const* cst = (me::Const*) store->arg_[0].op_;

                // create var which will hold the constant, the assignment and insert
                me::Var* newVar = function_->newSSAReg(cst->type_);
                me::AssignInstr* newCopy = new me::AssignInstr('=', newVar, cst);
                cfg_->instrList_.insert( iter->prev(), newCopy );

                // substitute operand with newVar
                instr->arg_[0].op_ = newVar;

                currentBB->value_->fixPointers();
            }
        } // if Store
    } // for each instruction
}

std::string X64RegAlloc::reg2String(const me::Reg* reg)
{
    me::Op::Type type = reg->type_;
    int color = reg->color_;

    swiftAssert(color >= 0, "no valid color here");

    std::ostringstream oss;

    // prepend leading per cent sign for registers
    oss << '%';

    switch (type)
    {
        case me::Op::R_BOOL: 
        case me::Op::R_INT8: 
        case me::Op::R_UINT8: 
            switch (color)
            {
                case RAX: oss << "al"; break;
                case RBX: oss << "bl"; break;
                case RCX: oss << "cl"; break;
                case RDX: oss << "dl"; break;
                case RSP: oss << "spl"; break;
                case RBP: oss << "bpl"; break;
                case RSI: oss << "sil"; break;
                case RDI: oss << "dil"; break;
                case R8:  oss << "r8b"; break;
                case R9:  oss << "r9b"; break;
                case R10: oss << "r10b"; break;
                case R11: oss << "r11b"; break;
                case R12: oss << "r12b"; break;
                case R13: oss << "r13b"; break;
                case R14: oss << "r14b"; break;
                case R15: oss << "r15b"; break;

                default:
                    swiftAssert( false, "unreachable code" );
            }
            break;

        case me::Op::R_INT16: 
        case me::Op::R_UINT16: 
            switch (color)
            {
                case RAX: oss << "ax"; break;
                case RBX: oss << "bx"; break;
                case RCX: oss << "cx"; break;
                case RDX: oss << "dx"; break;
                case RSP: oss << "sp"; break;
                case RBP: oss << "bp"; break;
                case RSI: oss << "si"; break;
                case RDI: oss << "di"; break;
                case R8:  oss << "r8w"; break;
                case R9:  oss << "r9w"; break;
                case R10: oss << "r10w"; break;
                case R11: oss << "r11w"; break;
                case R12: oss << "r12w"; break;
                case R13: oss << "r13w"; break;
                case R14: oss << "r14w"; break;
                case R15: oss << "r15w"; break;

                default:
                    swiftAssert( false, "unreachable code" );
            }
            break;

        case me::Op::R_INT32:
        case me::Op::R_UINT32:
            switch (color)
            {
                case RAX: oss << "eax"; break;
                case RBX: oss << "ebx"; break;
                case RCX: oss << "ecx"; break;
                case RDX: oss << "edx"; break;
                case RSP: oss << "esp"; break;
                case RBP: oss << "ebp"; break;
                case RSI: oss << "esi"; break;
                case RDI: oss << "edi"; break;
                case R8:  oss << "r8d"; break;
                case R9:  oss << "r9d"; break;
                case R10: oss << "r10d"; break;
                case R11: oss << "r11d"; break;
                case R12: oss << "r12d"; break;
                case R13: oss << "r13d"; break;
                case R14: oss << "r14d"; break;
                case R15: oss << "r15d"; break;

                default:
                    swiftAssert( false, "unreachable code" );
            }
            break;

        case me::Op::R_INT64:
        case me::Op::R_UINT64:
            switch (color)
            {
                case RAX: oss << "rax"; break;
                case RBX: oss << "rbx"; break;
                case RCX: oss << "rcx"; break;
                case RDX: oss << "rdx"; break;
                case RSP: oss << "rsp"; break;
                case RBP: oss << "rbp"; break;
                case RSI: oss << "rsi"; break;
                case RDI: oss << "rdi"; break;
                case R8:  oss << "r8"; break;
                case R9:  oss << "r9"; break;
                case R10: oss << "r10"; break;
                case R11: oss << "r11"; break;
                case R12: oss << "r12"; break;
                case R13: oss << "r13"; break;
                case R14: oss << "r14"; break;
                case R15: oss << "r15"; break;

                default:
                    swiftAssert( false, "unreachable code" );
            }
            break;
            
        case me::Op::R_REAL32:
        case me::Op::R_REAL64:
            switch (color)
            {
                case XMM0: oss << "xmm0"; break;
                case XMM1: oss << "xmm1"; break;
                case XMM2: oss << "xmm2"; break;
                case XMM3: oss << "xmm3"; break;
                case XMM4: oss << "xmm4"; break;
                case XMM5: oss << "xmm5"; break;
                case XMM6: oss << "xmm6"; break;
                case XMM7: oss << "xmm7"; break;
                case XMM8: oss << "xmm8"; break;
                case XMM9: oss << "xmm9"; break;
                case XMM10: oss << "xmm10"; break;
                case XMM11: oss << "xmm11"; break;
                case XMM12: oss << "xmm12"; break;
                case XMM13: oss << "xmm13"; break;
                case XMM14: oss << "xmm14"; break;
                case XMM15: oss << "xmm15"; break;

                default:
                    swiftAssert( false, "unreachable code" );
            }
            break;

        default:
            swiftAssert( false, "unreachable code" );
    } // switch (type)

    return oss.str();
}

} // namespace be
