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

#include "be/x64lexer.h"

#include <typeinfo>

#include "me/op.h"
#include "me/ssa.h"

#include "be/x64parser.h"
#include "be/x64typeconv.h"

using namespace be;

/*
 * globals
 */

me::InstrNode* currentInstrNode = 0;
std::ofstream* x64_ofs = 0;
me::StackLayout* x64_stacklayout = 0;

namespace {
    int pos = -1;
    me::Reg* regs[5];
    int reg_nr[5];
    size_t regs_index = 0;
}

void x64error(const char *s)
{
    printf( "%s: could not parse '%s'\n", s, currentInstrNode->value_->toString().c_str() );
}

int findOutOp(me::Op* op)
{
    const std::type_info& opTypeId = typeid(*op);
    me::Reg* reg;

    if ( opTypeId == typeid(me::Undef) )
    {
        x64lval.undef_ = (me::Undef*) op;
        return X64_UNDEF;
    }
    else if ( opTypeId == typeid(me::Const) )
    {
        x64lval.const_ = (me::Const*) op;
        return X64_CONST;
    }
    else if ( opTypeId == typeid(me::MemVar) )
    {
        x64lval.memVar_ = (me::MemVar*) op;
        return X64_MEM_VAR;
    }
    else 
    {
        swiftAssert( typeid(*op) == typeid(me::Reg), "must be a Reg" );
        reg = (me::Reg*) op;
        x64lval.reg_ = reg;

        if ( reg->isSpilled() )
        {
            return X64_REG_SPILLED;
        }

        regs[regs_index++] = x64lval.reg_;
    }

    if (regs_index == 1)
    {
        reg_nr[0] = X64_REG_1;
        return X64_REG_1;
    }

    int highest = X64_REG_1;
    for (size_t i = 0; i < regs_index - 1; ++i) // omit the newly added reg
    {
        me::Reg* iReg = regs[i];

        if (iReg->color_ == reg->color_)
        {
            reg_nr[regs_index] = reg_nr[i];
            return reg_nr[i];
        }
        else
        {
            if (iReg->color_ > highest)
                highest = iReg->color_;
        }
    }

    return highest + 1;
}

#define LEX_END default: pos = -1; regs_index = 0; return 0;

int x64lex()
{
    // increase pos
    ++pos;

    me::InstrBase* currentInstr = currentInstrNode->value_;
    const std::type_info& instrTypeId = typeid(*currentInstr);

    if ( instrTypeId == typeid(me::LabelInstr) )
    {
        switch (pos)
        {
            case 0:
                x64lval.label_ = (me::LabelInstr*) currentInstr;
                return X64_LABEL;
            LEX_END
        }
    }
    else if (instrTypeId == typeid(me::GotoInstr) )
    {
        switch (pos)
        {
            case 0:
                x64lval.goto_ = (me::GotoInstr*) currentInstr;
                return X64_GOTO;
            LEX_END
        }
    }
    else if (instrTypeId == typeid(me::BranchInstr) )
    {
        me::BranchInstr* bi = (me::BranchInstr*) currentInstr;

        switch (pos)
        {
            case 0:
            {
                x64lval.branch_ = bi;

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
            case 1:
                return meType2beType( bi->getOp()->type_ );
            case 2:
                return findOutOp( bi->getOp() );
            LEX_END
        }
    }
    else if ( instrTypeId == typeid(me::AssignInstr) )
    {
        me::AssignInstr* ai = (me::AssignInstr*) currentInstr;

        switch (pos)
        {
            case 0:
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
            case 1:
                return meType2beType( ai->arg_[0].op_->type_ );
            case 2:
                return findOutOp( ai->res_[0].var_ );
            case 3:
                return findOutOp( ai->arg_[0].op_ );
            case 4:
                if ( ai->isUnary() )
                {
                    pos = -1;
                    regs_index = 0;
                    return 0;
                }
                return findOutOp( ai->arg_[1].op_ );
            LEX_END
        }
    }
    //else if ( instrTypeId == typeid(me::Cast) )
    //{
        //me::Cast* cast = (me::Cast*) currentInstr;

        //switch (pos)
        //{
            //case 0:
                //x64lval.cast_ = cast;
                //return X64_CAST;
            //case 1:
                //return meType2beType( cast->res_[0].var_->type_ );
            //case 2:
                //return findOutOp( cast->res_[0].var_ );
            //case 3:
                //return meType2beType( cast->arg_[0].op_->type_ );
            //case 4:
                //return findOutOp( cast->arg_[0].op_ );
            //LEX_END
        //}
    //}
    else if ( instrTypeId == typeid(me::Spill) )
    {
        me::Spill* spill = (me::Spill*) currentInstr;

        switch (pos)
        {
            case 0:
                x64lval.spill_ = spill;
                return X64_SPILL;
            case 1:
                return meType2beType( spill->arg_[0].op_->type_ );
            case 2:
                return findOutOp( spill->res_[0].var_ );
            case 3:
                return findOutOp( spill->arg_[0].op_ );
            LEX_END
        }
    }
    else if ( instrTypeId == typeid(me::Reload) )
    {
        me::Reload* reload = (me::Reload*) currentInstr;
        switch (pos)
        {
            case 0:
                x64lval.reload_ = reload;
                return X64_RELOAD;
            case 1:
                return meType2beType( reload->arg_[0].op_->type_ );
            case 2:
                return findOutOp( reload->res_[0].var_ );
            case 3:
                return findOutOp( reload->arg_[0].op_ );
            LEX_END
        }
    }
    else if ( instrTypeId == typeid(me::Load) )
    {
        me::Load* load = (me::Load*) currentInstr;

        switch (pos)
        {
            case 0:
                x64lval.load_ = load;
                return X64_LOAD;
            case 1:
                return meType2beType( load->res_[0].var_->type_ );
            case 2:
                return findOutOp( load->res_[0].var_ );
            case 3:
                return findOutOp( load->arg_[0].op_ );
            case 4:
                if ( load->arg_.size() == 1 )
                {
                    pos = -1;
                    regs_index = 0;
                    return 0;
                }
                return findOutOp( load->arg_[1].op_ );
            LEX_END
        }
    }
    else if ( instrTypeId == typeid(me::Store) )
    {
        me::Store* store = (me::Store*) currentInstr;

        switch (pos)
        {
            case 0:
                x64lval.store_ = store;
                return X64_STORE;
            case 1:
                return meType2beType( store->arg_[0].op_->type_ );
            case 2:
                return findOutOp( store->arg_[0].op_ );
            case 3:
                return findOutOp( store->arg_[1].op_ );
            case 4:
                if ( store->arg_.size() == 2 )
                {
                    pos = -1;
                    regs_index = 0;
                    return 0;
                }
                return findOutOp( store->arg_[2].op_ );
            LEX_END
        }
    }
    else if ( instrTypeId == typeid(me::LoadPtr) )
    {
        me::LoadPtr* loadPtr = (me::LoadPtr*) currentInstr;

        switch (pos)
        {
            case 0:
                x64lval.loadPtr_ = loadPtr;
                return X64_LOAD_PTR;
            case 1:
                return findOutOp( loadPtr->res_[0].var_ );
            case 2:
                return findOutOp( loadPtr->arg_[0].op_ );
            LEX_END
        }
    }
    else if ( dynamic_cast<me::CallInstr*>(currentInstr) )
    {
        me::CallInstr* call = (me::CallInstr*) currentInstr;

        switch (pos)
        {
            case 0:
                x64lval.call_ = call;
                return X64_CALL;
            LEX_END
        }
    }
    else
    {
        swiftAssert( instrTypeId == typeid(me::NOP), "must be a NOP" );

        switch (pos)
        {
            case 0:
                return X64_NOP;
            LEX_END
        }
    }
}

#undef LEX_END

