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

/**
 * Function has in, inout and out going parameters and, of course, an identifier.
*/
struct Function
{
    std::string* id_;
    int counter_;
    InstrList instrList_;

    RegMap in_;
    RegMap inout_;
    RegMap out_;
    RegMap vars_;

    BBList bbList_;

    Function(std::string* id)
        : id_(id)
        , counter_(0)
    {}
    ~Function();

    void findBasicBlocks();
    void dump(std::ofstream& ofs);
};

struct FunctionTable
{
    typedef std::map<std::string*, Function*, StringPtrCmp> FunctionMap;

    FunctionMap functions_;
    Function* current_;
    std::string filename_;

    FunctionTable(const std::string& filename)
        : filename_(filename)
    {}
    ~FunctionTable();

    Function* insertFunction(std::string* id);

    inline void insert(PseudoReg* reg);
    PseudoReg* newTemp(PseudoReg::RegType regType);
    PseudoReg* lookupReg(int regNr);

    void appendInstr(InstrBase* instr);
    void findBasicBlocks();

    void dump(const std::string& extension = ".ssa");
};

typedef FunctionTable FuncTab;
extern FuncTab* functab;

#endif // SWIFT_FUNCTAB_H
