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
     * constructor
     */

    Coloring(Function* function, int typeMask, const Colors& reservoir);

    /*
     * methods
     */

    virtual void process();

private:

    void color();
    void colorRecursive(BBNode* bbNode);
    void colorConstraintedInstr(InstrNode* instrNode, Colors& colors);
};

} // namespace me

#endif // ME_COLORING_H
