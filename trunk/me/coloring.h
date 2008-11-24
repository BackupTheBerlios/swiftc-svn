#ifndef ME_COLORING_H
#define ME_COLORING_H

#include <fstream>
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

    Coloring(Function* function);

    /*
     * methods
     */

    virtual void process();

private:

    typedef std::set<int> Colors;
    static int findFirstFreeColorAndAllocate(Colors& colors);
    void color();
    void colorRecursive(BBNode* bb);
};

} // namespace me

#endif // ME_COLORING_H
