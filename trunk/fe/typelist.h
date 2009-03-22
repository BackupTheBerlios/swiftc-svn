#ifndef SWIFT_TYPE_LIST_H
#define SWIFT_TYPE_LIST_H

#include <string>
#include <vector>

namespace swift {

class Type;

class TypeItem
{
public:

    TypeItem(const Type* type, int modifier)
        : type_(type)
        , modifier_(modifier)
    {}
    
    const Type* getType() const;
    bool isReadOnly() const;

    std::string toString() const;

private:

    /*
     * data
     */

    const Type* type_;
    const int modifier_;
};

//------------------------------------------------------------------------------

class TypeList : public std::vector<TypeItem>
{
public:

    /*
     * further methods
     */

    std::string toString() const;
}; 

}// namespace swift

#endif // SWIFT_TYPE_LIST_H
