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

#include "me/constpool.h"

#include "utils/box.h"

namespace me {

//------------------------------------------------------------------------------

// init global
ConstPool* constpool = 0;

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

void ConstPool::insert(UInt128 value) 
{
    uint128_.insert( std::make_pair(value, counter_++) );
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
