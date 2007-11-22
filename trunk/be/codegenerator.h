#ifndef SWIFT_CODEGENERATOR_H
#define SWIFT_CODEGENERATOR_H

#include <fstream>

#include "me/functab.h"

#include "be/spiller.h"

// forward declarations
struct CFG;

struct CodeGenerator
{
    static Spiller* spiller_;

    Function*       function_;
    CFG*            cfg_;
    std::ofstream&  ofs_;

    CodeGenerator(std::ofstream& ofs, Function* function)
        : function_(function)
        , cfg_(&function->cfg_)
        , ofs_(ofs)
    {}

    void genCode();

private:

/*
    liveness stuff
*/
    /// Knows during the liveness analysis which basic blocks have already been visted
    typedef std::set<BasicBlock*> BBSet;
    BBSet walked_;

    void livenessAnalysis();
    void liveOutAtBlock(BBNode* bbNode, PseudoReg* var);
    void liveInAtInstr (InstrNode instr, PseudoReg* var);
    void liveOutAtInstr(InstrNode instr, PseudoReg* var);

    void spill();

    void color();
    void colorRecursive(BBNode* bb);

    void coalesce();
};

#endif // SWIFT_CODEGENERATOR_H
