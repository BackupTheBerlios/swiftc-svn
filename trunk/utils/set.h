#ifndef SWIFT_SET_H
#define SWIFT_SET_H

#include <algorithm>
#include <set>
#include <vector>

template<class T>
class Set : public std::set<T>
{
public:

    inline bool contains(const T& t) const
    {
        return find(t) != this->end();
    }

    typedef std::vector<T> ResultVec;
     
    inline ResultVec difference(const Set<T>& s) const
    {
        std::vector<T> result( this->size() );

        result.erase( 
                std::set_difference( 
                    this->begin(), this->end(), 
                    s.begin(), s.end(), 
                    result.begin() ), 
                result.end() );

        return result;
    }
};
#endif // SWIFT_SET_H
