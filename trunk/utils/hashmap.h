#ifndef SWIFT_HASHMAP_H
#define SWIFT_HASHMAP_H

#include <functional>   // for std::equal_to
#include <limits>
#include <string>
#include <utility>      // for std::pair and std::make_pair

#include "list.h"
#include "types.h"

/*
    some hashCode functions
*/

/**
 * This hashCode used by HashFuc
 * should be sufficient for: <br>
 *  int8_t <br>
 * uint8_t <br>
 *  int16_t <br>
 * uint16_t <br>
 *  int32_t <br>
 * uint32_t <br>
 */
template<class T>
inline uint32_t hashCode(T t)
{
    return static_cast<uint32_t>(t);
}

/// @brief specialization for uint64_t
inline uint32_t hashCode(uint64_t ui64)
{
    // just add the lower to the higer 32 bit
    // perhaps there is a smarter way to do this
    uint32_t high = ui64 >> 32;
    uint32_t low  = ui64 & 0x00000000FFFFFFFF;

    return low * 31 + high;
}

/// @brief specialization for int64_t
inline uint32_t hashCode(int64_t i64)
{
    return hashCode( static_cast<uint64_t>(i64) );
}

/// @brief specialization for float
inline uint32_t hashCode(float f)
{
    // this assumes sizeof(float) == sizeof(uint32_t)
    float* pf = const_cast<float*>(&f);
    return *reinterpret_cast<uint32_t*>(pf);
}

/// @brief specialization for double
inline uint32_t hashCode(double d)
{
    // this assumes sizeof(double) == sizeof(uint64_t)
    double* pd = const_cast<double*>(&d);
    return hashCode( *reinterpret_cast<uint64_t*>(pd) );
}

/**
 * The hash code for a String object is computed as: <br>
 * s[0]*31^(n-1) + s[1]*31^(n-2) + ... + s[n-1] <br>
 * using int arithmetic, where s[i] is the ith character of the string,
 * n is the length of the string, and ^ indicates exponentiation.
 * (The hash value of the empty string is zero.)
 * This is the same formula used in Java for String. <br>
 * This is not a specialization, just overloading.
 */
uint32_t hashCode(const std::string& s)
{
    uint32_t result = 0;
    uint32_t factor = 1;
    size_t n = s.length();

    // this assumes size_t is unsigned
    for (size_t i = n-1; i < n; --i) // iterate backwards over the string
    {
        result += s[i] * factor;

        // calculate new factor
        uint32_t oldFactor = factor;
        factor <<= 5;// factor *= 32
        factor -= oldFactor;
        // -> factor *= 32
    }

    return result;
}

/**
 * @brief Specialization for C strings
 * The hash code is computed as in the specialization for std::string,
 * but in reverse order: <br>
 * s[0] + s[1]*31^1 + ... + s[n-1]*31^(n-1)
 */
uint32_t hashCode(const char* s)
{
    uint32_t result = 0;
    uint32_t factor = 1;

    // this assumes size_t is unsigned
    for (size_t i = 0; s[i] != 0; ++i)
    {
        result += s[i] * factor;

        // calculate new factor
        uint32_t oldFactor = factor;
        factor <<= 5;// factor *= 32
        factor -= oldFactor;
        // -> factor *= 32
    }

    return result;
}

//------------------------------------------------------------------------------

/**
 * This standard hash function implements the multiplication method for hashing: <br>
 * h(numSlots, hashCode) = floor( numSlots * (hashCode * A mod 1) ) <br>
 * "hashCode A mod 1" means the fractional part of hashCode * A, that is, hashCode * A - floor(hashCode * A) <br>
 * A is constant and is set to (sqrt(5) - 1) / 2 as Knuth suggests. <br>
 * <br>
 * In order to use this function object,
 * an appropriate uint32_t hashCode(const T& t) function must exist
 */
template<class T>
struct HashFunc
{
    enum {
        /**
         * ((sqrt(5) - 1) / 2) * 2^32 <br>
         * aka: golden ratio * 2^(sizeof(int32_t))
         */
        s = 2654435769
    };

    inline uint32_t operator() (size_t numSlots, const T& t) {
        // (hashCode * s) % numSlots; numSlots is guaranteed to be a power of 2
        return (hashCode(t) * s) & !numSlots;
    }
};

//------------------------------------------------------------------------------

/**
 * This container implents a class similar to std::map
 * but uses hashing instead of a binary tree interally. <br>
 * Values are searched by Keys. Besides a hash function object must be given and
 * a function object to test keys for equality.
 */
template
<
    class Key,                          /// for this key is searched
    class Value,                        /// this is the actual data the HashMap holds
    class Hash  = HashFunc<Key>,        /// this is a function object which capsulates the hash function
    class Equal = std::equal_to<Key>    /// this is a function object which capsulates a predicate whether two keys a equal
>
class HashMap
{
private:

    typedef List< std::pair<Key, Value> > SlotList;

    size_t      tableSize_;
    size_t      size_; // current size of the container = number of items in the HashMap
    SlotList*   table_;

    size_t      first_; // index of the first element in the table_
    size_t      last_;  // index of the last element in the table_

public:

    /**
    * Bidirectional iterator for the HashMap.
    */
    class iterator
    {
        friend class HashMap<Key, Value, Hash, Equal>;

    private:

        typedef std::list< std::pair<Key, Value> > SlotList;

        HashMap<Key, Value, Hash, Equal>* hashMap_;
        size_t index_;
        typename SlotList::iterator iter_;

        iterator(HashMap<Key, Value, Hash, Equal>* hashMap, size_t index, typename std::list< std::pair<Key, Value> >::iterator& iter)
            : hashMap_(hashMap)
            , index_(index)
            , iter_(iter)
        {}

    public:

        iterator() {}

        ///  preincrement operator
        iterator& operator ++ ()
        {
            ++iter_;
            if (iter_ != hashMap_->table_[index_].end() )
                return *this;

            ++index_;
            for (; index_ < hashMap_->tableSize_; ++index_) {
                if ( !hashMap_->table_[index_].empty() ) {
                    iter_ = hashMap_->table_[index_].begin();
                    return *this;
                }
            }

            // no element found
            *this = hashMap_->end();
            return *this;
        };

        ///  preincrement operator
        iterator& operator -- ();

        /// postincrement operator
        iterator operator ++ (int)
        {
            iterator iter(*this);
            this->operator ++ ();
            return iter;
        }

        /// postincrement operator
        iterator operator -- (int)
        {
            iterator iter(*this);
            this->operator ++ ();
            return iter;
        }

        bool operator == (const iterator& iter)
        {
            // is this the end?
            if ( iter.index_ == size_t(0 - 1) )
            {
                if ( index_ == size_t(0 - 1) )
                    return true;
                else
                    return false;
            }

            // no? - then do a normal check
            return (index_ == iter.index_) && (hashMap_ == iter.hashMap_) && (iter_ == iter.iter_);
        }

        bool operator != (const iterator& iter)
        {
            // is this the end?
            if ( index_ == std::numeric_limits<size_t>::max() && iter.index_ == std::numeric_limits<size_t>::max() )
                    return false;

            // no? - then do a normal check
            return (index_ != iter.index_) || (hashMap_ != iter.hashMap_) || (iter_ != iter.iter_);
        }

        std::pair<Key, Value>* operator->()
        {
            return &*iter_;
        }

        std::pair<Key, Value>& operator*()
        {
            return *iter_;
        }
    };

    /**
     * Creates a HashMap. You can define the size of the internal table.
     * @param tableSize the size of the internal table
     */
    explicit HashMap(size_t tableSize = 128)
        : tableSize_(tableSize)
        , size_(0) // no elements
        , first_(0)
        , last_(0 - 1)
    {
        table_ = new SlotList[tableSize];
    }

    /// cleans up
    ~HashMap()
    {
        delete[] table_;
    }

    /**
     * Resizes the internal array. All iterators are invalid after this operation.
     * @param newSize   size of the new table which is increased
     *                  to the next power of 2 if it is not already a power of 2
    */
    void rehash(size_t newSize);

    /**
     * Inserts a new item to the hash map.
     * @param p item to be inserted
     */
    std::pair<iterator, bool> insert(const std::pair<Key, Value>& p)
    {
        size_t index = static_cast<size_t>( Hash()(tableSize_, p.first) );

        // check whether the key is already in the map.
        typename SlotList::iterator iter;
        for (iter = table_[index].begin(); iter != table_[index].end(); ++iter)
        {
            if ( Equal()(iter->first, p.first) )
                return std::make_pair( iterator(this, index, iter), false ); // nothing inserted
        }
        // not in the map - so insert...

        // update first_ and last_ indices if necessary
        first_ = index > first_ ? index : first_;
        last_  = index < first_ ? index : first_;

        ++size_;

        if (size_ == tableSize_)
            rehash(tableSize_ << 1);

        table_[index].push_back(p);

        return std::make_pair( iterator(this, index, --table_[last_].end()), true );
    }

    /**
     * Finds a value by a given key.
     * @param key the key
     */
    iterator find(const Key& key) {
        size_t index = static_cast<size_t>( Hash()(tableSize_, key) );

        for (typename SlotList::iterator iter = table_[index].begin(); iter != table_[index].end(); ++iter)
        {
            if ( Equal()(iter->first, key) )
            {
                return iterator(this, index, iter);
            }
        }

        return end();
    }

    /// returns an iterator to the beginning
    iterator begin()
    {
        // begin is the first item in this container
        iterator iter;
        iter.hashMap_ = this;
        iter.index_ = first_;
        iter.iter_ = table_[first_].begin();

        return iter;
    }

    /// returns an iterator at the end thus it points to invalid data
    iterator end()
    {
        // end is
        iterator iter;
        iter.hashMap_ = this;
        iter.index_ = size_t(0 - 1);

        return iter;
    }

    /// Returns the number of items this container currently holds
    size_t size() const
    {
        return size_;
    }

    /// Returns whether this container is empty
    bool empty() const
    {
        return size_ == 0;
    }

    /// Returns the size of the internal table
    size_t tableSize() const
    {
        return tableSize_;
    }
};

/*
    non-inline implementations
*/

// template<class Key, class Value, class Hash, class Equal>
// HashMap<Key, Value, Hash, Equal>::iterator
// HashMap<Key, Value, Hash, Equal>::iterator::operator ++ () {
//     return *this;
// }

template<class Key, class Value, class Hash, class Equal>
void HashMap<Key, Value, Hash, Equal>::rehash(size_t newSize)
{
    // increase to the next power of 2
//     size_t tmp = newSize;
//     int bitCount = 0;

//     for (int i = 0; i < sizeof(tmp); ++i) {
//         tmp >> sizeof(tmp) - 1 - i;
//     }

    // create new table
    SlotList* newTable = new SlotList[newSize];
    first_ = 0;
    last_ = newSize;

    // copy values
    for (size_t i = 0; i < tableSize_; ++i)
    {
        if (table_[i].empty())
            continue;

        // iterate over all entries of the list
        for (typename SlotList::iterator iter = table_[i].begin(); iter != table_[i].end(); ++iter)
            table_[ Hash()(tableSize_, iter->first) ].push_back(*iter);
    }

    // destroy old table and set table_ to new pointer
    delete[] table_;
    table_ = newTable;
}

#endif // SWIFT_HASHMAP_H
