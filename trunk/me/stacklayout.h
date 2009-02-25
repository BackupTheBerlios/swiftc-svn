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
    /*
     * constructor and destructor
     */

    StackLayout(size_t numStackPlaces);

    /*
     * further methods
     */

    void insertColor(size_t place, int color);

    void appendMem(MemVar* memVar);

    void arangeStackLayout();

public:

    struct MemSlot
    {
        MemVar* memVar_;
        int offset_;
    };

    typedef std::vector<MemSlot> Color2MemSlot;
    Color2MemSlot color2MemSlot_;

    int memSlotsSize_;

    typedef std::map<int, int> Color2Slot;

    struct Place
    {
        Color2Slot color2Slot_;
        int counter_;
    };
    
    typedef std::vector<Place> Places;
    Places places_;
};

} // namespace me

#endif // ME_STACK_LAYOUT_H
