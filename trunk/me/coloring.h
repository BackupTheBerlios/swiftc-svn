#ifndef ME_COLORING_H
#define ME_COLORING_H

#include <set>

#include "me/codepass.h"
#include "me/forward.h"

namespace me {

typedef std::set<int> Colors;

class Coloring : public CodePass
{
private: 

    int typeMask_;
    const Colors reservoir_;

public:

    /*
     * constructor
     */

    Coloring(Function* function, int typeMask, const Colors& reservoir);

    /*
     * methods
     */

    virtual void process();

private:

    int findFirstFreeColorAndAllocate(Colors& colors);
    void color();
    void colorRecursive(BBNode* bbNode);
    void colorConstraintedInstr(InstrNode* instrNode, Colors& colors);
};

} // namespace me

#endif // ME_COLORING_H
