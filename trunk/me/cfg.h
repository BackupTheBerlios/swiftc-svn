#ifndef ME_CFG_H
#define ME_CFG_H

#include <map>
#include <stack>

#include "utils/graph.h"

#include "me/basicblock.h"

namespace me {

// forward declarations
struct Function;
struct RegDefUse;

//------------------------------------------------------------------------------

/** 
 * @brief This represents the control flow graph of a function.  
 *
 * Each CFG has exactly one entry node. This is the BasicBlock which is defined
 * by the first instruction -- which is a LabelInstr -- given in the 
 * \a instrList_. <br>
 * Furthermore it has exactly one exit node. This BasicBlock contains the last
 * instruction -- which is also a LabelInstr -- given in the \a instrList_.
 * No other instructions are in this BasicBlock. BasicBlock::end_ points to the
 * sentinel of this CFG's \a instrList_.
 */
struct CFG : public Graph<BasicBlock>
{
    Function* function_;
    InstrList& instrList_;

    BBNode* entry_;
    BBNode* exit_;


    BBNode** idoms_;

    typedef std::map<InstrNode*, BBNode*> LabelNode2BBNodeMap;

    /**
     * @brief With this data structure we can quickly find a BB with 
     * a given starting label.
     */
    LabelNode2BBNodeMap labelNode2BBNode_;

    typedef std::map<int, BBNode*> FirstOccurance;
    FirstOccurance firstOccurance_;

    /*
     * constructor and destructor
     */

    CFG(Function* function);
    ~CFG();

    /*
     * graph creation
     */

    void calcCFG();
    void eliminateCriticalEdges();
    void calcDomTree();
    BBNode* intersect(BBNode* b1, BBNode* b2);
    void calcDomFrontier();

    /*
     * phi functions
     */

    void placePhiFunctions();
    void renameVars();
    void rename(BBNode* bb, std::vector< std::stack<Reg*> >& names);

    /*
     * SSA form construction
     */

    /** 
     * @brief Invoke this method in order to construct SSA Form.
     *
     * Furthermore the dominator tree, the dominance frontier and the placement of phi functions
     * will be calculated. Vars will be properly renamed and the def and use information is
     * calculated at last.
     */
    void constructSSAForm();

    /*
     * further methods
     */

    BBSet calcIteratedDomFrontier(BBSet bbs);

    /** 
     * @brief Splits the basic block referenced by \p bb at \p instrNode.
     *
     * \p instrNode itself will be the first instruction after the leading
     * LabelInstr in the bottom basic block.
     * 
     * @param instrNode The instruction where to split
     * @param bbNode The basic block where \p instrNode belongs to.
     */
    void splitBB(me::InstrNode* instrNode, me::BBNode* bbNode);


    /*
     * SSA reconstruction and rewiring
     */

    void reconstructSSAForm(RegDefUse* rdu);

    Reg* findDef(size_t i, InstrNode* instrNode, BBNode* bbNode, RegDefUse* rdu, BBSet& iDF);

    /*
     * dump methods
     */

    virtual std::string name() const;
    std::string dumpIdoms() const;
    std::string dumpDomChildren() const;
    std::string dumpDomFrontier() const;
};

#define CFG_RELATIVES_EACH(iter, relatives) \
    for (me::CFG::Relative* (iter) = (relatives).first(); (iter) != (relatives).sentinel(); (iter) = (iter)->next())

} // namespace me

#endif // ME_CFG_H
