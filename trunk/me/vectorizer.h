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

#ifndef ME_VECTORIZER_H
#define ME_VECTORIZER_H

#include "utils/map.h"

#include "me/codepass.h"
#include "me/forward.h"

namespace me {

class Vectorizer : public CodePass
{
public:

    /*
     * constructor
     */

    Vectorizer(Function* function);

    /*
     * virtual methods
     */

    void process();

    /*
     * further methods
     */

    Function* getSimdFunction();
    Function* function();
    int getSimdLength();

//private:

    Function* simdFunction_;

    InstrNode* currentInstrNode_;
    BBNode* currentBB_;

    typedef Map<InstrNode*, InstrNode*> Label2Label;
    Label2Label src2dstLabel_;
    Label2Label dst2srcLabel_;

    typedef Map<int, int> Nr2Nr;
    Nr2Nr src2dstNr_;
};

} // namespace me

#endif // ME_VECTORIZER_H
