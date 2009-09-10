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

/*
 * remarks:
 *
 * - the nodes in recolorChunk must be sorted by costs, too
 * - intNeighbors(var) must not return var itself
 * - first/last instr must be examined in findInt, too
 * - intNeighbors must examine the liveOut set once befor iteration
 * - intNeighbors must ignore phi functions
 * - constrained regs and args in intNeighbors must be returned, too
 */

#include "me/coalescing.h"

#include <limits>

#include "me/cfg.h"
#include "me/functab.h"
#include "me/op.h"
#include "me/ssa.h"

/*
 * helper
 */

namespace {

struct NodeCmp
{
    bool operator () (const me::Coalescing::Node* n1, const me::Coalescing::Node* n2)
    {
        return n1->costs_ > n2->costs_;
    }
};

}

#define NODESET_EACH(iter, nodeSet) \
    for (me::Coalescing::NodeSet::iterator (iter) = (nodeSet).begin(); (iter) != (nodeSet).end(); ++(iter))

namespace me {

/*
 * constructor
 */

Coalescing::Coalescing(Function* function, 
                       const Colors& reservoir, 
                       int typeMask)
    : CodePass(function)
    , reservoir_(reservoir)
    , typeMask_(typeMask)
{}

Coalescing::Coalescing(Function* function, 
                       int typeMask)
    : CodePass(function)
    , reservoir_(Colors())  // use an empty set
    , typeMask_(typeMask)
{}

/*
 * virtual methods
 */


void Coalescing::process()
{
    // create nodes for each regs in question
    VARMAP_EACH(iter, function_->vars_)
    {
        Reg* reg = iter->second->isReg( typeMask_, isSpilled() );
        if (reg)
            reg2Node_.insert( std::make_pair(reg, Node(reg)) );
    }

    buildAffinityEdges();
    buildChunks();

    // transfer chunks_ to q_
    for (size_t i = 0; i < chunks_.size(); ++i)
        q_.push( chunks_[i] );

    //std::cout << "-- " << *function_->id_ << " --" << std::endl;

    while ( !q_.empty() )
    {
        currentChunk_ = q_.top();
        q_.pop();

        std::sort( currentChunk_->nodes_.begin(), 
                   currentChunk_->nodes_.end(), 
                   ::NodeCmp() );

        //for (size_t i = 0; i < currentChunk_->nodes_.size(); ++i)
            //std::cout << "\t" << currentChunk_->nodes_[i]->reg_->toString() << " | " << currentChunk_->nodes_[i]->costs_ << std::endl;
        //std::cout << "---" << std::endl;

        recolorChunk();
    }

    // clean up
    for (size_t i = 0; i < chunks_.size(); ++i)
        delete chunks_[i];
}

/*
 * further methods
 */

bool Coalescing::isSpilled() const
{
    return reservoir_.empty();
}

void Coalescing::buildAffinityEdges()
{
    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        PhiInstr* phi = dynamic_cast<PhiInstr*>(iter->value_);

        if (phi)
        {
            size_t numArgs = phi->arg_.size();

            Reg* to = phi->result()->isReg( typeMask_, isSpilled() );
            reg2Node_[to].costs_ += 1;

            if (!to)
                continue;

            // for each arg
            for (size_t i = 0; i < numArgs; ++i)
            {
                swiftAssert( phi->arg_[i].op_->isReg(typeMask_, isSpilled()),
                        "must be an appropriate reg" );
                Reg* from = (Reg*) phi->arg_[i].op_;

                BBList& domFrontier = phi->sourceBBs_[i]->value_->domFrontier_;
                if ( domFrontier.find( to->def_.bbNode_) != domFrontier.sentinel() )
                {
                    //std::cout << from->toString() << " -> " << to->toString() << std::endl;
                    reg2Node_[from].costs_ += 100;
                    reg2Node_[to].costs_ += 10;
                }
                else
                    reg2Node_[from].costs_ += 1;


                affinityEdges_.push_back( AffinityEdge(from, to) );
                regs_.insert(from);
                regs_.insert(to);
            }
        }
    }

    for (size_t i = 0; i < affinityEdges_.size(); ++i)
    {
        AffinityEdge& ae = affinityEdges_[i];
        ae.costs_ = reg2Node_[ae.from_].costs_ + reg2Node_[ae.to_].costs_;
    }

    std::sort( affinityEdges_.begin(), affinityEdges_.end() );
}

void Coalescing::buildChunks()
{
    /*
     * give every reg a unique number
     */

    typedef Map<Reg*, size_t> Reg2Id;
    Reg2Id reg2Id;

    size_t idCounter = 0;
    REGSET_EACH(iter, regs_)
        reg2Id[*iter] = idCounter++;

    // every set belongs to one reg now identified via the id
    DisjointSets chunkSets_(idCounter);

    // for each affinity edge
    for (size_t i = 0; i < affinityEdges_.size(); ++i)
    {
        Reg* from = affinityEdges_[i].from_;
        Reg* to   = affinityEdges_[i].to_;

        swiftAssert( reg2Id.contains(from), "must be found here" );
        swiftAssert( reg2Id.contains(to),   "must be found here" );

        size_t fromSet = chunkSets_.findSet( reg2Id[from] );
        size_t toSet   = chunkSets_.findSet( reg2Id[to] );

        /*
         * if no node in from’s chunk does interfere with any node of to’s
         * chunk, the chunks of from and to are unified
         */

        RegSet neighbors = cfg_->intNeighbors( from, typeMask_, isSpilled() );

        // for each interfering neighbor of from
        REGSET_EACH(iter, neighbors)
        {
            Reg* reg = *iter;
            Reg2Id::iterator reg2IdIter = reg2Id.find(reg);
            if ( reg2IdIter == reg2Id.end() )
                continue;

            size_t regSet = chunkSets_.findSet(reg2IdIter->second);

            if (fromSet == regSet && toSet == regSet)
                goto outer_loop; // do not unite
        }

        // no interference between from's and to's chunks -> join them
        chunkSets_.unite(fromSet, toSet);

outer_loop:
        continue;
    }

    typedef Map<size_t, size_t> Set2ChunkIndex;
    Set2ChunkIndex set2ChunkIndex;

    // create chunks
    chunks_.resize( chunkSets_.numSets() );

    for (size_t i = 0; i < chunks_.size(); ++i)
        chunks_[i] = new Chunk();

    size_t indexCounter = 0;

    REGSET_EACH(iter, regs_)
    {
        Reg* reg = *iter;
        swiftAssert( reg2Id.contains(reg), "must be found here" );
        size_t regSet = chunkSets_.findSet( reg2Id[reg] );

        size_t index;

        Set2ChunkIndex::iterator mapIter = set2ChunkIndex.find(regSet);
        if ( mapIter == set2ChunkIndex.end() )
        {
            // not found -> create mapping
            index = indexCounter++;
            swiftAssert( index < chunks_.size(), "counter exceeded size" );
            set2ChunkIndex.insert( std::make_pair(regSet, index) );
        }
        else
            index = mapIter->second; // found

        chunks_[index]->nodes_.push_back( &reg2Node_[reg] );
    }

    // calc costs
    for (size_t i = 0; i < chunks_.size(); ++i)
        chunks_[i]->calcCosts();
}

void Coalescing::recolorChunk()
{
    int bestCosts = 0;
    int bestColor;
    Chunk bestSubChunk;

    // for each color
    for (Colors::iterator iter = reservoir_.begin(); iter != reservoir_.end(); ++iter)
    {
        // unfix all nodes in chunk
        currentChunk_->unfixNodes();

        int color = *iter;

        // try to bring all nodes to color
        for (size_t i = 0; i < currentChunk_->nodes_.size(); ++i)
        {
            Node* n = currentChunk_->nodes_[i];
            recolor(n, color, false);
            n->fixed_ = true;
        }

        Chunk subChunk;
        // check which nodes got color 'color'
        for (size_t i = 0; i < currentChunk_->nodes_.size(); ++i)
        {
            Node* n = currentChunk_->nodes_[i];
            if (n->reg_->color_ == color)
                subChunk.nodes_.push_back(n);
        }

        subChunk.calcCosts();
        if (subChunk.costs_ > bestCosts)
        {
            bestColor = color;
            bestSubChunk = subChunk;
            bestCosts = subChunk.costs_;
        }
    }

    /*
     * finally color all nodes to the best color
     */

    // unfix all nodes in currentChunk_
    currentChunk_->unfixNodes();

    NodeSet bestSubChunkNodes;

    // try to bring all nodes to bestColor
    for (size_t i = 0; i < bestSubChunk.nodes_.size(); ++i)
    {
        Node* n = bestSubChunk.nodes_[i];
        recolor(n, bestColor, true);
        n->fixed_ = true;
        bestSubChunkNodes.insert(n);
    }

    if ( bestSubChunk.nodes_.size() != currentChunk_->nodes_.size() )
    {
        // -> build new chunk out of the rest

        Chunk* restChunk = new Chunk;;

        for (size_t i = 0; i < currentChunk_->nodes_.size(); ++i)
        {
            Node* n = currentChunk_->nodes_[i];
            if ( !bestSubChunkNodes.contains(n) )
                restChunk->nodes_.push_back(n);
        }

        restChunk->calcCosts();
        chunks_.push_back(restChunk);
        q_.push(restChunk);
    }
}

void Coalescing::recolor(Node* n, int color, bool final)
{
    Reg* reg = n->reg_;

    // recolor possible?
    if (n->fixed_)
        return;

    if ( !reg->isColorAdmissible(color) )
        return;

    NodeSet changed;
    setColor(n, color, changed, final);

    RegSet neighbors = cfg_->intNeighbors( reg, typeMask_, isSpilled() );

    // for each neighbor
    REGSET_EACH(iter, neighbors)
    {
        Reg* neighbor = *iter;
        swiftAssert( reg2Node_.contains(neighbor), "must be found" );

        // does it also have color 'color'?
        if (neighbor->color_ == color)
        {
            Node* neighborNode = &reg2Node_[neighbor];

            if ( !avoidColor(neighborNode, color, changed, final) )
            {
                // -> that did not work so rollback
                NODESET_EACH(changedIter, changed)
                {
                    Node* current = *changedIter;
                    swiftAssert(current->oldColor_ != -1, "must be set");
                    current->reg_->color_ = current->oldColor_;
                }
            }
        }
    }

    NODESET_EACH(changedIter, changed)
        (*changedIter)->fixed_ = false;
}

void Coalescing::setColor(Node* n, int color, NodeSet& changed, bool final)
{
    n->fixed_ = true;
    n->oldColor_ = n->reg_->color_;
    n->reg_->color_ = color;
    changed.insert(n);

    if (final)
        function_->usedColors_.insert(color);
}

bool Coalescing::avoidColor(Node* n, int color, NodeSet& changed, bool final)
{
    if (n->reg_->color_ != color)
        return true;

    if (n->fixed_)
        return false;

    // find admissible colors for n
    Colors admissible;
    for (Colors::iterator iter = reservoir_.begin(); iter != reservoir_.end(); ++iter)
    {
        int admissibleColor = *iter;

        if (admissibleColor == color)
            continue;

        if ( n->reg_->isColorAdmissible(admissibleColor) )
            admissible.insert(admissibleColor);
    }

    if ( admissible.empty() )
        return false;

    RegSet neighbors = cfg_->intNeighbors( n->reg_, typeMask_, isSpilled() );

    // select a color newColor in admissible which is used least in neighbors
    int min = std::numeric_limits<int>::max();
    int newColor;

    for (Colors::iterator iter = admissible.begin(); iter != admissible.end(); ++iter)
    {
        int currentColor = *iter;
        int counter = 0;

        REGSET_EACH(iter, neighbors)
        {
            Reg* neighbor = *iter;
            if (neighbor->color_ == currentColor)
                ++counter;
        }

        if (min > counter )
        {
            min = counter;
            newColor = currentColor;
        }
    }

    swiftAssert( min != std::numeric_limits<int>::max(), 
            "at least one color must have been found" );
    setColor(n, newColor, changed, final);

    REGSET_EACH(iter, neighbors)
    {
        Reg* neighbor = *iter;
        swiftAssert( reg2Node_.contains(neighbor), "must be found" );
        Node* neighborNode = &reg2Node_[neighbor];
        if ( !avoidColor(neighborNode, newColor, changed, final) )
            return false;
    }

    return true;
}

#undef NODESET_EACH

} // namespace me
