#ifndef SWIFT_CAST_H
#define SWIFT_CAST_H

#include "utils/assert.h"

template<class T, class U>
T* cast(U* u)
{
    swiftAssert( dynamic_cast<T*>(u), "cast not possible" );
    return static_cast<T*>(u);
}

template<class T, class U>
const T* cast(const U* u)
{
    swiftAssert( dynamic_cast<const T*>(u), "cast not possible" );
    return static_cast<const T*>(u);
}

#endif // SWIFT_CAST_H
