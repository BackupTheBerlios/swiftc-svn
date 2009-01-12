#include "be/x64codegen.h"

#include <iostream>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

#include "be/x64parser.h"


//------------------------------------------------------------------------------

/*
 * lexer to parser interface
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
                return LABEL;
            }
            else if (instrTypeId == typeid(me::GotoInstr) )
            {
                x64lval.goto_ = (me::GotoInstr*) currentInstr;
                return GOTO;
            }
            else if (instrTypeId == typeid(me::BranchInstr) )
            {
                x64lval.branch_ = (me::BranchInstr*) currentInstr;
                return BRANCH;
            }
            else if ( instrTypeId == typeid(me::AssignInstr) )
            {
                me::AssignInstr* ai = (me::AssignInstr*) currentInstr;
                x64lval.assign_ = ai;

                switch (ai->kind_)
                {
                    case '=': return MOV;
                    case '+': return ADD;
                    case '-': return SUB;
                    case '*': return MUL;
                    case '/': return DIV;
                    default:
                        return 0;
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
                return NOP;
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
                case me::Op::R_BOOL:  return BOOL;

                case me::Op::R_INT8:  return INT8;
                case me::Op::R_INT16: return INT16;
                case me::Op::R_INT32: return INT32;
                case me::Op::R_INT64: return INT64;
                case me::Op::R_SAT8:  return SAT8;
                case me::Op::R_SAT16: return SAT16;

                case me::Op::R_UINT8:  return UINT8;
                case me::Op::R_UINT16: return UINT16;
                case me::Op::R_UINT32: return UINT32;
                case me::Op::R_UINT64: return UINT64;
                case me::Op::R_USAT8:  return USAT8;
                case me::Op::R_USAT16: return USAT16;

                case me::Op::R_REAL32: return REAL32;
                case me::Op::R_REAL64: return REAL64;
            }
        }
        case OP1:
        {
            if ( currentInstr->arg_.empty() )
                return 0;

            location = OP2;

            swiftAssert( typeid(*currentInstr) == typeid(me::AssignInstr),
                    "must be an AssignInstr here");
            me::AssignInstr* ai = (me::AssignInstr*) currentInstr;

            me::Op* op = ai->arg_[0].op_;
            const std::type_info& opTypeId = typeid(*op);

            if ( opTypeId == typeid(me::Undef) )
            {
                x64lval.undef_ = (me::Undef*) op;
                lastOp = UNDEF;
            }
            else if ( opTypeId == typeid(me::Const) )
            {
                x64lval.const_ = (me::Const*) op;
                lastOp = CONST;
            }
            else 
            {
                swiftAssert( opTypeId == typeid(me::Reg), "must be a Reg" );
                me::Reg* reg = (me::Reg*) op;
                x64lval.reg_ = reg;

                if ( !ai->res_.empty() && ai->res_[0].reg_->color_ == reg->color_ )
                    lastOp = REG_1;
                else
                    lastOp = REG_2;
            }
            return lastOp;
        }
        case OP2:
        {
            location = END;

            if ( currentInstr->arg_.size() < 2 )
                return 0;

            swiftAssert( typeid(*currentInstr) == typeid(me::AssignInstr),
                    "must be an AssignInstr here");
            me::AssignInstr* ai = (me::AssignInstr*) currentInstr;

            me::Op* op = ai->arg_[1].op_;
            const std::type_info& opTypeId = typeid(*op);

            if ( opTypeId == typeid(me::Undef) )
            {
                x64lval.undef_ = (me::Undef*) op;
                lastOp = UNDEF;
            }
            else if ( opTypeId == typeid(me::Const) )
            {
                x64lval.const_ = (me::Const*) op;
                lastOp = CONST;
            }
            else 
            {
                swiftAssert( opTypeId == typeid(me::Reg), "must be a Reg here");
                me::Reg* reg = (me::Reg*) op;
                x64lval.reg_ = reg;

                if ( ai->res_[0].reg_->color_ == reg->color_ )
                    lastOp = REG_1;
                else if ( lastOp == REG_2 && ((me::Reg*) ai->arg_[0].op_)->color_ == reg->color_ )
                    lastOp = REG_2;
                else if (lastOp == REG_1)
                    lastOp = REG_2;
                else
                    return REG_3;
            }
            return lastOp;
        }
        case END:
            return 0;
    }

    return ERROR;
}

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

//void X64CodeGen::genAsmInstr(me::InstrBase* instr)
//{
    //const std::type_info& instrTypeId = typeid(*instr);

    //if ( instrTypeId == typeid(me::LabelInstr) )
        //ofs_ << instr->toString() << ":\n";
    //else if (instrTypeId == typeid(me::GotoInstr) )
    //{
        //me::GotoInstr* gi = (me::GotoInstr*) instr;
        //ofs_ << "\tjmp " << gi->label()->toString() << '\n';
    //}
    //else if (instrTypeId == typeid(me::BranchInstr) )
    //{
        //me::BranchInstr* bi = (me::BranchInstr*) instr;
        //std::string strOp = op2string(bi->arg_[0].op_);
        //ofs_ << "\torb " << strOp << ", " <<  strOp << '\n';
        //ofs_ << "\tjz " << bi->falseLabel()->toString() << '\n';
        //ofs_ << "\tjmp " << bi->trueLabel()->toString() << '\n';
    //}
    //else if ( instrTypeId == typeid(me::AssignInstr) )
    //{
        //me::Op::Type resType = instr->res_[0].reg_->type_;

        //if (resType == me::Op::R_REAL32 || resType == me::Op::R_REAL64)
            //genFloatingPointInstr( (me::AssignInstr*) instr );
        //else
            //genGeneralPurposeInstr( (me::AssignInstr*) instr );
    //}
//}

//enum
//{
    //CONST,      // a = CONST
    //EQUAL,   // r1 = x # x
    //SIMPLE, // r1 = r1/const # x
    //COMMUTATIVE_SIMPLE // r1 = x # r1/const

//};

//void X64CodeGen::checkInstrComplexity(me::Op* op1, me::Op2)
//{
    //me::InstrBase::OpType opType1 = ai->getOpType(0);
    //me::InstrBase::OpType opType2 = ai->getOpType(1);

    //if (opType1 == me::InstrBase::CONST && opType2 == me::InstrBase::CONST)
        //return CONST;

    //if (op1 == op2)
        //return SIMPLE;

//}

//void X64CodeGen::genGeneralPurposeInstr(me::InstrBase* instr)
//{
    //me::AssignInstr* ai = (me::AssignInstr*) instr;

    //me::Reg* res = instr->res_[0].reg_;
    //me::Op*  op1 = instr->arg_[0].op_;
    //me::Op*  op2 = instr->arg_[1].op_;

    //switch (ai->kind_)
    //{
        //case '=':
            //// ignore undefined inits
            //if ( typeid(*op1) == typeid(me::Undef) )
                //break;

            //// ignore r = r
            //if ( typeid(*op1) == typeid(me::Reg) && res->color_ == ((me::Reg*) op1)->color_)
                //break;

            //ofs_ << "\tmov" << type2postfix(res->type_) << ' ';
            //ofs_ << op2string(op1) << ", ";
            //ofs_ << X64RegAlloc::reg2String(res) << "\n";
            //break;

        //case '+':
            //ofs_ << "\tadd" << type2postfix(res->type_) << " %eax, %eax\n";
            //break;

        //default:
            //// TODO
            //break;
    //}
//}

//void X64CodeGen::genFloatingPointInstr(me::InstrBase* instr)
//{
    //me::AssignInstr* ai = (me::AssignInstr*) instr;

    //me::Reg* res = instr->res_[0].reg_;
    //me::Op*  op1 = instr->arg_[0].op_;
    //me::Op*  op2 = instr->arg_[1].op_;

    //switch (ai->kind_)
    //{
        //case '=':
            //// ignore undefined inits
            //if ( typeid(*op1) == typeid(me::Undef) )
                //break;

            //// ignore r = r
            //if ( typeid(*op1) == typeid(me::Reg) && res->color_ == ((me::Reg*) op1)->color_)
                //break;

            //ofs_ << "\tmov" << type2postfix(res->type_) << ' ';
            //ofs_ << op2string(op1) << ", ";
            //ofs_ << X64RegAlloc::reg2String(res) << "\n";
            //break;

        //case '+':
            //ofs_ << "\tadd" << type2postfix(res->type_) << " %xmm1, %xmm0\n";
            //break;

        //default:
            //// TODO
            //break;
    //}
//}

} // namespace be
