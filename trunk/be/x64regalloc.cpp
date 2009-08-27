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

#include "me/cfg.h"
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
 * statics
 */

int X64RegAlloc::intRegs[NUM_INT_REGS] = {RDI, RSI, RDX, RCX, R8, R9};
int X64RegAlloc::xmmRegs[NUM_XMM_REGS] = {XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7};

int X64RegAlloc::intReturnRegs[NUM_INT_RETURN_REGS] = {RAX, RDX};
int X64RegAlloc::xmmReturnRegs[NUM_XMM_RETURN_REGS] = {XMM0, XMM1};

int X64RegAlloc::intClobberedRegs[NUM_INT_CLOBBERED_REGS] = 
    {RAX, RDX, RCX, RSI, RDI, R8, R9, R10, R11};

int X64RegAlloc::xmmClobberedRegs[NUM_XMM_CLOBBERED_REGS] = 
    {XMM0, XMM1,  XMM2,  XMM3,  XMM4,  XMM5,  XMM6,  XMM7, 
     XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15};

me::Colors* X64RegAlloc::intColors_ = 0;
me::Colors* X64RegAlloc::xmmColors_ = 0;

/*
 * constructor and destructor
 */

X64RegAlloc::X64RegAlloc(me::Function* function)
    : me::RegAlloc(function)
{
    if (!intColors_)
    {
        intColors_ = new me::Colors();
        xmmColors_ = new me::Colors();

        me::Colors intColors;
        for (int i = R0; i <= R15; ++i)
            intColors_->insert(i);

        // do not use the stack pointer 
        intColors_->erase(RSP);
        //intColors_->erase(RBP);

        me::Colors xmmColors;
        for (int i = XMM0; i <= XMM15; ++i)
            xmmColors_->insert(i);
    }
}

void X64RegAlloc::destroyColors()
{
    delete intColors_;
    delete xmmColors_;
}

/*
 * further methods
 */

const me::Colors* X64RegAlloc::getIntColors()
{
    return intColors_;
}

const me::Colors* X64RegAlloc::getXmmColors()
{
    return xmmColors_;
}

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

    me::Spiller( function_, intColors_->size(), INT_TYPE_MASK ).process();

    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * spill XMM registers
     */

    me::Spiller( function_, xmmColors_->size(), XMM_TYPE_MASK ).process();

    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();

    /*
     * copy insertion
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

    /*
     * live range splitting
     */

    me::LiveRangeSplitting(function_).process();
    // recalulate def-use and liveness stuff
    me::DefUseCalc(function_).process();
    me::LivenessAnalysis(function_).process();
    
    /*
     * coloring
     */

    // general purpose registers
    me::Coloring(function_, INT_TYPE_MASK, *intColors_).process();

    // XMM registers
    me::Coloring(function_, XMM_TYPE_MASK, *xmmColors_).process(); 

    // color spill slots
    me::Coloring(function_, QUADWORD_TYPE_MASK, X64::QUADWORDS).process();
    me::Coloring(function_,  OCTWORD_TYPE_MASK, X64:: OCTWORDS).process();
}

bool X64RegAlloc::arg2Reg(me::InstrNode* iter, size_t i)
{
    me::InstrBase* instr = iter->value_;
    me::Op* op = instr->arg_[i].op_;

    if ( typeid(*op) == typeid(me::Reg*) )
        return false;

    // create var which will holds the arg, the assignment and insert
    me::Var* newVar = function_->newSSAReg(op->type_);
    me::AssignInstr* newCopy = new me::AssignInstr('=', newVar, op);
    cfg_->instrList_.insert( iter->prev(), newCopy );

    // substitute operand with newVar
    instr->arg_[i].op_ = newVar;

    return true;
}

void X64RegAlloc::registerTargeting()
{
    me::BBNode* currentBB;

    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        me::InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(me::LabelInstr) )
            currentBB = cfg_->labelNode2BBNode_[iter];
        else if ( typeid(*instr) == typeid(me::BranchInstr) )
            targetBranchInstr(iter, currentBB);
        else if ( typeid(*instr) == typeid(me::SetParams) )
            targetSetParams(iter, currentBB);
        else if ( typeid(*instr) == typeid(me::SetResults) )
            targetSetResults(iter, currentBB);
        else if ( typeid(*instr) == typeid(me::AssignInstr) )
            targetAssignInstr(iter, currentBB);
        else if ( typeid(*instr) == typeid(me::Store) )
            targetStore(iter, currentBB);
        else if ( dynamic_cast<me::CallInstr*>(instr) )
            targetCallInstr(iter, currentBB);
    } // for each instruction
}

void X64RegAlloc::targetAssignInstr(me::InstrNode* iter, me::BBNode* currentBB)
{
    me::AssignInstr* ai = (me::AssignInstr*) iter->value_;
    swiftAssert( ai->res_.size() >= 1, "one or more results must be here" );
    swiftAssert( ai->arg_.size() == 1 || ai->arg_.size() == 2,
            "one or two args must be here" );

    if ( ai->isComparison() )
        return;

    me::Op::Type type = ai->res_[0].var_->type_;

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

        if (opType2 == me::InstrBase::VARIABLE_DEAD)
        {
            if (opType1 == me::InstrBase::LITERAL)
            {
                swiftAssert( typeid(*ai->arg_[0].op_) == typeid(me::Const), 
                        "must be a Const here");

                swiftAssert( arg2Reg(iter, 0), "must be true" );
                currentBB->value_->fixPointers();
            }
            else if (opType1 == me::InstrBase::VARIABLE)
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

    if ( ai->res_[0].var_->isReal() || ai->res_[0].var_->isSimd() )
        return;

    if (ai->kind_ == '*' || ai->kind_ == '/')
    {
        if ( ai->kind_ == '*' && 
                (type == me::Op::R_INT8  || type == me::Op::R_INT16 || 
                    type == me::Op::R_INT32 || type == me::Op::R_INT64) )
        {
            // this is signed int = signed int * signed int
            return; // -> everything's fine
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
}

void X64RegAlloc::targetBranchInstr(me::InstrNode* iter, me::BBNode* currentBB)
{
    me::BranchInstr* bi = (me::BranchInstr*) iter->value_;

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
                        return;

                    // check whether this is the only use
                    if (var->uses_.size() == 1)
                    {
                        // do not color this var
                        var->color_ = me::Var::DONT_COLOR;
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

            return;
        } // if var defined in previous instruction
    } // if bi->getOP() is a Var
    else 
    {
        swiftAssert( typeid(*bi->getOp()) == typeid(me::Const),
                "must be a me::Const here" );
    }
}

void X64RegAlloc::targetStore(me::InstrNode* iter, me::BBNode* currentBB)
{
    if ( arg2Reg(iter, 0 ) )
        currentBB->value_->fixPointers();
}

void X64RegAlloc::targetSetParams(me::InstrNode* iter, me::BBNode* currentBB)
{
    me::SetParams* sp = (me::SetParams*) iter->value_;

    if ( !sp->res_.empty() )
        sp->constrain();

    int intCounter = 0;
    int xmmCounter = 0;

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
            case me::Op::R_PTR:
                sp->res_[i].constraint_ = intRegs[intCounter++];
                break;

            case me::Op::R_REAL32:
            case me::Op::R_REAL64:
            case me::Op::S_REAL32:
            case me::Op::S_REAL64:
            case me::Op::S_INT8:
            case me::Op::S_INT16:
            case me::Op::S_INT32:
            case me::Op::S_INT64:
            case me::Op::S_SAT8:
            case me::Op::S_SAT16:
            case me::Op::S_UINT8:
            case me::Op::S_UINT16:
            case me::Op::S_UINT32:
            case me::Op::S_UINT64:
            case me::Op::S_USAT8:
            case me::Op::S_USAT16:
                sp->res_[i].constraint_ = xmmRegs[xmmCounter++];
                break;

            default:
                swiftAssert( false, "unreachable code" );
        }
    }
}

void X64RegAlloc::targetCallInstr(me::InstrNode* iter, me::BBNode* currentBB)
{
    me::CallInstr* ci = (me::CallInstr*) iter->value_;

    if ( ci->res_.size() + ci->arg_.size() == 0 )
        return;

    ci->constrain();

    size_t intReturnCounter = 0;
    size_t xmmReturnCounter = 0;

    // for each result
    for (size_t i = 0; i < ci->res_.size(); ++i)
    {
        if ( ci->res_[i].var_->isReal() )
            ci->res_[i].constraint_ = xmmReturnRegs[xmmReturnCounter++];
        else
            ci->res_[i].constraint_ = intReturnRegs[intReturnCounter++];
    }

    bool fixPointers = false;
    size_t intCounter = 0;
    size_t xmmCounter = 0;

    // for each arg
    for (size_t i = 0; i < ci->arg_.size(); ++i)
    {
        fixPointers |= arg2Reg(iter, i);

        if ( ci->arg_[i].op_->isReal() )
            ci->arg_[i].constraint_ = xmmRegs[xmmCounter++];
        else
            ci->arg_[i].constraint_ = intRegs[intCounter++];
    }

    /*
     * if this is a vararg function add an additional dummy arg with 
     * RAX constraint which will hold the number of sse regs used
     */
    if ( ci->isVarArg() )
    {
        me::Var* newVar = function_->newSSAReg(me::Op::R_INT64);
        ci->arg_.push_back( me::Arg(newVar, RAX) );
    }

    // add clobbered int registers as dummy results
    for (size_t i = intReturnCounter; i < NUM_INT_CLOBBERED_REGS; ++i)
    {
        me::Var* dummy = function_->newSSAReg(me::Op::R_INT64);
        ci->res_.push_back( me::Res(dummy, 0, intClobberedRegs[i]) );
    }

    // add clobbered xmm registers as dummy results
    for (size_t i = xmmReturnCounter; i < NUM_XMM_CLOBBERED_REGS; ++i)
    {
        me::Var* dummy = function_->newSSAReg(me::Op::R_REAL64);
        ci->res_.push_back( me::Res(dummy, 0, xmmClobberedRegs[i]) );
    }

    if (fixPointers)
        currentBB->value_->fixPointers();
}

void X64RegAlloc::targetSetResults(me::InstrNode* iter, me::BBNode* currentBB)
{
    me::SetResults* sr = (me::SetResults*) iter->value_;

    swiftAssert( !sr->arg_.empty(), "must not be empty" );
    sr->constrain();

    // TODO only one result supported
    if ( sr->arg_[0].op_->isReal() )
        sr->arg_[0].constraint_ = XMM0;
    else
        sr->arg_[0].constraint_ = RAX;
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
        case me::Op::R_PTR:
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
        case me::Op::S_REAL32:
        case me::Op::S_REAL64:
        case me::Op::S_INT8:
        case me::Op::S_INT16:
        case me::Op::S_INT32:
        case me::Op::S_INT64:
        case me::Op::S_SAT8:
        case me::Op::S_SAT16:
        case me::Op::S_UINT8:
        case me::Op::S_UINT16:
        case me::Op::S_UINT32:
        case me::Op::S_UINT64:
        case me::Op::S_USAT8:
        case me::Op::S_USAT16:
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
