#include "me/constpool.h"

namespace me {

//------------------------------------------------------------------------------

// init global
ConstPool* constpool = 0;

//------------------------------------------------------------------------------

// internal helper
template<class From, class To>
To convert(From from)
{
    union Converter
    {
        From from_;
        To to_;
    };

    Converter conv;
    conv.from_ = from;

    return conv.to_;
}

//------------------------------------------------------------------------------

ConstPool::ConstPool()
    : counter_(1)
{}

/*
 * boolean
 */

void ConstPool::insert(bool value) 
{
    uint8_.insert( 
            std::make_pair( convert<bool, uint8_t>(value), counter_++) );
}

/*
 * integer types
 */

void ConstPool::insert(int8_t value) 
{
    uint8_.insert( 
            std::make_pair( convert<int8_t, uint8_t>(value), counter_++) );
}

void ConstPool::insert(int16_t value) 
{
    uint16_.insert( 
            std::make_pair( convert<int16_t, uint16_t>(value), counter_++) );
}

void ConstPool::insert(int32_t value) 
{
    uint32_.insert( 
            std::make_pair( convert<int32_t, uint32_t>(value), counter_++) );
}

void ConstPool::insert(int64_t value) 
{
    uint64_.insert( 
            std::make_pair( convert<int64_t, uint64_t>(value), counter_++) );
}

/*
 * unsigned integer types
 */

void ConstPool::insert(uint8_t value) 
{
    uint8_.insert( std::make_pair(value, counter_++) );
}

void ConstPool::insert(uint16_t value) 
{
    uint16_.insert( std::make_pair(value, counter_++) );
}

void ConstPool::insert(uint32_t value) 
{
    uint32_.insert( std::make_pair(value, counter_++) );
}

void ConstPool::insert(uint64_t value) 
{
    uint64_.insert( std::make_pair(value, counter_++) );
}

/*
 * real types
 */

void ConstPool::insert(float value) 
{
    uint32_.insert( 
            std::make_pair( convert<float, uint32_t>(value), counter_++) );
}

void ConstPool::insert(double value) 
{
    uint64_.insert( 
            std::make_pair( convert<double, uint64_t>(value), counter_++) );
}


} // namespace me
