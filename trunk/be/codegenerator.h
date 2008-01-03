#ifndef BE_CODEGENERATOR_H
#define BE_CODEGENERATOR_H

#include <fstream>

#include "me/functab.h"

#include "be/spiller.h"

// forward declarations
namespace me {
    struct CFG;
}

namespace be {

struct IGraph;

struct CodeGenerator
{
    static Spiller* spiller_;

    me::Function*   function_;
    me::CFG*        cfg_;
    std::ofstream&  ofs_;
#ifdef SWIFT_DEBUG
    IGraph*         ig_;
#endif // SWIFT_DEBUG

/*
    constructor and destructor
*/
    CodeGenerator(std::ofstream& ofs, me::Function* function);
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
    typedef std::set<me::BasicBlock*> BBSet;
    BBSet walked_;

    void livenessAnalysis();
    void liveOutAtBlock(me::BBNode bbNode,   me::PseudoReg* var);
    void liveInAtInstr (me::InstrNode instr, me::PseudoReg* var);
    void liveOutAtInstr(me::InstrNode instr, me::PseudoReg* var);

/*
    register allocation
*/
    void spill();

    typedef std::set<int> Colors;
    static int findFirstFreeColorAndAllocate(Colors& colors);

    void color();
    void colorRecursive(me::BBNode bb);

    void coalesce();
};

} // namespace be

#endif // BE_CODEGENERATOR_H
