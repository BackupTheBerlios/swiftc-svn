#ifndef SWIFT_LIST_H
#define SWIFT_LIST_H

#include <cstddef>

#include "assert.h"

// Because std::list sucks here my own implementation

/**
 *  A circular doubly linked list
 */
template<class T>
class List
{
public:
    struct Node
    {
        Node* next_;
        Node* prev_;

        T value_;

        Node(const T& t = T())
            : value_(t)
        {}
        ~Node()
        {
            if (next_)
                delete next_;
        }

        Node* next()
        {
            return next_;
        }

        Node* prev()
        {
            return prev_;
        }
    };

private:

    size_t  size_;
    Node*   sentinel_;

public:

    List()
    {
        sentinel_ = new Node();
        sentinel_->prev_ = sentinel_;
        sentinel_->next_ = sentinel_;

        size_ = 0;
    }

    ~List()
    {
        // mark end of destruction
        sentinel_->prev_->next_ = 0;
        delete sentinel_;
    }

    /**
     * appends n to the list
     * @param n the node to be appended
     */
    void append(Node* n)
    {
        Node* formerLast = sentinel_->prev_;

        sentinel_->prev_ = n;
        formerLast->next_ = n;

        n->next_ = sentinel_;
        n->prev_ = formerLast;

        ++size_;
    }

    /**
     * appends t to the list
     * @param t the value to be appended
     */
    void append(const T& t)
    {
        Node* n = new Node(t);

        Node* formerLast = sentinel_->prev_;

        sentinel_->prev_ = n;
        formerLast->next_ = n;

        n->next_ = sentinel_;
        n->prev_ = formerLast;

        ++size_;
    }

    /**
     * prepends n to the list
     * @param n the node to be prepended
     */
    void prepend(Node* n)
    {
        Node* formerFirst = sentinel_->next_;

        sentinel_->next_ = n;
        formerFirst->prev_ = n;

        n->prev_ = sentinel_;
        n->next_ = formerFirst;

        ++size_;
    }

    /**
     * Prepends n to the list-
     * @param t the value to be prepended
     */
    void prepend(const T& t)
    {
        Node* n = new Node(t);

        Node* formerFirst = sentinel_->next_;

        sentinel_->next_ = n;
        formerFirst->prev_ = n;

        n->prev_ = sentinel_;
        n->next_ = formerFirst;

        ++size_;
    }

    /**
     * Inserts a Node n after node prev.
     * @param prev the predecessor of n
     * @param n the node to be inserted
     */
    void insert(Node* prev, Node* n)
    {
        swiftAssert(n != sentinel_, "tried to insert sentinel");

        Node* next = prev->next_;

        prev->next_ = n;
        n->prev_ = prev;

        n->next_ = next;
        next->prev_ = n;

        ++size_;
    }

    /**
     * inserts a node n after node prev
     * @param prev the predecessor of n
     * @param t the value to be inserted
     */
    void insert(Node* prev, const T& t)
    {
        Node* n = new Node(t);
        Node* next = prev->next_;

        prev->next_ = n;
        n->prev_ = prev;

        n->next_ = next;
        next->prev_ = n;

        ++size_;
    }

    /// removes the first item of the list
    void removeFirst()
    {
        Node* newFirst = sentinel_->next_->next_;

        delete sentinel_->next_;

        sentinel_->next_ = newFirst;
        newFirst->prev_ = sentinel_;

        --size_;
    }

    /// removes the first item of the list
    void removeLast()
    {
        Node* newLast = sentinel_->prev_->prev_;

        delete sentinel_->prev_;

        sentinel_->prev_ = newLast;
        newLast->next_ = sentinel_;

        --size_;
    }

    void erase(Node* n)
    {
        swiftAssert(n != sentinel_, "tried to erase sentinel");

        Node* prev = n->prev_;
        Node* next = n->next_;

        delete n;

        prev->next_ = next;
        next->prev_ = prev;

        --size_;
    }

    /**
     * Searches for the first t in the list. Returns sentinel() if not found.
     * @param t the value to be searched
     * @return The node which contains t if found.
     * If t was not found sentinel() is returned.
     */
    Node* search(const T& t)
    {
        Node* n = sentinel_->next_;

        while (n != sentinel_ && n->value_ != t)
            n = n->next_;

        return n;
    }

    Node* sentinel()
    {
        return sentinel_;
    }

    Node* first()
    {
        return sentinel_->next_;
    }

    Node* last()
    {
        return sentinel_->prev_;
    }

    size_t size() {
        return size_;
    }

    bool empty()
    {
        return bool(size_);
    }
};

#endif // SWIFT_LIST_H
