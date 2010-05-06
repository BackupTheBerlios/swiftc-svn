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

#ifndef ME_INSTR_COALESCING_H
#define ME_INSTR_COALESCING_H

#include <queue>

#include "utils/disjointsets.h"
#include "utils/set.h"

#include "me/codepass.h"
#include "me/op.h"

namespace me {

class InstrCoalescing : public CodePass
{
public:

    /*
     * constructor
     */

    /// Use this one for coloring of spill slots.
    InstrCoalescing(Function* function, const Colors& reservoir, int typeMask);
    InstrCoalescing(Function* function, int typeMask);

    /*
     * virtual methods
     */

    virtual void process();

    /*
     * further methods
     */

    struct Node
    {
        Reg* reg_;
        bool fixed_;
        int oldColor_;

        Node() {}
        Node(Reg* reg, bool fixed = false)
            : reg_(reg)
            , fixed_(fixed)
#ifdef SWIFT_DEBUG
            , oldColor_(-1)
#endif // SWIFT_DEBUG
        {}
        Node(const Node& n)
            : reg_(n.reg_)
            , fixed_(n.fixed_)
            , oldColor_(n.oldColor_)
        {}
    };

    typedef Map<Reg*, Node> Nodes;
    typedef Set<Node*> NodeSet;


private:

    bool isSpilled() const;

    void recolor(Node* n, int color);
    void setColor(Node* n, int color, NodeSet& changed);
    bool avoidColor(Node* n, int color, NodeSet& changed);
    void coalesceVars();

    /*
     * data
     */

    const Colors reservoir_;
    int typeMask_;
    Nodes nodes_;
};

} // namespace me

#endif // ME_INSTR_COALESCING_H

