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

#include "utils/disjointsets.h"

#include "utils/assert.h"

/*
 * constructor and destructor
 */

DisjointSets::DisjointSets()
    : numSets_(0)
    , nodes_(0)
{}

DisjointSets::DisjointSets(size_t numElements)
    : numSets_(numElements)
    , nodes_(numElements)
{
    for (size_t i = 0; i < nodes_.size(); ++i)
    {
        nodes_[i] = new Node();
        nodes_[i]->rank_ = 0;
        nodes_[i]->index_ = i;
        nodes_[i]->parent_ = 0;
    }
}

DisjointSets::~DisjointSets()
{
    for (size_t i = 0; i < nodes_.size(); ++i)
        delete nodes_[i];
}

/*
 * further methods
 */

size_t DisjointSets::findSet(size_t elementId)
{
    swiftAssert(elementId < nodes_.size(), "element is greater than number of elements");
    Node* current = nodes_[elementId];

    // find element's root
    Node* root = current;
    while (root->parent_ != 0)
        root = root->parent_;

    // apply path compression
    while(current != root)
    {
        Node* next = current->parent_;
        current->parent_ = root;
        current = next;
    }

    return root->index_;
}

void DisjointSets::unite(size_t setId1, size_t setId2)
{
    swiftAssert(setId1 < nodes_.size(), "setId1 is greater than number of elements");
    swiftAssert(setId2 < nodes_.size(), "setId2 is greater than number of elements");

    if (setId1 == setId2)
        return; // nothing to do

    Node* set1 = nodes_[setId1];
    Node* set2 = nodes_[setId2];

    // apply union by rank
    if (set1->rank_ > set2->rank_)
        set2->parent_ = set1;
    else
    {
        set1->parent_ = set2;

        if (set1->rank_ == set2->rank_)
            ++set2->rank_;
    }

    // keep track of the number of ranks
    --numSets_;
}

/*
 * getters
 */

size_t DisjointSets::numSets() const
{
    return numSets_;
}

size_t DisjointSets::numElements() const
{
    return nodes_.size();
}

