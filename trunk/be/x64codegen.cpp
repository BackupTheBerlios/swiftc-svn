#include "be/x64codegen.h"

#include <iostream>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

#include "be/x64parser.h"

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
    END
};

me::InstrBase* currentInstr;
Location location;
std::ofstream* x64_ofs = 0;
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
}

/*
 * further methods
 */

void X64CodeGen::process()
{
    ofs_ << '\n';
    ofs_ << *function_->id_ << ":\n";
    ofs_ << "\tenter $0, $32\n";

    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        me::InstrBase* instr = iter->value_;
        currentInstr = instr;
        location = INSTRUCTION;
        x64parse();
    }

    ofs_ << "\tleave\n\n";
    ofs_ << "\tret\n";
    ofs_ << '\n';
}

} // namespace be

//------------------------------------------------------------------------------

/*
 * lexer to parser interface
 */

void x64error(char *s)
{
    printf( "%s: could not parse '%s'\n", s, currentInstr->toString().c_str() );
}

int x64lex()
{
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
                x64lval.branch_ = (me::BranchInstr*) currentInstr;
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
                    case '-': return X64_SUB;
                    case '*': return X64_MUL;
                    case '/': return X64_DIV;
                    case me::AssignInstr::EQ: return X64_EQ;
                    case me::AssignInstr::NE: return X64_NE;
                    case '<': return X64_L;
                    case '>': return X64_G;
                    case me::AssignInstr::LE: return X64_LE;
                    case me::AssignInstr::GE: return X64_GE;
                    default:
                        swiftAssert(false, "unreachable code");
                }
            }
            else if ( instrTypeId == typeid(me::PhiInstr) )
            {
                //me::PhiInstr* phi = (me::PhiInstr*) currentInstr;
                // TODO
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
            me::Op::Type type =  currentInstr->arg_[0].op_->type_;
            switch (type)
            {
                case me::Op::R_BOOL:  return X64_BOOL;

                case me::Op::R_INT8:  return X64_INT8;
                case me::Op::R_INT16: return X64_INT16;
                case me::Op::R_INT32: return X64_INT32;
                case me::Op::R_INT64: return X64_INT64;
                case me::Op::R_SAT8:  return X64_SAT8;
                case me::Op::R_SAT16: return X64_SAT16;

                case me::Op::R_UINT8:  return X64_UINT8;
                case me::Op::R_UINT16: return X64_UINT16;
                case me::Op::R_UINT32: return X64_UINT32;
                case me::Op::R_UINT64: return X64_UINT64;
                case me::Op::R_USAT8:  return X64_USAT8;
                case me::Op::R_USAT16: return X64_USAT16;

                case me::Op::R_REAL32: return X64_REAL32;
                case me::Op::R_REAL64: return X64_REAL64;
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
            else 
            {
                swiftAssert( opTypeId == typeid(me::Reg), "must be a Reg" );
                me::Reg* reg = (me::Reg*) op;
                x64lval.reg_ = reg;

                if ( !currentInstr->res_.empty() && currentInstr->res_[0].reg_->color_ == reg->color_ )
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
                swiftAssert( opTypeId == typeid(me::Reg), "must be a Reg here");
                me::Reg* reg = (me::Reg*) op;
                x64lval.reg_ = reg;

                if ( currentInstr->res_[0].reg_->color_ == reg->color_ )
                    lastOp = X64_REG_1;
                else if ( lastOp == X64_REG_2 && ((me::Reg*) currentInstr->arg_[0].op_)->color_ == reg->color_ )
                    lastOp = X64_REG_2;
                else if (lastOp == X64_REG_1)
                    lastOp = X64_REG_2;
                else
                    return X64_REG_3;
            }
            return lastOp;
        }
        case END:
            return 0;
    }

    return 0;
}
