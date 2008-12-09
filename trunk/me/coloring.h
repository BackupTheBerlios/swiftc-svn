#ifndef ME_COLORING_H
#define ME_COLORING_H

#include <set>

#include "me/codepass.h"
#include "me/forward.h"

namespace me {

class Coloring : public CodePass
{
public:

    /*
     * constructor
     */

    Coloring(Function* function, int typeMask, Colors reservoir);

    /*
     * methods
     */

    virtual void process();

private:

    int typeMask_;
    const Colors reservoir_;

    int findFirstFreeColorAndAllocate(Colors& colors);
    void color();
    void colorRecursive(BBNode* bbNode);
    void colorConstraintedInstr(InstrNode* instrNode);
};

} // namespace me

#endif // ME_COLORING_H
