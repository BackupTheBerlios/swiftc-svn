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

#ifndef ME_CONST_POOL_H
#define ME_CONST_POOL_H

#include "utils/box.h"
#include "utils/map.h"
#include "utils/types.h"

namespace me {

#define UINT8MAP_EACH(iter) \
    for (me::ConstPool::UInt8Map::iterator (iter) = me::constpool->uint8_.begin(); (iter) != me::constpool->uint8_.end(); ++(iter))

#define UINT16MAP_EACH(iter) \
    for (me::ConstPool::UInt16Map::iterator (iter) = me::constpool->uint16_.begin(); (iter) != me::constpool->uint16_.end(); ++(iter))

#define UINT32MAP_EACH(iter) \
    for (me::ConstPool::UInt32Map::iterator (iter) = me::constpool->uint32_.begin(); (iter) != me::constpool->uint32_.end(); ++(iter))

#define UINT64MAP_EACH(iter) \
    for (me::ConstPool::UInt64Map::iterator (iter) = me::constpool->uint64_.begin(); (iter) != me::constpool->uint64_.end(); ++(iter))

#define UINT128MAP_EACH(iter) \
    for (me::ConstPool::UInt128Map::iterator (iter) = me::constpool->uint128_.begin(); (iter) != me::constpool->uint128_.end(); ++(iter))

class ConstPool
{
private:
    
    int counter_;

public:

    typedef Map< uint8_t, int>   UInt8Map;
    typedef Map<uint16_t, int>  UInt16Map;
    typedef Map<uint32_t, int>  UInt32Map;
    typedef Map<uint64_t, int>  UInt64Map;
    typedef Map<UInt128,  int> UInt128Map;

    UInt8Map    uint8_;
    UInt16Map   uint16_;
    UInt32Map   uint32_;
    UInt64Map   uint64_;
    UInt128Map uint128_;

    ConstPool();

    void insert(bool value);

    void insert(int8_t  value);
    void insert(int16_t value);
    void insert(int32_t value);
    void insert(int64_t value);

    void insert(uint8_t  value);
    void insert(uint16_t value);
    void insert(uint32_t value);
    void insert(uint64_t value);
    void insert(UInt128  value);

    void insert(float value);
    void insert(double value);
};

extern ConstPool* constpool;

} // namespace me

#endif // ME_CONST_POOL_H
