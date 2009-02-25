#ifndef ME_STACK_LAYOUT_H
#define ME_STACK_LAYOUT_H

#include <map>
#include <vector>

namespace me {

// forward declarations
struct MemVar;

class StackLayout
{
public:

    typedef std::vector<MemVar*> Color2MemVar;
    /// Maps color to size.
    Color2MemVar color2MemVar_;

    typedef std::map<int, int> Color2Slot;
    typedef std::vector<Color2Slot> Places;

    Places places_;

    typedef std::vector<int> SlotCounters;
    SlotCounters slotCounters_;

    /*
     * constructor and destructor
     */

    StackLayout(size_t numStackPlaces);

    /*
     * further methods
     */

    void insertColor(size_t place, int color);

    void appendMem(MemVar* memVar);
};

} // namespace me

#endif // ME_STACK_LAYOUT_H
