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
    destructor
*/

    virtual ~CFErrorHandler();

/*
    further methods
*/

    virtual void error(CFError cfError, int varNr) = 0;
};

//------------------------------------------------------------------------------

struct CFG : public Graph<BasicBlock>
{
    BBNode* idoms_;

    Function* function_;
    InstrList& instrList_;

    typedef std::map<InstrNode, BBNode> LabelNode2BBNodeMap;
    /// with this data structure we can quickly find a BB with a given starting label
    LabelNode2BBNodeMap labelNode2BBNode_;

    typedef std::map<int, BBNode> FirstOccurance;
    FirstOccurance firstOccurance_;

    BBNode entry_;
    BBNode exit_;

    CFErrorHandler* cfErrorHandler_;

/*
    constructor and destructor
*/

    CFG(Function* function);
    ~CFG();

/*
    graph creation
*/

    void calcCFG();
    void calcDomTree();
    BBNode intersect(BBNode b1, BBNode b2);
    void calcDomFrontier();

/*
    error handling related to control flow
*/

    void installCFErrorHandler(CFErrorHandler* cfErrorHandler);

/*
    phi functions
*/

    void placePhiFunctions();
    void renameVars();
    void rename(BBNode bb, std::vector< std::stack<Reg*> >& names);

/*
    def-use-chains
*/

    void calcDef();
    void calcUse();
    void calcUse(Reg* var, BBNode bb);

/*
    dump methods
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
