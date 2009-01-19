#ifndef ME_FUNCTAB_H
#define ME_FUNCTAB_H

#include <fstream>
#include <iostream>
#include <map>
#include <stack>

#include "utils/list.h"
#include "utils/stringhelper.h"

#include "me/basicblock.h"
#include "me/cfg.h" 
#include "me/op.h"
#include "me/ssa.h"

namespace me {

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
    CFG  cfg_;

    /// Indicates whether a LivenessAnalysis CodePass has already been performed.
    bool firstLiveness_;

    /// Indicates whether a DefUseCalc CodePass has already been performed.
    bool firstDefUse_;

    InstrNode* lastLabelNode_;


    InstrList instrList_;

    RegMap in_;
    RegMap inout_;
    RegMap out_;
    RegMap vars_;


    typedef std::map<Reg*, int> Reg2Color;
    Reg2Color reg2Color_;

    /*
     * constructor and destructor
     */

    Function(std::string* id);
    ~Function();

    /*
     * further methods
     */

#ifdef SWIFT_DEBUG

    /**
     * @brief This method creates a new temp Reg.
     * This is a var which is guaranteed to be defined only once.
     *
     * @param type The type of the Reg.
     * @param id The name of the original var.
     */
    Reg* newSSA(Op::Type type, std::string* id = 0);

    /**
     * @brief This method creates a new var Reg.
     * This will be transforemd to SSA form later on.
     *
     * @param type The type of the Reg.
     * @param varNr The varNr of the var; must be positive.
     * @param id The name of the original var.
     */
    Reg* newVar(Op::Type type, int varNr, std::string* id);

    /**
     * @brief This method creates a new var in a memory location.
     *
     * @param type The type of the Reg.
     * @param varNr The varNr of the var; must be positive.
     * @param id The name of the original var.
     */
    Reg* newMem(Op::Type type, int varNr, std::string* id = 0);

#else // SWIFT_DEBUG

    /**
     * @brief This method creates a new temp Reg.
     * This is a var which is guaranteed to be defined only once.
     *
     * @param type The type of the Reg.
     */
    Reg* newSSA(Op::Type type);

    /**
     * @brief This method creates a new var Reg.
     * This will be transforemd to SSA form later on.
     *
     * @param type the type of the Reg.
     * @param varNr the varNr of the var; must be positive.
     */
    Reg* newVar(Op::Type type, int varNr);

    /**
     * @brief This method creates a new var in a memory location.
     *
     * @param type The type of the Reg.
     * @param varNr The varNr of the var; must be positive.
     */
    Reg* newMem(Op::Type type, int varNr);

#endif // SWIFT_DEBUG

    inline void insert(Reg* reg);

    /*
     * dump methods
     */

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

#ifdef SWIFT_DEBUG

    /**
     * @brief This method creates a new Reg which is only defined once.
     *
     * @param type the type of the Reg
     */
    Reg* newSSA(Op::Type type, std::string* id = 0);

    /**
     * @brief This method creates a new var Reg.
     *
     * @param type the type of the Reg
     * @param varNr the varNr of the var; must be positive.
     * @param id the name of the original var
     */
    Reg* newVar(Op::Type type, int varNr, std::string* id);

    /**
     * @brief This method creates a new memory variable.
     *
     * @param type the type of the Reg
     * @param varNr the varNr of the var; must be positive.
     * @param id the name of the original var
     */
    Reg* newMem(Op::Type type, int varNr, std::string* id = 0);

#else // SWIFT_DEBUG

    /**
     * @brief This method creates a new temp Reg.
     *
     * @param type the type of the Reg
     */
    Reg* newSSA(Op::Type type);

    /**
     * @brief This method creates a new var Reg.
     * 
     * @param type the type of the Reg
     * @param varNr the varNr of the var; must be positive.
     */
    Reg* newVar(Op::Type type, int varNr);

    /**
     * @brief This method creates a new memory variable.
     *
     * @param type the type of the Reg
     * @param varNr the varNr of the var; must be positive.
     */
    Reg* newMem(Op::Type type, int varNr);

#endif // SWIFT_DEBUG

    Reg* lookupReg(int varNr);

    void appendInstr(InstrBase* instr);
    void appendInstrNode(InstrNode* node);
    InstrNode* getLastLabelNode();

    void buildUpME();

    void dumpSSA();
    void dumpDot();

    void genCode();
};

typedef FunctionTable FuncTab;
extern FuncTab* functab;

} // namespace me

#endif // ME_FUNCTAB_H
