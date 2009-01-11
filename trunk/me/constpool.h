#ifndef ME_CONST_POOL_H
#define ME_CONST_POOL_H

#include <map>

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

class ConstPool
{
private:
    
    int counter_;

public:

    typedef std::map< uint8_t, int>  UInt8Map;
    typedef std::map<uint16_t, int> UInt16Map;
    typedef std::map<uint32_t, int> UInt32Map;
    typedef std::map<uint64_t, int> UInt64Map;

    UInt8Map  uint8_;
    UInt16Map uint16_;
    UInt32Map uint32_;
    UInt64Map uint64_;


    ConstPool();

    void insert(bool value);

    void insert(int8_t value);
    void insert(int16_t value);
    void insert(int32_t value);
    void insert(int64_t value);

    void insert(uint8_t value);
    void insert(uint16_t value);
    void insert(uint32_t value);
    void insert(uint64_t value);

    void insert(float value);
    void insert(double value);
};

extern ConstPool* constpool;

} // namespace me

#endif // ME_CONST_POOL_H
