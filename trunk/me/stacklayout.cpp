#include "me/stacklayout.h"

#include <iostream>

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
    // fill itemSize_ for each place
    for (size_t i = 0; i < arch->getNumStackPlaces(); ++i)
        places_[i].itemSize_ = arch->getItemSize(i);

    /*
     * calculate offset_ for each place
     */

    if ( color2MemSlot_.empty() )
        places_[0].offset_ = 0;
    else
    {
        // firstPlaceOffset = align( lastMemSlot-offset + lastMemSlot-size, first-item-size )
        MemSlot& lastMemSlot= *color2MemSlot_.rbegin();
        places_[0].offset_ = arch->calcAlignedStackOffset(
                lastMemSlot.offset_ + lastMemSlot.memVar_->memory_->size_, places_[0].itemSize_);
    }

    swiftAssert( !places_.empty(), "places_ must not be empty" );

    for (size_t i = 1; i < places_.size(); ++i)
    {
        // newOffset = align( pred-offset + pre-item-size * pre-num-items, current-item-size )
        Place& pre = places_[i - 1];
        places_[i].offset_ = arch->calcAlignedStackOffset(
                pre.offset_ + pre.itemSize_ * pre.color2Slot_.size(), 
                places_[i].itemSize_);
    }

    // calc the size of the stack frame
    Place& lastPlace = *places_.rbegin();
    size_ = lastPlace.offset_ + lastPlace.itemSize_ * lastPlace.color2Slot_.size();

    // and align properly
    size_ = Arch::align( size_, arch->getStackAlignment() );
}

} // namespace me
