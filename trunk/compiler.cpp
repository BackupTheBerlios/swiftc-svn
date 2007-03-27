#include "compiler.h"

#include <iostream>
#include <fstream>
#include <typeinfo>

#include "im/ssa.h"
#include "utils/assert.h"

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

// bool Compiler::genCode()
// {
//     for (InstrList::reverse_iterator iter = instrlist.rbegin(); iter != instrlist.rend(); ++iter)
//     {
//     }
// }

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

    for (InstrList::Node* n = instrlist.first(); n != instrlist.sentinel(); n = n->next())
    {
        std::cout << n->value_->toString() << std::endl;

        // check whether it is a TagInstr
        TagInstr* tagInstr = dynamic_cast<TagInstr*>(n->value_);
        if ( tagInstr )
        {
            // update scoping of the symtab
            TagInstr* tagInstr = (TagInstr*) n->value_;

            if ( tagInstr->enter_ )
                tagInstr->enter();
            else
                tagInstr->leave();

            // everything done with this instruction so process the next one
            continue;
        }

        swiftAssert( dynamic_cast<ExprInstr*>(n->value_), "The Instruction found is neither a TagInstr nor a ExprInstr." );
        ExprInstr* exprInstr = (ExprInstr*) n->value_;
        exprInstr->genCode(ofs);
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
