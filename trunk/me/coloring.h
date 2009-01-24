#ifndef ME_COLORING_H
#define ME_COLORING_H

#include "utils/set.h"

#include "me/codepass.h"
#include "me/forward.h"

namespace me {

typedef Set<int> Colors;

class Coloring : public CodePass
{
private: 

    int typeMask_;
    const Colors reservoir_;

public:

    /*
     * constructors
     */

    /// Use this one for coloring of memory locations.
    Coloring(Function* function);
    Coloring(Function* function, int typeMask, const Colors& reservoir);

    /*
     * methods
     */

    virtual void process();

private:

    /*
     * memory location coloring
     */

    void colorRecursiveMem(BBNode* bbNode);
    int getFreeMemColor(Colors& colors);

    /*
     * register coloring
     */

    void colorRecursive(BBNode* bbNode);
    void colorConstraintedInstr(InstrNode* instrNode, RegSet& alreadyColored);
};

} // namespace me

#endif // ME_COLORING_H
