#include "compiler.h"

#include <iostream>
#include <fstream>
#include <typeinfo>

#include "utils/assert.h"
#include "me/scopetable.h"
#include "me/ssa.h"

bool Compiler::parse()
{
    parserInit(&syntaxTree_);
    yyparse();

    return true;
}

bool Compiler::analyze()
{
    symtab.reset();
    symtab.enterModule();

    bool result = syntaxTree_.rootModule_->analyze();

    symtab.leaveModule();

    return result;
}

bool Compiler::genCode()
{
    // now the syntax tree is not needed anymore
//     delete syntaxTree_.rootModule_;

    std::ofstream ofs("out.yasm");

    // stream startup code to file
    ofs << "; startup code\n";
    ofs << "bits 64\n";
    ofs << "section .text\n";
    ofs << "global _start\n\n";
    ofs << "_start:\n";

    // prepare for iteration of the abstract language code (alc):
    symtab.reset();

    Scope::InstrList& instrList = scopetab.currentScope()->instrList_;
    for (Scope::InstrList::Node* n = instrList.first(); n != instrList.sentinel(); n = n->next())
    {
        std::cout << n->value_->toString() << std::endl;

        // check whether it is a DummyInstr
        if ( typeid(*n->value_) == typeid(DummyInstr) )
        {
            // this is just a dummy instruction so process the next one
            continue;
        }

        // check whether it is a NOPInstr
        if ( typeid(*n->value_) == typeid(NOPInstr) )
        {
            // this is just a "no operation" instruction so process the next one
            continue;
        }

        // check whether it is a EnterScopeInstr
        EnterScopeInstr* enterScopeInstr = dynamic_cast<EnterScopeInstr*>(n->value_);
        if ( enterScopeInstr )
        {
            // update scoping of the scopetab
            enterScopeInstr->updateScoping();
            // everything done with this instruction so process the next one
            continue;
        }

        swiftAssert( dynamic_cast<PseudoRegInstr*>(n->value_),
            "The Instruction found should be a PseudoRegInstr" );
        PseudoRegInstr* instr = (PseudoRegInstr*) n->value_;
        instr->genCode(ofs);
    }

    // stream ending code to file
    ofs << "\n; ending code\n";
    ofs << "\t\tmov rax, 1\n";
    ofs << "\t\txor rbx, rbx\n";
    ofs << "\t\tint 80h\n";

    // finish
    ofs.close();

    return true;
}

bool Compiler::optimize()
{
    return true;
}

bool Compiler::buildAssemblyCode()
{
    return true;
}

std::string Compiler::toString()
{
    return syntaxTree_.rootModule_->toString();
}
