#include "me/stacklayout.h"

namespace me {

/*
 * constructor and destructor
 */

StackLayout::StackLayout(size_t numStackPlaces)
    : memSlot2Size_(0)
    , places_(numStackPlaces)
    , slotCounters_(numStackPlaces)
{
    // init all slot counters with zero
    for (size_t i = 0; i < slotCounters_.size(); ++i)
        slotCounters_[i] = 0; 
}

/*
 * further methods
 */

void StackLayout::insertColor(size_t place, int color)
{
    Color2Slot::iterator iter = places_[place].find(color);

    // check whether color has already been inserted
    if ( iter == places_[place].end() )
    {
        // no -> so insert with a new slot
        places_[place][color] = slotCounters_[place]++;
    }
}

void StackLayout::appendMem(int size)
{
    memSlot2Size_.push_back(size);
}

} // namespace me
