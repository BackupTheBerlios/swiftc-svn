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

#ifndef BE_X64_CODE_GEN_H
#define BE_X64_CODE_GEN_H

#include <fstream>
#include <string>

#include "me/arch.h"
#include "me/codepass.h"

namespace be {

/*
 * forward declarations
 */

struct StackLayout;

//------------------------------------------------------------------------------

class X64CodeGen : public me::CodeGen
{
public:

    /*
     * constructor
     */

    X64CodeGen(me::Function* function, std::ofstream& ofs);

    /*
     * further methods
     */

    virtual void process();
    void genPhiInstr(me::BBNode* prevNode, me::BBNode* nextNode);
    void genMove(me::Op::Type type, int r1, int r2);
};

} // namespace be

//------------------------------------------------------------------------------

/*
 * globals
 */

int x64lex();
void x64error(const char* s);

#endif // BE_X64_CODE_GEN_H

