#ifndef ME_SPILLER_H
#define ME_SPILLER_H

#include <fstream>
#include <map>
#include <vector>

#include "me/codepass.h"
#include "me/functab.h"

// forward declarations
namespace me {

/** 
 * @brief This CodePass inserts necessary spills and reloads.
 *
 * This class in independet from any architecture and can be adopted by the
 * back-end.
 */
class Spiller : public CodePass
{
private:

    /*
     * types and member vars
     */

    size_t numRegs_;
    int typeMask_;

    /**
     * If vars have to be spilled this counter is used to create new numbers in order to identify
     * same vars.
     */
    int spillCounter_;

    /// Reg -> Mem
    typedef std::map<Reg*, Reg*> SpillMap;

    /// This set knows for each spilled var the first memory var. 
    SpillMap spillMap_;


    /// BBNode -> RegSet
    typedef std::map<BBNode*, RegSet> BB2RegSet;

    /** 
     * Find the set of registers of a corresponding basic block which is going
     * into that basic block in \em real registers.
     */
    BB2RegSet in_;

    /** 
     * Find the set of registers of a corresponding basic block which is going
     * out of that basic block in \em real registers.
     */
    BB2RegSet out_;

    /// Keeps registers with their definitions und uses.
    typedef std::map<Reg*, RegDefUse*> RDUMap;

    /**
     * Contains for each reg inside: <br>
     * - defs: several x = spill(reg) <br>
     *  or exactly one reg = phispill(___) <br>
     * - uses: several y = reload(x) with x = spillMap_[reg] <br>
     *   perhaps one   y = phispill(x, ___) with x in res(defs)
     *
     * reconstructSSAForm is first called with this Map so all args of the
     * reloads are rewired correclty.
     */
    RDUMap spills_;

    /**
     * Contains for each reg inside: <br>
     * - defs:  several   y = reload(x) with x = spillMap_[reg] <br>
     *          one     reg = ___ <br>
     * - uses:  several   y = spills(reg) <br>
     *          several ___ = reg <br>
     *
     * reconstructSSAForm is called with this map second so all args of the
     * spills are rewired correclty.
     */
    RDUMap reloads_;
    
     struct Substitute
     {
         PhiInstr* phi_;
         size_t arg_;
 
         Substitute() {}
         Substitute(PhiInstr* phi, size_t arg)
             : phi_(phi)
             , arg_(arg)
         {}
     };
 
     typedef std::vector<Substitute> Substitutes;
     Substitutes substitutes_;

     typedef std::vector<DefUse> PhiSpills;
     PhiSpills phiSpills_;

    typedef std::vector<DefUse> LaterReloads;
    LaterReloads laterReloads_;
    
public:

    /*
     * constructor and destructor
     */

    Spiller(Function* function, size_t numRegs, int typeMask);
    ~Spiller();

    /*
     * methods
     */

    /**
     * Performs the spilling in all basic blocks locally and gloablly 
     * and repairs SSA form.
     */
    virtual void process();

private:

    /*
     * insertion of spills and reloads
     */

    /** 
     * @brief Inserts a newly created Spill instruction in \p bbNode of register
     * \p reg after instruction \p appendto.
     * 
     * @param bbNode Basic block of the to be created spill.
     * @param reg Register to be spilled.
     * @param appendTo Append to which instruction?
     * 
     * @return The newly created memory pseudo register where \p reg is spilled
     * to.
     */
    Reg* insertSpill(BBNode* bbNode, Reg* reg, InstrNode* appendTo);

    /** 
     * @brief Inserts a newly created Reload instruction in \p bbNode 
     * of register \p reg after instruction \p appendto.
     * 
     * @param bbNode Basic block of the to be created reload.
     * @param reg Register to be reloaded.
     * @param appendTo Append to which instruction?
     */
    void insertReload(BBNode* bbNode, Reg* reg, InstrNode* appendTo);

    /*
     * distance calculation via Belady
     */

    /// Needed for sorting via the distance method.
    struct RegAndDistance 
    {
        Reg* reg_;
        int distance_;

        /*
         * constructors
         */

        RegAndDistance() {}

        RegAndDistance(Reg* reg, int distance)
            : reg_(reg)
            , distance_(distance)
        {}

        /// copy constructor
        RegAndDistance(const RegAndDistance& rd)
            : reg_(rd.reg_)
            , distance_(rd.distance_)
        {}

        /*
         * operator
         */

        /// needed by std::sort
        bool operator < (const RegAndDistance& r) const
        {
            return distance_ > r.distance_; // sort farest first
        }
    };


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
    int distance(BBNode* bbNode, Reg* reg, InstrNode* instrNode, BBSet walked);
    
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
    int distanceRec(BBNode* bbNode, Reg* reg, InstrNode* instrNode, BBSet walked);

    /*
     * local spilling
     */

    typedef std::multiset<RegAndDistance> DistanceBag;

    void discardFarest(DistanceBag& ds);

#ifndef SWIFT_DEBUG
    static 
#endif // SWIFT_DEBUG
    Reg* regFind(DistanceBag& ds, Reg* reg);

    /** 
     * @brief Performs the spilling locally in one basic block.
     * 
     * @param bbNode The basic block which should be spilled.
     */
    void spill(BBNode* bbNode);

    /*
     * global combining
     */

    /** 
     * @brief Combines the spilling results globally.
     * 
     * @param bbNode Currntly visited basic block.
     */
    void combine(BBNode* bbNode);

    /** 
     * @brief 
     *
     * Inserts a Spill at the end of a basic block b wich holds this property: <br>
     * b is first basic block which dominats \p bbNode where reg in is out_[b] 
     * 
     * @param reg 
     * @param bbNode 
     */
    void insertSpillIfNecessarry(Reg* reg, BBNode* bbNode);
};

} // namespace me

#endif // ME_SPILLER_H
