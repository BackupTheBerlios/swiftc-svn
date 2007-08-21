#ifndef SWIFT_LIST_H
#define SWIFT_LIST_H

#include <cstddef>

#include "utils/assert.h"

// Because std::list sucks here my own implementation

/**
 *  @brief A circular doubly linked list
 */
template<class T>
class List
{
public:

    struct Node
    {
        friend List::~List();
        friend void List::removeFirst();
        friend void List::removeLast();
        friend void List::erase(Node* n);

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

        const Node* next() const
        {
            return next_;
        }

        const Node* prev() const
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
     * Inserts a Node n behind node prev.
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
     * inserts a node n behind node prev
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

        // mark end of destruction
        sentinel_->next_->next_ = 0;
        delete sentinel_->next_;

        sentinel_->next_ = newFirst;
        newFirst->prev_ = sentinel_;

        --size_;
    }

    /// removes the first item of the list
    void removeLast()
    {
        Node* newLast = sentinel_->prev_->prev_;

        // mark end of destruction
        sentinel_->prev_->next_ = 0;
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

        // mark end of destruction
        n->next_ = 0;
        delete n;

        prev->next_ = next;
        next->prev_ = prev;

        --size_;
    }

    /**
     * Searches for the first \p t in the list. Returns sentinel() if not found.
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

    /**
     * Searches for the Node \p n and returns its position.
     * 0 means the first and so on. size() is returned if it is not found.
     * @param n the node to be searched
     * @return The node which contains t if found.
     * If t was not found sentinel() is returned.
     */
    size_t position(Node* n) const
    {
        size_t pos = 0;
        Node* iter = sentinel_->next_;

        while (iter != n && iter != sentinel_)
        {
            iter = iter->next_;
            ++pos;
        }

        return pos;
    }

    /**
     * Searches for the first \p t and returns its position.
     * 0 means the first and so on. size() is returned if it is not found.
     * @param t the value to be searched
     * @return The node which contains t if found.
     * If t was not found sentinel() is returned.
     */
    size_t position(const T& t) const
    {
        size_t pos = 0;
        Node* iter = sentinel_->next_;

        while (iter != sentinel_ && iter->value_ != t)
        {
            iter = iter->next_;
            ++pos;
        }

        return pos;
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

    const Node* sentinel() const
    {
        return sentinel_;
    }

    const Node* first() const
    {
        return sentinel_->next_;
    }

    const Node* last() const
    {
        return sentinel_->prev_;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const
    {
        return size_ == 0;
    }
};

#endif // SWIFT_LIST_H
