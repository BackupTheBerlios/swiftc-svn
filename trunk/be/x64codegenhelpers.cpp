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

#include <typeinfo>

#include "be/x64codegenhelpers.h"

#include "me/constpool.h"

#include "be/x64codegen.h"
#include "be/x64parser.h"
#include "be/x64regalloc.h"


namespace be {

std::string suffix(int type)
{
    switch (type)
    {
        case X64_BOOL:
        case X64_INT8:
        case X64_UINT8:
            return "b"; // byte

        case X64_INT16:
        case X64_UINT16:
            return "s"; // short

        case X64_INT32:
        case X64_UINT32:
            return "l"; // long

        case X64_INT64:
        case X64_UINT64:
            return "q"; // quad

        case X64_REAL32:
            return "ss";// scalar single
            
        case X64_REAL64:
            return "sd";// scalar double

        default:
            return "TODO";
    }
}

std::string reg2str(me::Reg* reg)
{
    if ( reg->isMem() )
    {
        std::ostringstream oss;
        oss << (reg->color_ + 1) * 16 << "(%rbp)";
        return oss.str();
    }
    else
        return X64RegAlloc::reg2String(reg);
}

std::string mcst2str(me::Const* cst)
{
    std::ostringstream oss;
    
    switch (cst->type_)
    {
        case me::Op::R_INT8:
        case me::Op::R_UINT8:
        {
            me::ConstPool::UInt8Map::iterator iter =
                me::constpool->uint8_.find(cst->value_.uint8_);

            if ( iter == me::constpool->uint8_.end() )
                me::constpool->insert(cst->value_.uint8_);

            oss << ".LC" << me::constpool->uint8_[cst->value_.uint8_];
            break;
        }
        case me::Op::R_INT16:
        case me::Op::R_UINT16:
        {
            me::ConstPool::UInt16Map::iterator iter =
                me::constpool->uint16_.find(cst->value_.uint16_);

            if ( iter == me::constpool->uint16_.end() )
                me::constpool->insert(cst->value_.uint16_);

            oss << ".LC" << me::constpool->uint16_[cst->value_.uint16_];
            break;
        }
        case me::Op::R_INT32:
        case me::Op::R_UINT32:
        case me::Op::R_REAL32:
        {
            me::ConstPool::UInt32Map::iterator iter =
                me::constpool->uint32_.find(cst->value_.uint32_);

            if ( iter == me::constpool->uint32_.end() )
                me::constpool->insert(cst->value_.uint32_);

            oss << ".LC" << me::constpool->uint32_[cst->value_.uint32_];
            break;
        }
        case me::Op::R_INT64:
        case me::Op::R_UINT64:
        case me::Op::R_REAL64:
        {
            me::ConstPool::UInt64Map::iterator iter =
                me::constpool->uint64_.find(cst->value_.uint64_);

            if ( iter == me::constpool->uint64_.end() )
                me::constpool->insert(cst->value_.uint64_);

            oss << ".LC" << me::constpool->uint64_[cst->value_.uint64_];
            break;
        }
        default:
            swiftAssert(false, "unreachable code");
    }

    return oss.str();
}

std::string sar_cst2str(int type)
{
    switch (type)
    {
        case X64_INT8 : return "$7";
        case X64_INT16: return "$15";
        case X64_INT32: return "$31";
        case X64_INT64: return "$63";
        default:
            swiftAssert(false, "unreachable code");
    }

    return "error";
}

std::string sgn_cst2str(me::Const* cst)
{
    switch (cst->type_)
    {
        case me::Op::R_INT8 : 
            if (cst->value_.int16_ < 0)  
                return "$255"; 
            else 
                return "$0";
        case me::Op::R_INT16: 
            if (cst->value_.int16_ < 0) 
                return  "$65535"; 
            else 
                return "$0";
        case me::Op::R_INT32: 
            if (cst->value_.int32_ < 0) 
                return  "$4294967295"; 
            else 
                return "$0";
        case me::Op::R_INT64: 
            if (cst->value_.int64_ < 0) 
                return  "$18446744073709551615"; 
            else 
                return "$0";
        default:
            swiftAssert(false, "unreachable code");
    }

    return "error";
}

std::string rdx2str(int type)
{
    switch (type)
    {
        case X64_INT8: 
        case X64_UINT8: 
            return "%dl";
        case X64_INT16: 
        case X64_UINT16: 
            return "%dx";
        case X64_INT32: 
        case X64_UINT32: 
            return "%edx";
        case X64_INT64: 
        case X64_UINT64: 
            return "%rdx";
        default:
            swiftAssert(false, "unreachable code");
    }

    return "error";
}

std::string div2str(int type)
{
    if (type == X64_INT8)
        return "idiv";

    swiftAssert(type == X64_UINT8, "must be uint8");
    return "div";
}

std::string cst2str(me::Const* cst)
{
    std::ostringstream oss;
    oss << '$';
    oss << cst->value_.uint64_;

    return oss.str();
}

std::string instr2str(me::AssignInstr* ai)
{
    switch (ai->kind_)
    {
        case '+': return "add";
        case '-': return "sub";
        case '*': return "mul";
        case '/': return "div";
        default:
            swiftAssert(false, "unreachable code");
    }

    return "";
}

std::string mul2str(int type)
{
    switch (type)
    {
        case X64_INT8:
        case X64_INT16:
        case X64_INT32:
        case X64_INT64:
            return "imul";
        case X64_UINT8:
        case X64_UINT16:
        case X64_UINT32:
        case X64_UINT64:
            return "mul";
        default:
            swiftAssert(false, "unreachable code"); 
    }

    return "error";
}

std::string cst_op_cst(me::AssignInstr* ai, me::Const* cst1, me::Const* cst2, bool mem /*= false*/)
{
    swiftAssert(cst1->type_ == cst2->type_, "types must be equal" );

    std::ostringstream oss;
    oss << '$';
    int kind = ai->kind_;

    me::Const::Value box;

    /*
     * signed integers
     */

    switch (cst1->type_)
    {

#define CONST_OP_CONST_CASE(type, member, box_member)\
    case me::Op::type :\
        switch (kind)\
        {\
            case '+': box.member = cst1->value_.member + cst2->value_.member; break;\
            case '-': box.member = cst1->value_.member - cst2->value_.member; break;\
            case '*': box.member = cst1->value_.member * cst2->value_.member; break;\
            case '/': box.member = cst1->value_.member / cst2->value_.member; break;\
            case me::AssignInstr::EQ:\
                if (cst1->value_.member == cst2->value_.member) return "$1"; else return "$0";\
            case me::AssignInstr::NE:\
                if (cst1->value_.member != cst2->value_.member) return "$1"; else return "$0";\
            case '<':\
                if (cst1->value_.member <  cst2->value_.member) return "$1"; else return "$0";\
            case me::AssignInstr::LE:\
                if (cst1->value_.member <= cst2->value_.member) return "$1"; else return "$0";\
            case '>':\
                if (cst1->value_.member >  cst2->value_.member) return "$1"; else return "$0";\
            case me::AssignInstr::GE:\
                if (cst1->value_.member >= cst2->value_.member) return "$1"; else return "$0";\
            default:\
                swiftAssert(false, "unreachable code"); \
        }\
        if (mem)\
        {\
            me::Const cst(cst1->type_);\
            cst.value_ = box;\
            return mcst2str(&cst);\
        }\
        oss << box.box_member;\
        return oss.str();\

        CONST_OP_CONST_CASE(R_INT8,  int8_,  int32_)
        CONST_OP_CONST_CASE(R_INT16, int16_, int16_)
        CONST_OP_CONST_CASE(R_INT32, int32_, int32_)
        CONST_OP_CONST_CASE(R_INT64, int64_, int64_)

        CONST_OP_CONST_CASE(R_UINT8,  uint8_ , uint32_) 
        CONST_OP_CONST_CASE(R_UINT16, uint16_, uint16_)
        CONST_OP_CONST_CASE(R_UINT32, uint32_, uint32_)
        CONST_OP_CONST_CASE(R_UINT64, uint64_, uint64_)

        CONST_OP_CONST_CASE(R_REAL32, real32_, uint32_)
        CONST_OP_CONST_CASE(R_REAL64, real64_, uint64_)

#undef CONST_OP_CONST_CASE

        default:
            swiftAssert(false, "unreachable code");
    }

    return "error";
}

std::string ccsuffix(me::AssignInstr* ai, int type, bool neg /*= false*/)
{
    int kind = ai->kind_;
    /*
     * for != and == just compare the bit pattern
     */
    if (kind == me::AssignInstr::EQ)
    {
        if (neg) 
            return "ne"; 
        else 
            return  "e";
    }
    else if (kind == me::AssignInstr::NE)
    {
        if (neg) 
            return  "e"; 
        else 
            return "ne";
    }

    /*
     * signed and unsigned integers must be distinguished
     */
    switch (type)
    {
        case X64_INT8:
        case X64_INT16:
        case X64_INT32:
        case X64_INT64:
            switch (kind)
            {
                case '<' : if (neg) return "nl";  else return "l";
                case '>' : if (neg) return "ng";  else return "g";
                case me::AssignInstr::LE: if (neg) return "nle"; else return "le";
                case me::AssignInstr::GE: if (neg) return "nge"; else return "ge";
                default:
                    swiftAssert(false, "unreachable code"); 
            }
        case X64_UINT8:
        case X64_UINT16:
        case X64_UINT32:
        case X64_UINT64:
        case X64_REAL32:
        case X64_REAL64:
            switch (kind)
            {
                case '<' : if (neg) return "nb";  else return "b";
                case '>' : if (neg) return "na";  else return "a";
                case me::AssignInstr::LE: if (neg) return "nbe"; else return "be";
                case me::AssignInstr::GE: if (neg) return "nae"; else return "ae";
                default:
                    swiftAssert(false, "unreachable code"); 
            }
        default:
            swiftAssert(false, "unreachable code"); 
    }

    return "error";
}

std::string jcc(me::BranchInstr* bi, bool neg /*= false*/)
{
    switch (bi->cc_)
    {
        case X64RegAlloc::C_EQ: if (neg) return "ne";  else return "e";
        case X64RegAlloc::C_NE: if (neg) return  "e";  else return "ne";
        case X64RegAlloc::C_L : if (neg) return "nl";  else return "l";
        case X64RegAlloc::C_LE: if (neg) return "nle"; else return "le";
        case X64RegAlloc::C_G:  if (neg) return "ng";  else return "g";
        case X64RegAlloc::C_GE: if (neg) return "nge"; else return "ge";
        case X64RegAlloc::C_B : if (neg) return "nb";  else return "b";
        case X64RegAlloc::C_BE: if (neg) return "nbe"; else return "be";
        case X64RegAlloc::C_A:  if (neg) return "na";  else return "a";
        case X64RegAlloc::C_AE: if (neg) return "nae"; else return "ae";
        default:
            swiftAssert(false, "unreachable code"); 
    }

    return "error";
}

std::string neg_mask(int type)
{
    std::ostringstream oss;
    oss << ".LCS";

    switch (type)
    {
        case X64_REAL32:
            oss << "32";
            break;
        case X64_REAL64:
            oss << "64";
            break;
        default:
            swiftAssert(false, "unreachable code"); 
    }

    return "error";
}

} // namespace be
