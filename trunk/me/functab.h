#ifndef SWIFT_FUNCTAB_H
#define SWIFT_FUNCTAB_H

#include <fstream>
#include <iostream>
#include <map>
#include <stack>

#include "utils/list.h"
#include "utils/stringptrcmp.h"

#include "me/basicblock.h"
#include "me/pseudoreg.h"
#include "me/ssa.h"

//------------------------------------------------------------------------------

typedef void (*ProcessBBFunc)(BasicBlock*);

/**
 * Function has in, inout and out going parameters and, of course, an identifier.
*/
struct Function
{
    std::string* id_;
    int counter_;
    bool reachedValue_;

    InstrList instrList_;

    RegMap in_;
    RegMap inout_;
    RegMap out_;
    RegMap vars_;

    BBList bbList_;

    BasicBlock** doms_;
    size_t      numBBs_;

    typedef std::map<InstrList::Node*, BasicBlock*> LabelNode2BBMap;
    /// with this data structure we can quickly find a BB with a given starting label
    LabelNode2BBMap labelNode2BB_;

    Function(std::string* id)
        : id_(id)
        , counter_(0)
        , reachedValue_(true)
    {}
    ~Function();

    void calcCFG();
    void calcDomTree();
    BasicBlock* intersect(BasicBlock* b1, BasicBlock* b2);

    void dumpSSA(std::ofstream& ofs);
    void dumpDot(const std::string& baseFilename);

    BasicBlock* getEntry()
    {
        return bbList_.first()->value_;
    }
    BasicBlock* getExit()
    {
        return bbList_.last()->value_;
    }

    /// traverses the cfg in reverse post order for each node func is executed
    void reversePostOrderWalk(ProcessBBFunc process)
    {
        r_reversePostOrderWalk(process, getExit());
        // toggle reachedValue_
        reachedValue_ = !reachedValue_;
    }
    void r_reversePostOrderWalk(ProcessBBFunc process, BasicBlock* bb);
};

struct FunctionTable
{
    std::string filename_;

    typedef std::map<std::string*, Function*, StringPtrCmp> FunctionMap;
    FunctionMap functions_;
    Function*   current_;

    FunctionTable(const std::string& filename)
        : filename_(filename)
    {}
    ~FunctionTable();

    Function* insertFunction(std::string* id);

    inline void insert(PseudoReg* reg);
    PseudoReg* newTemp(PseudoReg::RegType regType, int varNr = PseudoReg::TEMP);
    PseudoReg* lookupReg(int regNr);

    void appendInstr(InstrBase* instr);
    void appendInstrNode(InstrList::Node* node);
    void calcCFG();
    void calcDomTree();

    void dumpSSA();
    void dumpDot();
};

typedef FunctionTable FuncTab;
extern FuncTab* functab;

#endif // SWIFT_FUNCTAB_H
