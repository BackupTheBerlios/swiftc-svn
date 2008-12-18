#ifndef ME_LIVE_RANGE_SPLITTING_H
#define ME_LIVE_RANGE_SPLITTING_H

#include "me/codepass.h"
#include "me/functab.h"

namespace me {

class LiveRangeSplitting : public CodePass
{
private:

    RDUMap phis_;

public:

    /*
     * constructor and destructor
     */

    LiveRangeSplitting(Function* function);
    virtual ~LiveRangeSplitting();

    /*
     * further methods
     */

    virtual void process();

private:

    void liveRangeSplit(InstrNode* instrNode, BBNode* bbNode);
};

} // namespace me

#endif // ME_LIVE_RANGE_SPLITTING_H

