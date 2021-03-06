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

#ifndef SWIFT_GRAPH_H
#define SWIFT_GRAPH_H

#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "utils/assert.h"
#include "utils/list.h"

template<class T>
class Graph
{
public:

    struct Node;
    typedef List<Node*> Relatives;
    typedef typename List<Node*>::Node Relative;

#define RELATIVES_EACH(iter, relatives) \
    for (Relative* (iter) = (relatives).first(); (iter) != (relatives).sentinel(); (iter) = (iter)->next())

    struct Node
    {
        T* value_;

        size_t preOrderIndex_;
        size_t inOrderIndex_;
        size_t postOrderIndex_;

        Relatives pred_;
        Relatives succ_;

        mutable bool visted_; // Since this is an implementation detail it is declared mutable.

    public:

        Node(T* t)
            : value_(t)
            , preOrderIndex_( std::numeric_limits<size_t>::max() )
            , inOrderIndex_( std::numeric_limits<size_t>::max() )
            , postOrderIndex_( std::numeric_limits<size_t>::max() )
            , visted_(false)
        {}

        ~Node()
        {
            delete value_;
        }

        void link(Node* n)
        {
            succ_.append(n);
            n->pred_.append(this);
        }

        bool isPred(const Node* n) const
        {
            return pred_.find(n) != pred_.sentinel();
        }

        bool isSucc(const Node* n) const
        {
            return succ_.find(n) != succ_.sentinel();
        }

        size_t numPreds() const
        {
            return pred_.size();
        }

        size_t numSuccs() const
        {
            return succ_.size();
        }

        const Relatives& getPred()
        {
            return pred_;
        }

        const Relatives& getSucc()
        {
            return succ_;
        }

        bool isPreOrderIndexValid() const
        {
            return preOrderIndex_ != std::numeric_limits<size_t>::max();
        }
        bool isInOrderIndexValid() const
        {
            return inOrderIndex_ != std::numeric_limits<size_t>::max();
        }
        bool isPostOrderIndexValid() const
        {
            return postOrderIndex_ != std::numeric_limits<size_t>::max();
        }

        void invalidatePreOrderIndex()
        {
            preOrderIndex_ = std::numeric_limits<size_t>::max();
        }
        void invalidateInOrderIndex()
        {
            inOrderIndex_ = std::numeric_limits<size_t>::max();
        }
        void invalidatePostOrderIndex()
        {
            postOrderIndex_ = std::numeric_limits<size_t>::max();
        }

        size_t whichPred(Node* n) const
        {
            return pred_.position(n);
        }
        size_t whichSucc(Node* n) const
        {
            return succ_.position(n);
        }
    };

    Graph(bool noDigraph = false)
        : noDigraph_(noDigraph)
    {}

    virtual ~Graph();

    Node* insert(Node* n)
    {
        nodes_.append(n);
        return n;
    }

    Node* insert(T* t)
    {
        Node* n = new Node(t);
        nodes_.append(n);
        return n;
    }

    void erase(Node* n);

    void calcPreOrder(Node* root);
    void calcInOrder(Node* root);
    void calcPostOrder(Node* root);

    void dumpDot(const std::string& baseFilename);

    size_t size() const
    {
        return nodes_.size();
    }

    virtual std::string name() const = 0;

    bool noDigraph_;

    Relatives nodes_;

    std::vector<Node*> preOrder_;
    std::vector<Node*> inOrder_;
    std::vector<Node*> postOrder_;

private:

    void preOrderWalk(Node* n);
    void inOrderWalk(Node* n);
    void postOrderWalk(Node* n);

    size_t indexCounter_;
};

//------------------------------------------------------------------------------
//  Non inline implementation
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Graph::Node
//------------------------------------------------------------------------------

//...

//------------------------------------------------------------------------------
// Graph
//------------------------------------------------------------------------------

template<class T>
Graph<T>::~Graph()
{
    for (Relative* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
        delete iter->value_;
}

template<class T>
void Graph<T>::erase(Node* n)
{
    // for each predecessor
    for (Relative* iter = n->pred_.first(); iter != n->pred_.sentinel(); iter = iter->next())
    {
        Relative* f = iter->value_->succ_.find(n);
        // erase this node
        iter->value_->succ_.erase( f );
    }

    // for each successor
    for (Relative* iter = n->succ_.first(); iter != n->succ_.sentinel(); iter = iter->next())
    {
        Relative* f = iter->value_->pred_.find(n);
        // erase this node
        iter->value_->pred_.erase(f  );
    }

    nodes_.erase( nodes_.find(n) );
    delete n;
}

template<class T>
void Graph<T>::calcPreOrder(Graph<T>::Node* root)
{
    if ( root->isPreOrderIndexValid() )
    {
        // -> we must reset all postOrderIndices
        for (Relative* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
            iter->value_->invalidatePreOrderIndex();

        // clear old postOrder_ vector
        preOrder_.clear();
    }

    preOrder_.resize( nodes_.size() );

    indexCounter_ = 0;
    preOrderWalk(root);
}

template<class T>
void Graph<T>::preOrderWalk(Node* n)
{
    // process this node
    preOrder_[indexCounter_] = n;
    n->preOrderIndex_ = indexCounter_;
    ++indexCounter_;

    for (Relative* iter = n->succ_.first(); iter != n->succ_.sentinel(); iter = iter->next())
    {
        // is this node already reached?
        if ( iter->value_->isPreOrderIndexValid() )
            continue;

        preOrderWalk(iter->value_);
    }
}

template<class T>
void Graph<T>::calcInOrder(Graph<T>::Node* root)
{
    if ( root->isInOrderIndexValid() )
    {
        // -> we must reset all postOrderIndices
        for (Relative* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
            iter->value_->invalidateInOrderIndex();

        // clear old postOrder_ vector
        inOrder_.clear();
    }

    inOrder_.resize( nodes_.size() );

    indexCounter_ = 0;
    inOrderWalk(root);
}

template<class T>
void Graph<T>::inOrderWalk(Node* n)
{
    for (Relative* iter = n->succ_.first(); iter != n->succ_.sentinel(); iter = iter->next())
    {
        // is this node already reached?
        if ( iter->value_->isInOrderIndexValid() )
            continue;

        inOrderWalk(iter->value_);

        // process this node
        inOrder_[indexCounter_] = n;
        n->inOrderIndex_ = indexCounter_;
        ++indexCounter_;
    }
}

template<class T>
void Graph<T>::calcPostOrder(Graph<T>::Node* root)
{
    if ( root->isPostOrderIndexValid() )
    {
        // -> we must reset all postOrderIndices
        for (Relative* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
        {
            iter->value_->invalidatePostOrderIndex();
            iter->value_->visted_ = false;
        }

        // clear old postOrder_ vector
        postOrder_.clear();
    }

    postOrder_.resize( nodes_.size() );

    indexCounter_ = 0;
    postOrderWalk(root);
}

template<class T>
void Graph<T>::postOrderWalk(Node* n)
{
    // mark node as visted
    n->visted_ = true;

    for (Relative* iter = n->succ_.first(); iter != n->succ_.sentinel(); iter = iter->next())
    {
        // is this node already reached?
        if ( iter->value_->visted_ == true )
            continue;

        postOrderWalk(iter->value_);
    }

    // vist node
    postOrder_[indexCounter_] = n;
    n->postOrderIndex_ = indexCounter_;
    ++indexCounter_;
}

template<class T>
void Graph<T>::dumpDot(const std::string& baseFilename)
{
    std::ostringstream oss;
    oss << baseFilename << ".dot";

    std::ofstream ofs( oss.str().c_str() );// std::ofstream does not support std::string...

    // prepare graphviz dot file
    ofs << "digraph " << this->name() << " {" << std::endl << std::endl;

    // set nodes to record shape
    ofs << "node [shape=record]" << std::endl << std::endl;
    if (noDigraph_) // HACK
        ofs << "edge [arrowhead=none]" << std::endl << std::endl;

    // iterate over all nodes
    for (Relative* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
    {
        Node* n = iter->value_;
        // start a new node
        ofs << n->value_->name() << " [label=\"{\\" << std::endl;
        ofs << n->value_->toString();
        // close this node
        ofs << "}\"]" << std::endl << std::endl;
    }

    // iterate over all nodes in order to print connections
    for (Relative* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
    {
        Node* n = iter->value_;

        // for all successors
        for (Relative* iter = n->succ_.first(); iter != n->succ_.sentinel(); iter = iter->next())
            ofs << n->value_->name() << " -> " << iter->value_->value_->name() << std::endl;
    }

    // end graphviz dot file
    ofs << std::endl << '}' << std::endl;
    ofs.close();
}

#endif // SWIFT_GRAPH_H
