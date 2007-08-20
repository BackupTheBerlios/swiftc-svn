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
    typedef List<Node*> Nodes;

    struct Node
    {
        T value_;

        size_t preOrderIndex_;
        size_t inOrderIndex_;
        size_t postOrderIndex_;

        Nodes pred_;
        Nodes succ_;

        Node(const T& t = T())
            : value_(t)
            , preOrderIndex_( std::numeric_limits<size_t>::max() )
            , inOrderIndex_( std::numeric_limits<size_t>::max() )
            , postOrderIndex_( std::numeric_limits<size_t>::max() )
        {}

        void link(Node* n)
        {
            succ_.append(n);
            n->pred_.append(this);
        }

        bool isPred(const Node* n) const
        {
            return pred_.search(n) != pred_.sentinel();
        }

        bool isSucc(const Node* n) const
        {
            return succ_.search(n) != succ_.sentinel();
        }

        size_t numPreds() const
        {
            return pred_.size();
        }

        size_t numSuccs() const
        {
            return pred_.size();
        }

        const Nodes& getPred()
        {
            return pred_;
        }

        const Nodes& getSucc()
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

        bool invalidatePreOrderIndex() const
        {
            preOrderIndex_ = std::numeric_limits<size_t>::max();
        }
        bool invalidateInOrderIndex() const
        {
            inOrderIndex_ = std::numeric_limits<size_t>::max();
        }
        bool invalidatePostOrderIndex() const
        {
            postOrderIndex_ = std::numeric_limits<size_t>::max();
        }
    };

    ~Graph();

    Node* insert(Node* n)
    {
        nodes_.append(n);
        return n;
    }

    Node* insert(const T& t)
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

    virtual std::string name() const = 0;

    Nodes nodes_;

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
    for (typename Nodes::Node* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
        delete iter->value_;
}

template<class T>
void Graph<T>::erase(Node* n)
{
    // for each predecessor
    for (typename Nodes::Node* iter = n->pred_.first(); iter != n->pred_.sentinel(); iter = iter->next())
    {
        typename Nodes::Node* f = iter->value_->succ_.search(n);
        // erase this node
        iter->value_->succ_.erase( f );
    }

    // for each successor
    for (typename Nodes::Node* iter = n->succ_.first(); iter != n->succ_.sentinel(); iter = iter->next())
    {
        typename Nodes::Node* f = iter->value_->pred_.search(n);
        // erase this node
        iter->value_->pred_.erase(f  );
    }

    nodes_.erase( nodes_.search(n) );
    delete n;
}

template<class T>
void Graph<T>::calcPreOrder(Graph<T>::Node* root)
{
    if ( root->isPreOrderIndexValid() )
    {
        // -> we must reset all postOrderIndices
        for (typename Nodes::Node* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
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

    for (typename Nodes::Node* iter = n->succ_.first(); iter != n->succ_.sentinel(); iter = iter->next())
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
        for (typename Nodes::Node* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
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
    for (typename Nodes::Node* iter = n->succ_.first(); iter != n->succ_.sentinel(); iter = iter->next())
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
        for (typename Nodes::Node* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
            iter->value_->invalidatePostOrderIndex();

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
    for (typename Nodes::Node* iter = n->succ_.first(); iter != n->succ_.sentinel(); iter = iter->next())
    {
        // is this node already reached?
        if ( iter->value_->isPostOrderIndexValid() )
            continue;

        postOrderWalk(iter->value_);
    }

    // process this node
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

    // iterate over all nodes
    for (typename Nodes::Node* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
    {
        Node* n = iter->value_;
        // start a new node
        ofs << n->value_.name() << " [shape=box, label=\"\\" << std::endl;
        ofs << n->value_.toString();
        // close this node
        ofs << "\"]" << std::endl << std::endl;
    }

    // iterate over all nodes in order to print connections
    for (typename Nodes::Node* iter = nodes_.first(); iter != nodes_.sentinel(); iter = iter->next())
    {
        Node* n = iter->value_;

        // for all successors
        for (typename Nodes::Node* iter = n->succ_.first(); iter != n->succ_.sentinel(); iter = iter->next())
            ofs << n->value_.name() << " -> " << iter->value_->value_.name() << std::endl;
    }

    // end graphviz dot file
    ofs << std::endl << '}' << std::endl;
    ofs.close();
}

#endif // SWIFT_GRAPH_H
