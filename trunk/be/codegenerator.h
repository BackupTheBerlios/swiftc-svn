#ifndef BE_CODEGENERATOR_H
#define BE_CODEGENERATOR_H

#include <fstream>

#include "me/functab.h"

#include "be/spiller.h"

// forward declarations
namespace me {
    struct CFG;
}

namespace be {

struct IGraph;

struct CodeGenerator
{
    static Spiller* spiller_;

    me::Function*   function_;
    me::CFG*        cfg_;
    std::ofstream&  ofs_;
#ifdef SWIFT_DEBUG
    IGraph*         ig_;
#endif // SWIFT_DEBUG

    /**
     * If vars have to be spilled this counter is used to create new numbers in order to identify
     * same vars.
     */
    int spillCounter_;

    typedef std::map<me::Reg*, me::Reg*> SpillMap;
    /// This set knows for each spilled var the corresponding memory var. 
    SpillMap spillMap_;

    typedef std::set<me::InstrNode*> InstrSet;
    InstrSet spills_;
    InstrSet reloads_;
    
    /*
     * constructor and destructor
     */

    CodeGenerator(std::ofstream& ofs, me::Function* function);
#ifdef SWIFT_DEBUG
    ~CodeGenerator();
#endif // SWIFT_DEBUG

    /*
     * methods
     */

    void genCode();

private:

    /*
     * liveness analysis
     */
    
    /// Knows during the liveness analysis which basic blocks have already been visted.
    typedef std::set<me::BasicBlock*> BBSet;
    BBSet walked_;

    /** 
     * @brief Performs the liveness analysis.
     *
     * Invokes \a liveOutAtBlock, \a liveInAtInstr and \a liveOutAtInstr.
     */
    void livenessAnalysis();
    void liveOutAtBlock(me::BBNode* bbNode,   me::Reg* var);
    void liveInAtInstr (me::InstrNode* instr, me::Reg* var);
    void liveOutAtInstr(me::InstrNode* instr, me::Reg* var);

    /*
     * register allocation
     */

    /// Performs the spilling in all basic blocks.
    void spill();

    /** 
     * @brief Performs the spilling in one basic block.
     * 
     * @param bbNode The basic block which should be spilled.
     */
    void spill(me::BBNode* bbNode);

    void reconstructSSAForm(me::RegSet* defs);

    void findDef(me::InstrNode* instrNode, size_t p, me::RegSet* defs);
    
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
    int distance(me::BBNode* bbNode, me::Reg* reg, me::InstrNode* instrNode);
    
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
    int distanceRec(me::BBNode* bbNode, me::Reg* reg, me::InstrNode* instrNode);

    void insertReload();

    void insertSpill();

    /*
     * coloring
     */

    typedef std::set<int> Colors;
    static int findFirstFreeColorAndAllocate(Colors& colors);

    void color();
    void colorRecursive(me::BBNode* bb);

    /*
     * coalesing
     */

    void coalesce();
};

} // namespace be

#endif // BE_CODEGENERATOR_H
