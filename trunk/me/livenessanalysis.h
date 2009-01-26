#ifndef ME_LIVENESS_ANALYSIS
#define ME_LIVENESS_ANALYSIS

#include <fstream>

#include "me/codepass.h"
#include "me/functab.h"

namespace me {

// forward declaration
class IGraph;

class LivenessAnalysis : public CodePass
{
private:

    /// Knows during the liveness analysis which basic blocks have already been visted.
    BBSet walked_;

#ifdef SWIFT_USE_IG
    IGraph* ig_;
#endif // SWIFT_USE_IG

public:

    /*
     * constructor and destructor
     */

    LivenessAnalysis(Function* function);
#ifdef SWIFT_USE_IG
    ~LivenessAnalysis();
#endif // SWIFT_USE_IG

    /*
     * methods
     */

    /** 
     * @brief Performs the liveness analysis.
     *
     * Invokes \a liveOutAtBlock, \a liveInAtInstr and \a liveOutAtInstr.
     */
    virtual void process();

private:

    void liveOutAtBlock(BBNode* bbNode,   Reg* var);
    void liveInAtInstr (InstrNode* instrNode, Reg* var);
    void liveOutAtInstr(InstrNode* instrNode, Reg* var);
};

} // namespace me

#endif // ME_LIVENESS_ANALYSIS
