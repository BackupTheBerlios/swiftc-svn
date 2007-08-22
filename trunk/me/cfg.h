#ifndef SWIFT_CFG_H
#define SWIFT_CFG_H

#include <map>
#include <stack>

#include "utils/graph.h"

#include "me/basicblock.h"

// forward declarations
struct Function;


struct CFG : public Graph<BasicBlock>
{
    BBNode** idoms_;

    Function* function_;
    InstrList& instrList_;

    typedef std::map<InstrList::Node*, BBNode*> LabelNode2BBNodeMap;
    /// with this data structure we can quickly find a BB with a given starting label
    LabelNode2BBNodeMap labelNode2BBNode_;

    typedef std::map<int, BBNode*> FirstOccurance;
    FirstOccurance firstOccurance_;

    BBNode* entry_;
    BBNode* exit_;

/*
    constructor and destructor
*/

    CFG(Function* function);
    ~CFG();

/*
    methods
*/

    void calcCFG();
    void calcDomTree();
    BBNode* intersect(BBNode* b1, BBNode* b2);
    void calcDomFrontier();
    void placePhiFunctions();
    void renameVars();
    void rename(BBNode* bb, std::stack<PseudoReg*>* names);

/*
    dump methods
*/

    virtual std::string name() const;
    std::string dumpIdoms() const;
    std::string dumpDomChildren() const;
    std::string dumpDomFrontier() const;
};

#endif // SWIFT_CFG_H