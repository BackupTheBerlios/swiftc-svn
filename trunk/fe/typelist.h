#ifndef SWIFT_TYPE_LIST_H
#define SWIFT_TYPE_LIST_H

#include <string>
#include <vector>

namespace swift {

class Type;

class TypeList : public std::vector<const Type*>
{
public:

    /*
     * further methods
     */

    std::string toString() const;
}; 

}// namespace swift

#endif // SWIFT_TYPE_LIST_H
