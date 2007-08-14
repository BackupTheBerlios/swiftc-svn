#ifndef SWIFT_CODEGENERATOR_H
#define SWIFT_CODEGENERATOR_H

#include <fstream>

#include "me/functab.h"

#include "be/spiller.h"

struct CodeGenerator
{
    static Spiller* spiller_;

    Function* function_;
    std::ofstream& ofs_;

    CodeGenerator(std::ofstream& ofs, Function* function)
        : function_(function)
        , ofs_(ofs)
    {}

    void genCode();

//private:

    /// calculates the interference graph
    void calcIG();
    void spill();
    void color();
    void coalesce();
    void destructSSA();
};

#endif // SWIFT_CODEGENERATOR_H
