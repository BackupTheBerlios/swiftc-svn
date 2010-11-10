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

#include <vector>

#ifndef SWIFT_DISJOINT_SETS
#define SWIFT_DISJOINT_SETS

class DisjointSets
{
public:

    /*
     * constructor and destructor
     */

    /// Create an empty DisjointSets data structure.
    DisjointSets();
    /// Create a DisjointSets data structure with \p numElements elements.
    DisjointSets(size_t numElements);

    ~DisjointSets();

    /*
     * further methods
     */

    /**
     *
     * Find set identifier by element identifier
     *
     * @param element The element's identifier.
     * @return The set's identifier
     */
    size_t findSet(size_t elementId);

    /**
     * Unite two sets into one. 
     * All elements in those two sets will share the same set identifier
     * afterwards.
     * 
     * @param set1 Identifier of set 1.
     * @param set2 Identifier of set 2.
     */
    void unite(size_t setId1, size_t setId2);

    /*
     * getters
     */

    /// Returns the number of sets.
    size_t numSets() const;

    /// Return the number of elements.
    size_t numElements() const;

private:

    struct Node
    {
        size_t rank_;
        size_t index_;
        Node* parent_;
    };

    size_t numSets_; // the number of sets currently in the DisjointSets data structure.
    std::vector<Node*> nodes_;
};

#endif // SWIFT_DISJOINT_SETS
