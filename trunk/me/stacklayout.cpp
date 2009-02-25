#include "me/stacklayout.h"

#include "me/arch.h"
#include "me/op.h"
#include "me/struct.h"

namespace me {

/*
 * constructor and destructor
 */

StackLayout::StackLayout(size_t numStackPlaces)
    : color2MemSlot_(0)
    , memSlotsSize_(0)
    , places_(numStackPlaces)
{
    // init all slot counters with zero
    for (size_t i = 0; i < places_.size(); ++i)
        places_[i].counter_ = 0; 
}

/*
 * further methods
 */

void StackLayout::insertColor(size_t place, int color)
{
    Color2Slot::iterator iter = places_[place].color2Slot_.find(color);

    // check whether color has already been inserted
    if ( iter == places_[place].color2Slot_.end() )
    {
        // no -> so insert with a new slot
        places_[place].color2Slot_[color] = places_[place].counter_++;
    }
}

void StackLayout::appendMemVar(MemVar* memVar)
{
    int offset = arch->calcAlignedStackOffset(memSlotsSize_, memVar->memory_->size_);
    memSlotsSize_ = offset + memVar->memory_->size_;
    MemSlot ms = {memVar, offset};
    color2MemSlot_.push_back(ms);

    // set color to the index in the vector
    memVar->color_ = color2MemSlot_.size() - 1;
}

void StackLayout::arangeStackLayout()
{
    // TODO this is should be done in an arch independent way

}

} // namespace me
