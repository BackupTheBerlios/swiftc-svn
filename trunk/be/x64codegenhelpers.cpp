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

std::string mcst2str(me::Const* cst)
{
    std::ostringstream oss;
    
    switch (cst->type_)
    {
        case me::Op::R_INT8:
        case me::Op::R_UINT8:
            oss << ".LC" << me::constpool->uint8_[cst->value_.uint8_];
            break;
        case me::Op::R_INT16:
        case me::Op::R_UINT16:
            oss << ".LC" << me::constpool->uint16_[cst->value_.uint16_];
            break;
        case me::Op::R_INT32:
        case me::Op::R_UINT32:
        case me::Op::R_REAL32:
            oss << ".LC" << me::constpool->uint32_[cst->value_.uint32_];
            break;
        case me::Op::R_INT64:
        case me::Op::R_UINT64:
        case me::Op::R_REAL64:
            oss << ".LC" << me::constpool->uint64_[cst->value_.uint64_];
            break;
        default:
            swiftAssert(false, "unreachable code");
    }

    return oss.str();
}

std::string cst2str(me::Const* cst)
{
    std::ostringstream oss;
    oss << '$';
    oss << cst->value_.uint64_;

    return oss.str();
}

std::string op2str(me::Op* op)
{
    if ( typeid(*op) == typeid(me::Const) )
        return cst2str( (me::Const*) op );
    else
    {
        swiftAssert( typeid(*op) == typeid(me::Reg), "must be a Reg" );
        return reg2str( (me::Reg*) op );
    }

    // avoid warning
    return "";
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

std::string const_add_or_mul_const(int instr, me::Const* cst1, me::Const* cst2)
{
    swiftAssert(cst1->type_ == cst2->type_, "types must be equal" );
    swiftAssert(cst1->type_ == me::Op::R_REAL32 
             || cst2->type_ == me::Op::R_REAL64 , "type must be a real");
    swiftAssert(instr == X64_ADD || instr == X64_MUL, "must be X64_ADD or X64_MUL");

    std::ostringstream oss;
    if (cst1->type_ == me::Op::R_REAL32)
    {
        if (instr == X64_ADD)
            oss << (cst1->value_.real32_ + cst2->value_.real32_);
        else
            oss << (cst1->value_.real32_ * cst2->value_.real32_);
    }
    else
    {
        if (instr == X64_ADD)
            oss << (cst1->value_.real64_ + cst2->value_.real64_);
        else
            oss << (cst1->value_.real64_ * cst2->value_.real64_);
    }

    return oss.str();
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

std::string div2str(int type)
{
    switch (type)
    {
        case X64_INT8:
        case X64_INT16:
        case X64_INT32:
        case X64_INT64:
            return "idiv";
        case X64_UINT8:
        case X64_UINT16:
        case X64_UINT32:
        case X64_UINT64:
            return "div";
        default:
            swiftAssert(false, "unreachable code"); 
    }

    return "error";
}

std::string const_cmp_const(me::AssignInstr* ai, me::Const* cst1, me::Const* cst2)
{
    swiftAssert(cst1->type_ == cst2->type_, "types must be equal" );

    int kind = ai->kind_;
    if (kind == me::AssignInstr::EQ)
    {
        if (cst1->value_.uint64_ == cst2->value_.uint64_)
            return "$1";
        else 
            return "$0";
    }

    if (kind == me::AssignInstr::NE)
    {
        if (cst1->value_.uint64_ != cst2->value_.uint64_)
            return "$1";
        else 
            return "$0";
    }

    switch (cst1->type_)
    {
        case me::Op::R_INT8:
            switch (kind)
            {
                case '<':
                    if (cst1->value_.int8_ <  cst2->value_.int8_) return "$1"; else return "$0";
                case me::AssignInstr::LE:
                    if (cst1->value_.int8_ <= cst2->value_.int8_) return "$1"; else return "$0";
                case '>':
                    if (cst1->value_.int8_ >  cst2->value_.int8_) return "$1"; else return "$0";
                case me::AssignInstr::GE:
                    if (cst1->value_.int8_ >= cst2->value_.int8_) return "$1"; else return "$0";
                default:
                    swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_INT16:
            switch (kind)
            {
                case '<':
                    if (cst1->value_.int16_ <  cst2->value_.int16_) return "$1"; else return "$0";
                case me::AssignInstr::LE:
                    if (cst1->value_.int16_ <= cst2->value_.int16_) return "$1"; else return "$0";
                case '>':
                    if (cst1->value_.int16_ >  cst2->value_.int16_) return "$1"; else return "$0";
                case me::AssignInstr::GE:
                    if (cst1->value_.int16_ >= cst2->value_.int16_) return "$1"; else return "$0";
                default:
                    swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_INT32:
            switch (kind)
            {
                case '<':
                    if (cst1->value_.int32_ <  cst2->value_.int32_) return "$1"; else return "$0";
                case me::AssignInstr::LE:
                    if (cst1->value_.int32_ <= cst2->value_.int32_) return "$1"; else return "$0";
                case '>':
                    if (cst1->value_.int32_ >  cst2->value_.int32_) return "$1"; else return "$0";
                case me::AssignInstr::GE:
                    if (cst1->value_.int32_ >= cst2->value_.int32_) return "$1"; else return "$0";
                default:
                    swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_INT64:
            switch (kind)
            {
                case '<':
                    if (cst1->value_.int64_ <  cst2->value_.int64_) return "$1"; else return "$0";
                case me::AssignInstr::LE:
                    if (cst1->value_.int64_ <= cst2->value_.int64_) return "$1"; else return "$0";
                case '>':
                    if (cst1->value_.int64_ >  cst2->value_.int64_) return "$1"; else return "$0";
                case me::AssignInstr::GE:
                    if (cst1->value_.int64_ >= cst2->value_.int64_) return "$1"; else return "$0";
                default:
                    swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_UINT8:
            switch (kind)
            {
                case '<':
                    if (cst1->value_.uint8_ <  cst2->value_.uint8_) return "$1"; else return "$0";
                case me::AssignInstr::LE:
                    if (cst1->value_.uint8_ <= cst2->value_.uint8_) return "$1"; else return "$0";
                case '>':
                    if (cst1->value_.uint8_ >  cst2->value_.uint8_) return "$1"; else return "$0";
                case me::AssignInstr::GE:
                    if (cst1->value_.uint8_ >= cst2->value_.uint8_) return "$1"; else return "$0";
                default:
                    swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_UINT16:
            switch (kind)
            {
                case '<':
                    if (cst1->value_.uint16_ <  cst2->value_.uint16_) return "$1"; else return "$0";
                case me::AssignInstr::LE:
                    if (cst1->value_.uint16_ <= cst2->value_.uint16_) return "$1"; else return "$0";
                case '>':
                    if (cst1->value_.uint16_ >  cst2->value_.uint16_) return "$1"; else return "$0";
                case me::AssignInstr::GE:
                    if (cst1->value_.uint16_ >= cst2->value_.uint16_) return "$1"; else return "$0";
                default:
                    swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_UINT32:
            switch (kind)
            {
                case '<':
                    if (cst1->value_.uint32_ <  cst2->value_.uint32_) return "$1"; else return "$0";
                case me::AssignInstr::LE:
                    if (cst1->value_.uint32_ <= cst2->value_.uint32_) return "$1"; else return "$0";
                case '>':
                    if (cst1->value_.uint32_ >  cst2->value_.uint32_) return "$1"; else return "$0";
                case me::AssignInstr::GE:
                    if (cst1->value_.uint32_ >= cst2->value_.uint32_) return "$1"; else return "$0";
                default:
                    swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_UINT64:
            switch (kind)
            {
                case '<':
                    if (cst1->value_.uint64_ <  cst2->value_.uint64_) return "$1"; else return "$0";
                case me::AssignInstr::LE:
                    if (cst1->value_.uint64_ <= cst2->value_.uint64_) return "$1"; else return "$0";
                case '>':
                    if (cst1->value_.uint64_ >  cst2->value_.uint64_) return "$1"; else return "$0";
                case me::AssignInstr::GE:
                    if (cst1->value_.uint64_ >= cst2->value_.uint64_) return "$1"; else return "$0";
                default:
                    swiftAssert(false, "unreachable code"); 
            }
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
