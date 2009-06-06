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

private:

    Function* simdFunction_;

    /// Maps src labels to new labels.
    typedef std::map<InstrNode*, InstrNode*> LabelNode2LabelNode;
    LabelNode2LabelNode labelNode2LabelNode_;
};

} // namespace me

#endif // ME_VECTORIZER_H
