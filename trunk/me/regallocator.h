#ifndef ME_REG_ALLOCATOR_H
#define ME_REG_ALLOCATOR_H

#include <fstream>

#include "me/codepass.h"
#include "me/functab.h"

#include "be/spiller.h"

// forward declarations
namespace me {

class RegAllocator : public CodePass
{
    /**
     * If vars have to be spilled this counter is used to create new numbers in order to identify
     * same vars.
     */
    int spillCounter_;

    typedef std::map<Reg*, Reg*> SpillMap;
    /// This set knows for each spilled var the corresponding memory var. 
    SpillMap spillMap_;

    typedef std::set<InstrNode*> InstrSet;
    InstrSet spills_;
    InstrSet reloads_;
    
    /*
     * constructor 
     */

public:

    RegAllocator(Function* function);

    /*
     * methods
     */

    virtual void process();

private:

    /*
     * register allocation
     */

    /// Performs the spilling in all basic blocks.
    void spill();

    typedef std::map<BBNode*, RegSet> BB2RegSet;

    /** 
     * @brief Performs the spilling in one basic block.
     * 
     * @param bbNode The basic block which should be spilled.
     */
    void spill(BBNode* bbNode, BB2RegSet& in, BB2RegSet& out);
    
public:
    /** 
     * @brief This is used to represent an infinity distance
     * 
     * @return "infinity"
     */
    static int infinity() 
    {
        return std::numeric_limits<int>::max();
    }

private:

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
};

} // namespace me

#endif // ME_REG_ALLOCATOR_H
