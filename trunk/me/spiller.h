/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef ME_SPILLER_H
#define ME_SPILLER_H

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

    /// Var -> Mem
    typedef Map<Var*, Var*> SpillMap;

    /// This set knows for each spilled var the first memory var. 
    SpillMap spillMap_;

    /// BBNode -> VarSet
    typedef Map<BBNode*, VarSet> BB2VarSet;

    /** 
     * Find the set of registers of a corresponding basic block which is going
     * into that basic block in \em real registers.
     */
    BB2VarSet in_;

    /** 
     * Find the set of registers of a corresponding basic block which is going
     * out of that basic block in \em real registers.
     */
    BB2VarSet out_;

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
    VDUMap spills_;

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
    VDUMap reloads_;
    
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
     * @brief Inserts a newly created Spill instruction in \p bbNode of \p var
     * after instruction \p appendto.
     * 
     * @param bbNode Basic block of the to be created spill.
     * @param var Var to be spilled.
     * @param appendTo Append to which instruction?
     * 
     * @return The newly created memory pseudo register where \p reg is spilled
     * to.
     */
    Var* insertSpill(BBNode* bbNode, Var* var, InstrNode* appendTo);

    /** 
     * @brief Inserts a newly created Reload instruction in \p bbNode 
     * of \p var after instruction \p appendto.
     * 
     * @param bbNode Basic block of the to be created reload.
     * @param var Var to be reloaded.
     * @param appendTo Append to which instruction?
     */
    void insertReload(BBNode* bbNode, Var* var, InstrNode* appendTo);

    /*
     * distance calculation via Belady
     */

    /// Needed for sorting via the distance method.
    struct VarAndDistance 
    {
        Var* var_;
        int distance_;

        /*
         * constructors
         */

        VarAndDistance() {}

        VarAndDistance(Var* var, int distance)
            : var_(var)
            , distance_(distance)
        {}

        /// copy constructor
        VarAndDistance(const VarAndDistance& rd)
            : var_(rd.var_)
            , distance_(rd.distance_)
        {}

        /*
         * operator
         */

        /// needed by std::sort
        bool operator < (const VarAndDistance& v) const
        {
            return distance_ > v.distance_; // sort farest first
        }
    };

    int distance(BBNode* bbNode, Var* var, InstrNode* instrNode);

    /** 
     * @brief Calculates the distance of \p var from \p instrNode to its next use.
     *
     * \p bbNode is the basic block of \p instrNode.
     *
     * This formula ist used:
@verbatim
                                   / 0, if var is used at instrNode
distance(bbNode, var, instrNode) = |
                                   \ distanceRec(bbNode, var, instrNode), otherwise
@endverbatim
     * 
     * @param bbNode The \a BasicBlock which holds the \p instrNode.
     * @param var The var which distance to its next use should be found.
     * @param instrNode The starting InstrNode of the search.
     * 
     * @return The distance.  is used as "infinity".
     */
    int distanceHere(BBNode* bbNode, Var* var, InstrNode* instrNode, BBSet walked);
    
    /** 
     * @brief This is a helper for \a distance. 
     *
     * It calulates recursivly the distance.
     * 
     * This formula is used:
     * 
@verbatim
                                      / infinity, if var is not live at instrNode
distanceRec(bbNode, var, instrNode) = |
                                      | 1  +     min         distance(bbNode, var, instrNode'), otherwise 
                                      \  instrNode' \in succ(instrNode)
@endverbatim
     *
     * @param bbNode The \a BasicBlock which holds the \p instrNode.
     * @param var The var which distance to its next use should be found.
     * @param instrNode The starting InstrNode of the search.
     * 
     * @return 
     */
    int distanceRec(BBNode* bbNode, Var* var, InstrNode* instrNode, BBSet walked);

    /*
     * local spilling
     */

    typedef std::multiset<VarAndDistance> DistanceBag;

    void discardFarest(DistanceBag& ds);

#ifndef SWIFT_DEBUG
    static // a type check is only made in the debug version 
#endif // SWIFT_DEBUG
    Var* varFind(DistanceBag& ds, Var* var);

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

    void insertSpillIfNecessarry(Var* var, BBNode* bbNode);
};

} // namespace me

#endif // ME_SPILLER_H
