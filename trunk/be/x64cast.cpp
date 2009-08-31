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

#include "be/x64cast.h"

#include "me/op.h"
#include "me/ssa.h"

#include "be/x64codegenhelpers.h"

namespace be {

std::string cast2str(me::Cast* cast)
{
    me::Reg* dst = (me::Reg*) cast->res_[0].var_;
    me::Reg* src = (me::Reg*) cast->arg_[0].op_;

    me::Reg* scratch = cast->res_.size() == 2 
                     ? (me::Reg*) cast->res_[1].var_
                     : 0;

    std::ostringstream oss;

    if (dst->type_ == src->type_)
    {
        // this is an ordinary move
        if (dst->color_ == src->color_)
            return ""; // nothing to do

        me::Op::Type type = dst->type_;

        oss << mnemonic("\tmov", type) << '\t' << reg2str(src) << reg2str(dst);
        return oss.str();
    }
    // else -> proper cast

    switch (src->type_)
    {
        /*
         * signed integer src type
         */

        case me::Op::R_INT8:
            switch (dst->type_)
            {
                case me::Op::R_INT8:
                case me::Op::R_UINT8:
                    return ""; // nothing to do
                case me::Op::R_INT16:
                case me::Op::R_INT32:
                case me::Op::R_INT64:
                case me::Op::R_UINT16:
                case me::Op::R_UINT32:
                case me::Op::R_UINT64:
                    oss << "\tmovsb" << suffix(dst->type_);
                    oss << "\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL32:
                    swiftAssert(scratch, "must be valid here");
                    oss << "\tmovsbl\t" << reg2str(src) << ", " << reg2str(scratch) << '\n';
                    oss << "\tcvtsi2ss\t" << reg2str(scratch) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL64:
                    swiftAssert(scratch, "must be valid here");
                    oss << "\tmovsbl\t" << reg2str(src) << ", " << reg2str(scratch) << '\n';
                    oss << "\tcvtsi2sd\t" << reg2str(scratch) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                default: swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_INT16:
            switch (dst->type_)
            {
                case me::Op::R_INT8:
                case me::Op::R_UINT8:
                case me::Op::R_INT16:
                case me::Op::R_UINT16:
                    return ""; // nothing to do
                case me::Op::R_INT32:
                case me::Op::R_INT64:
                case me::Op::R_UINT32:
                case me::Op::R_UINT64:
                    oss << "\tmovsw" << suffix(dst->type_);
                    oss << "\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL32:
                    swiftAssert(scratch, "must be valid here");
                    oss << "\tmovswl\t" << reg2str(src) << ", " << reg2str(scratch) << '\n';
                    oss << "\tcvtsi2ss\t" << reg2str(scratch) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL64:
                    swiftAssert(scratch, "must be valid here");
                    oss << "\tmovswl\t" << reg2str(src) << ", " << reg2str(scratch) << '\n';
                    oss << "\tcvtsi2sd\t" << reg2str(scratch) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                default: swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_INT32:
            switch (dst->type_)
            {
                case me::Op::R_INT8:
                case me::Op::R_UINT8:
                case me::Op::R_INT16:
                case me::Op::R_UINT16:
                case me::Op::R_INT32:
                case me::Op::R_UINT32:
                    return ""; // nothing to do
                case me::Op::R_INT64:
                case me::Op::R_UINT64:
                    oss << "\tmovsl" << suffix(dst->type_);
                    oss << "\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL32:
                    swiftAssert(!scratch, "must be invalid here");
                    oss << "\tcvtsi2ss\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL64:
                    swiftAssert(!scratch, "must be invalid here");
                    oss << "\tcvtsi2sd\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                default: swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_INT64:
            switch (dst->type_)
            {
                case me::Op::R_INT8:
                case me::Op::R_UINT8:
                case me::Op::R_INT16:
                case me::Op::R_UINT16:
                case me::Op::R_INT32:
                case me::Op::R_UINT32:
                case me::Op::R_INT64:
                case me::Op::R_UINT64:
                    return ""; // nothing to do
                case me::Op::R_REAL32:
                    swiftAssert(!scratch, "must be invalid here");
                    oss << "\tcvtsi2ss\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL64:
                    swiftAssert(!scratch, "must be invalid here");
                    oss << "\tcvtsi2sd\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                default: swiftAssert(false, "unreachable code"); 
            }

        /*
         * unsigned integer src type
         */

        case me::Op::R_UINT8:
            switch (dst->type_)
            {
                case me::Op::R_INT8:
                case me::Op::R_UINT8:
                    return ""; // nothing to do
                case me::Op::R_INT16:
                case me::Op::R_INT32:
                case me::Op::R_INT64:
                case me::Op::R_UINT16:
                case me::Op::R_UINT32:
                case me::Op::R_UINT64:
                    oss << "\tmovzb" << suffix(dst->type_);
                    oss << "\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL32:
                    swiftAssert(scratch, "must be valid here");
                    oss << "\tmovzbl\t" << reg2str(src) << ", " << reg2str(scratch) << '\n';
                    oss << "\tcvtsi2ss\t" << reg2str(scratch) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL64:
                    swiftAssert(scratch, "must be valid here");
                    oss << "\tmovzbl\t" << reg2str(src) << ", " << reg2str(scratch) << '\n';
                    oss << "\tcvtsi2sd\t" << reg2str(scratch) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                default: swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_UINT16:
            switch (dst->type_)
            {
                case me::Op::R_INT8:
                case me::Op::R_UINT8:
                case me::Op::R_INT16:
                case me::Op::R_UINT16:
                    return ""; // nothing to do
                case me::Op::R_INT32:
                case me::Op::R_INT64:
                case me::Op::R_UINT32:
                case me::Op::R_UINT64:
                    oss << "\tmovzsw" << suffix(dst->type_);
                    oss << "\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL32:
                    swiftAssert(scratch, "must be valid here");
                    oss << "\tmovzwl\t" << reg2str(src) << ", " << reg2str(scratch) << '\n';
                    oss << "\tcvtsi2ss\t" << reg2str(scratch) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL64:
                    swiftAssert(scratch, "must be valid here");
                    oss << "\tmovzwl\t" << reg2str(src) << ", " << reg2str(scratch) << '\n';
                    oss << "\tcvtsi2sd\t" << reg2str(scratch) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                default: swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_UINT32:
            switch (dst->type_)
            {
                case me::Op::R_INT8:
                case me::Op::R_UINT8:
                case me::Op::R_INT16:
                case me::Op::R_UINT16:
                case me::Op::R_INT32:
                case me::Op::R_UINT32:
                    return ""; // nothing to do
                case me::Op::R_INT64:
                case me::Op::R_UINT64:
                    oss << "\tmovzl" << suffix(dst->type_);
                    oss << "\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL32:
                    swiftAssert(!scratch, "must be invalid here");
                    oss << "\tcvtsi2ss\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL64:
                    swiftAssert(!scratch, "must be invalid here");
                    oss << "\tcvtsi2sd\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                default: swiftAssert(false, "unreachable code"); 
            }
        case me::Op::R_UINT64:
            switch (dst->type_)
            {
                case me::Op::R_INT8:
                case me::Op::R_UINT8:
                case me::Op::R_INT16:
                case me::Op::R_UINT16:
                case me::Op::R_INT32:
                case me::Op::R_UINT32:
                case me::Op::R_INT64:
                case me::Op::R_UINT64:
                    return ""; // nothing to do
                case me::Op::R_REAL32:
                    swiftAssert(!scratch, "must be invalid here");
                    oss << "\tcvtsi2ss\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                case me::Op::R_REAL64:
                    swiftAssert(!scratch, "must be invalid here");
                    oss << "\tcvtsi2sd\t" << reg2str(src) << ", " << reg2str(dst) << '\n';
                    return oss.str();
                default: swiftAssert(false, "unreachable code"); 
            }
        default: swiftAssert(false, "TODO");
            return "error";
    }
}

} // namespace be
