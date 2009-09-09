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

#include <queue>

#include "utils/disjointsets.h"
#include "utils/set.h"

#include "me/codepass.h"
#include "me/op.h"

namespace me {

class Coalescing : public CodePass
{
public:

    /*
     * additional data structures
     */

    struct AffinityEdge 
    {
        Reg* from_;
        Reg* to_;
        int costs_;

        AffinityEdge(Reg* from, Reg* to)
            : from_(from)
            , to_(to)
            , costs_(0)
        {}

        bool operator < (const AffinityEdge& ae) const  
        {
            return costs_ > ae.costs_;
        }
    };

    struct Node
    {
        Reg* reg_;
        int costs_;
        bool fixed_;
        int oldColor_;

        Node() {}
        Node(Reg* reg, int costs = 1, bool fixed = false)
            : reg_(reg)
            , costs_(costs)
            , fixed_(fixed)
#ifdef SWIFT_DEBUG
            , oldColor_(-1)
#endif // SWIFT_DEBUG
        {}
        Node(const Node& n)
            : reg_(n.reg_)
            , costs_(n.costs_)
            , fixed_(n.fixed_)
            , oldColor_(n.oldColor_)
        {}
    };

    typedef std::vector<AffinityEdge> AffinityEdges;
    typedef std::vector<Node*> Nodes;


    struct Chunk
    {
        Nodes nodes_;
        int costs_;

        Chunk()
            : nodes_( Nodes() )
            , costs_(0)
        {}
        Chunk(const Chunk& chunk)
            : nodes_(chunk.nodes_)
            , costs_(chunk.costs_)
        {}

        void unfixNodes()
        {
            for (size_t i = 0; i < nodes_.size(); ++i)
                nodes_[i]->fixed_ = false;
        }

        void calcCosts()
        {
            costs_ = 0;

            for (size_t i = 0; i < nodes_.size(); ++i)
                costs_ += nodes_[i]->costs_;
        }
    };


    struct ChunkCmp
    {
        bool operator () (const Chunk* c1, const Chunk* c2)
        {
            return c1->costs_ < c2->costs_;
        }
    };

    typedef std::vector<Chunk*> Chunks;


    typedef std::priority_queue<Chunk*, std::vector<Chunk*>, ChunkCmp> Q;
    typedef Set<Node*> NodeSet;

    /*
     * constructor
     */

    /// Use this one for coloring of spill slots.
    Coalescing(Function* function, const Colors& reservoir, int typeMask);
    Coalescing(Function* function, int typeMask);

    /*
     * virtual methods
     */

    virtual void process();

    /*
     * further methods
     */

private:

    bool isSpilled() const;

    void buildAffinityEdges();
    void buildChunks();
    void recolorChunk();
    void recolor(Node* n, int color);
    void setColor(Node* n, int color, NodeSet& changed);
    bool avoidColor(Node* n, int color, NodeSet& changed);
    void coalesceVars();

    /*
     * data
     */

    const Colors reservoir_;
    int typeMask_;

    AffinityEdges affinityEdges_;
    RegSet regs_;

    Chunks chunks_;
    Chunk* currentChunk_;
    Q q_;

    typedef Map<Reg*, Node> Reg2Node;
    Reg2Node reg2Node_;
};

} // namespace me

#endif // ME_COALESCING_H
