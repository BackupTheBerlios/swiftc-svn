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

#ifndef ME_COALESCING_H
#define ME_COALESCING_H

#include "utils/set.h"

#include "me/codepass.h"

namespace me {

class Coalescing : public CodePass
{
public:

    /*
     * constructor
     */

    /// Use this one for coloring of spill slots.
    Coalescing(Function* function, int typeMask);

    /*
     * virtual methods
     */

    virtual void process();

private:

    struct Chunk
    {
        int todo_;
    };

    typedef Set<Chunk> Chunks;

    struct Node
    {
        int todo_;
    };

    typedef Set<Node*> Nodes;

    /*
     * further methods
     */

    Chunks buildChunks();
    void recolorChunks(Chunk& chunk);
    void recolor(Node& n, int color);
    void setColor(Node& n, int color, Nodes nodes);
    void avoidColor(Node& n, int color, Nodes nodes);
};

} // namespace me

#endif // ME_COALESCING_H
