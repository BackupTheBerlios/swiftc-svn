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

struct Function;

/**
 * Function has in, inout and out going parameters and, of course, an identifier.
*/
struct Function
{
    std::string* id_;
    int regCounter_;
    size_t indexCounter_;

    InstrList instrList_;

    RegMap in_;
    RegMap inout_;
    RegMap out_;
    RegMap vars_;

    BasicBlock* entry_;
    BasicBlock* exit_;

    BasicBlock** bbs_;
    BasicBlock** idoms_;
    size_t       numBBs_;

    typedef std::map<InstrList::Node*, BasicBlock*> LabelNode2BBMap;
    /// with this data structure we can quickly find a BB with a given starting label
    LabelNode2BBMap labelNode2BB_;

    Function(std::string* id)
        : id_(id)
        , regCounter_(0)
        , indexCounter_(0)
        , numBBs_(2) // every function does at least have an entry and an exit node
    {}
    ~Function();

    void calcCFG();
    void calcDomTree();
    BasicBlock* intersect(BasicBlock* b1, BasicBlock* b2);
    void assignPostOrderNr(BasicBlock* bb);
    void calcDoms(BasicBlock* bb);

    void dumpSSA(std::ofstream& ofs);
    void dumpDot(const std::string& baseFilename);

    /// traverses the cfg in post order so the bbs_ array and nr_ are properly initialized
    void postOrderWalk(BasicBlock* bb);
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
