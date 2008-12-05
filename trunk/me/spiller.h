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

    /**
     * If vars have to be spilled this counter is used to create new numbers in order to identify
     * same vars.
     */
    int spillCounter_;

    /// Reg -> Mem
    typedef std::map<Reg*, Reg*> SpillMap;

    /// This set knows for each spilled var the corresponding memory var. 
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

    struct Def
    {
        Reg* reg_;
        InstrNode* instrNode_;
        BBNode* bbNode_;

        Def() {}
        Def(Reg* reg, InstrNode* instrNode, BBNode* bbNode)
            : reg_(reg)
            , instrNode_(instrNode)
            , bbNode_(bbNode)
        {}
        Def(const Def& d)
            : reg_(d.reg_)
            , instrNode_(d.instrNode_)
            , bbNode_(d.bbNode_)
        {}
    };
    
    typedef List<Def> DefList;

    struct RegDefUse
    {
        DefList defs_;
        UseList uses_;
    };
    
    /// Keeps registers with their definitions und uses.
    typedef std::map<Reg*, RegDefUse*> RDUMap;

    /**
     * Contains each spill, i.e. each var defined by a spill 
     * and its use during a reload if applicable.
     */
    RDUMap spills_;

    /**
     * Contains each reload, its orignal definition and their uses
     */
    RDUMap reloads_;

    /// Remembers a Spill which remains to do during local spilling.
    struct PhiSpilledReload
    {
        BBNode* bbNode_;
        Reg* reg_;
        InstrNode* appendTo_;

        PhiSpilledReload(BBNode* bbNode, Reg* reg, InstrNode* appendTo)
            : bbNode_(bbNode)
            , reg_(reg)
            , appendTo_(appendTo)
        {}
    };

    /// std::vector of PhiSpilledReload.
    typedef std::vector<PhiSpilledReload> PhiSpilledReloads;

    /// Remembers spills which remain to do during local spilling.
    PhiSpilledReloads phiSpilledReloads_;
    
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
    
public:

    /*
     * constructor and destructor
     */

    Spiller(Function* function);
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
    void insertReload(BBNode* bbNode, Reg* reg, InstrNode* appendTo, bool first);

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
     * @brief Performs the spilling locally in one basic block.
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

    void reconstructSSAForm(RegDefUse* rdu);

    Reg* findDef(size_t i, InstrNode* instrNode, BBNode* bbNode, RegDefUse* rdu, BBSet& iDF);
};

} // namespace me

#endif // ME_SPILLER_H
