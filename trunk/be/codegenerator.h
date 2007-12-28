#ifndef SWIFT_CODEGENERATOR_H
#define SWIFT_CODEGENERATOR_H

#include <fstream>

#include "me/functab.h"

#include "be/spiller.h"

// forward declarations
struct CFG;
struct IGraph;

struct CodeGenerator
{
    static Spiller* spiller_;

    Function*       function_;
    CFG*            cfg_;
    std::ofstream&  ofs_;
#ifdef SWIFT_DEBUG
    IGraph*         ig_;
#endif // SWIFT_DEBUG

/*
    constructor and destructor
*/
    CodeGenerator(std::ofstream& ofs, Function* function);
#ifdef SWIFT_DEBUG
    ~CodeGenerator();
#endif // SWIFT_DEBUG

/*
    methods
*/

    void genCode();

private:

/*
    liveness analysis
*/
    /// Knows during the liveness analysis which basic blocks have already been visted
    typedef std::set<BasicBlock*> BBSet;
    BBSet walked_;

    void livenessAnalysis();
    void liveOutAtBlock(BBNode bbNode, PseudoReg* var);
    void liveInAtInstr (InstrNode instr, PseudoReg* var);
    void liveOutAtInstr(InstrNode instr, PseudoReg* var);

/*
    register allocation
*/
    void spill();

    typedef std::set<int> Colors;
    static int findFirstFreeColorAndAllocate(Colors& colors);

    void color();
    void colorRecursive(BBNode bb);

    void coalesce();
};

#endif // SWIFT_CODEGENERATOR_H
