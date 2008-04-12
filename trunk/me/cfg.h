#ifndef ME_CFG_H
#define ME_CFG_H

#include <map>
#include <stack>

#include "utils/graph.h"

#include "me/basicblock.h"

namespace me {

// forward declarations
struct Function;

//------------------------------------------------------------------------------

/**
 * Inherit from this class if you want to get callbacks for errors which are
 * discoverd during control flow analysis.
*/
struct CFErrorHandler
{
    enum CFError
    {
        VAR_NOT_INITIALIZED,
        TODO
    };

    /*
     * destructor
     */

    virtual ~CFErrorHandler();

    /*
     * further methods
     */

    virtual void error(CFError cfError, int varNr) = 0;
};

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
     * With this data structure we can quickly find a BB with a given starting label.
     * The last LabelInstr found in InstrList_ marks the end of the last BasicBlock
     * with the second but last LabelInstr as beginning of that BasicBlock.
     */
    LabelNode2BBNodeMap labelNode2BBNode_;

    typedef std::map<int, BBNode*> FirstOccurance;
    FirstOccurance firstOccurance_;

    CFErrorHandler* cfErrorHandler_;

    /*
     * constructor and destructor
     */

    CFG(Function* function);
    ~CFG();

    /*
     * graph creation
     */

    void calcCFG();
    void calcDomTree();
    BBNode* intersect(BBNode* b1, BBNode* b2);
    void calcDomFrontier();

    /*
     * error handling related to control flow
     */

    void installCFErrorHandler(CFErrorHandler* cfErrorHandler);

    /*
     * phi functions
     */

    void placePhiFunctions();
    void renameVars();
    void rename(BBNode* bb, std::vector< std::stack<Reg*> >& names);

    /*
     * def-use-chains
     */

    void calcDef();
    void calcUse();
    void calcUse(Reg* var, BBNode* bb);

    /*
     * dump methods
     */

    virtual std::string name() const;
    std::string dumpIdoms() const;
    std::string dumpDomChildren() const;
    std::string dumpDomFrontier() const;

    /*
     * further methods
     */

    BBNode* findBBNode(InstrNode* instrNode);
};

#define CFG_RELATIVES_EACH(iter, relatives) \
    for (me::CFG::Relative* (iter) = (relatives).first(); (iter) != (relatives).sentinel(); (iter) = (iter)->next())

} // namespace me

#endif // ME_CFG_H
