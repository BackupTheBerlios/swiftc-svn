#ifndef SWIFT_FUNCTAB_H
#define SWIFT_FUNCTAB_H

#include <fstream>
#include <iostream>
#include <map>
#include <stack>

#include "utils/list.h"
#include "utils/stringhelper.h"

#include "me/basicblock.h"
#include "me/cfg.h" // TODO move to .cpp
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

    size_t  numBBs_;
    CFG     cfg_;

    Function(std::string* id)
        : id_(id)
        , regCounter_(1)    // 0 is reserved for literals
        , indexCounter_(0)
        , numBBs_(2)        // every function does at least have an entry and an exit node
        , cfg_(this)
    {}
    ~Function();

    inline void insert(PseudoReg* reg);

    /**
     * This method creates a new temp PseudoReg.
     * @param regType the type of the PseudoReg
    */
    PseudoReg* newTemp(PseudoReg::RegType regType);

#ifdef SWIFT_DEBUG

    /**
     * This method creates a new temp PseudoReg.
     * @param regType the type of the PseudoReg
    */
    PseudoReg* newTemp(PseudoReg::RegType regType, std::string* id);

    /**
     * This method creates a new var PseudoReg.
     * @param regType the type of the PseudoReg
     * @param varNr the varNr of the var; must be positive.
     * @param id the name of the original var
    */
    PseudoReg* newVar(PseudoReg::RegType regType, int varNr, std::string* id);

#else // SWIFT_DEBUG

    /**
     * This method creates a new var PseudoReg.
     * @param regType the type of the PseudoReg
     * @param varNr the varNr of the var; must be positive.
    */
    PseudoReg* newVar(PseudoReg::RegType regType, int varNr);

#endif // SWIFT_DEBUG

    void dumpSSA(std::ofstream& ofs);
    void dumpDot(const std::string& baseFilename);
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

    /**
     * This method creates a new temp PseudoReg.
     * @param regType the type of the PseudoReg
    */
    PseudoReg* newTemp(PseudoReg::RegType regType);

#ifdef SWIFT_DEBUG

    /**
     * This method creates a new temp PseudoReg.
     * @param regType the type of the PseudoReg
    */
    PseudoReg* newTemp(PseudoReg::RegType regType, std::string* id);

    /**
     * This method creates a new var PseudoReg.
     * @param regType the type of the PseudoReg
     * @param varNr the varNr of the var; must be positive.
     * @param id the name of the original var
    */
    PseudoReg* newVar(PseudoReg::RegType regType, int varNr, std::string* id);

#else // SWIFT_DEBUG

    /**
     * This method creates a new var PseudoReg.
     * @param regType the type of the PseudoReg
     * @param varNr the varNr of the var; must be positive.
    */
    PseudoReg* newVar(PseudoReg::RegType regType, int varNr);

#endif // SWIFT_DEBUG

    PseudoReg* lookupReg(int regNr);

    void appendInstr(InstrBase* instr);
    void appendInstrNode(InstrNode node);
    void buildUpME();

    void dumpSSA();
    void dumpDot();

    void genCode();
};

typedef FunctionTable FuncTab;
extern FuncTab* functab;

#endif // SWIFT_FUNCTAB_H
