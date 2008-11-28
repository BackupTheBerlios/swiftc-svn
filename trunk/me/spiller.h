#ifndef ME_SPILLER_H
#define ME_SPILLER_H

#include <fstream>
#include <map>
#include <vector>

#include "me/codepass.h"
#include "me/functab.h"

#include "be/spiller.h"

// forward declarations
namespace me {

class Spiller : public CodePass
{
    /**
     * If vars have to be spilled this counter is used to create new numbers in order to identify
     * same vars.
     */
    int spillCounter_;

    /// Reg -> Mem
    typedef std::map<Reg*, Reg*> SpillMap;
    /// This set knows for each spilled var the corresponding memory var. 
    SpillMap spillMap_;

    typedef std::map<BBNode*, RegSet> BB2RegSet;
    BB2RegSet in_;
    BB2RegSet out_;

    typedef std::set<InstrNode*> InstrSet;
    InstrSet spills_;
    InstrSet reloads_;

    struct PhiSpilledReload
    {
        BasicBlock* bb_;
        Reg* reg_;
        InstrNode* appendTo_;

        PhiSpilledReload(BasicBlock* bb, Reg* reg, InstrNode* appendTo)
            : bb_(bb)
            , reg_(reg)
            , appendTo_(appendTo)
        {}
    };
    typedef std::vector<PhiSpilledReload> PhiSpilledReloads;
    PhiSpilledReloads phiSpilledReloads_;
    
    /*
     * constructor 
     */

public:

    Spiller(Function* function);

    /*
     * methods
     */

    /// Performs the spilling in all basic blocks.
    virtual void process();

private:

    Reg* insertSpill(BasicBlock* bb, Reg* reg, InstrNode* appendTo);
    void insertReload(BasicBlock* bb, Reg* reg, InstrNode* appendTo, bool first);

    /** 
     * @brief Calculates the distance of \p reg from \p instrNode to its next use.
     *
     * \p bbNode is the basic block of \p instrNode.
     *
     * This formula ist used:
@verbatim
                                   / 0, if reg is used at instrNode
distance(bbNode, reg, instrNode) = |
                                   \ distanceRec(bbNode, reg, instrNode), otherwise
@endverbatim
     * 
     * @param bbNode The \a BasicBlock which holds the \p instrNode.
     * @param reg The register which distance to its next use should be found.
     * @param instrNode The starting InstrNode of the search.
     * 
     * @return The distance.  is used as "infinity".
     */
    int distance(BBNode* bbNode, Reg* reg, InstrNode* instrNode);
    
    /** 
     * @brief This is a helper for \a distance. 
     *
     * It calulates recursivly the distance.
     * 
     * This formula is used:
     * 
@verbatim
                                      / infinity, if reg is not live at instrNode
distanceRec(bbNode, reg, instrNode) = |
                                      | 1  +     min         distance(bbNode, reg, instrNode'), otherwise 
                                      \  instrNode' \in succ(instrNode)
@endverbatim
     *
     * @param bbNode The \a BasicBlock which holds the \p instrNode.
     * @param reg The register which distance to its next use should be found.
     * @param instrNode The starting InstrNode of the search.
     * 
     * @return 
     */
    int distanceRec(BBNode* bbNode, Reg* reg, InstrNode* instrNode);

    /** 
     * @brief Performs the spilling in one basic block.
     * 
     * @param bbNode The basic block which should be spilled.
     */
    void spill(BBNode* bbNode);

    /** 
     * @brief Combines the spilling results globally.
     * 
     * @param bbNode Currntly visited basic block.
     */
    void combine(BBNode* bbNode);
};

} // namespace me

#endif // ME_SPILLER_H
