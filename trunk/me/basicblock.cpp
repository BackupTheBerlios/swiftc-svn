#include "basicblock.h"

#include <sstream>

#include "utils/assert.h"


std::string BasicBlock::name() const
{
    std::ostringstream oss;

    if ( isEntry() )
        oss << "ENTRY";
    else if ( isExit() )
        oss << "EXIT";
    else
        oss << begin_->value_->toString();

    return oss.str();
}

std::string BasicBlock::toString() const
{
    std::ostringstream oss;

    if (begin_ == 0)
        oss << "\tENTRY\\l\\" << std::endl;
    else if (end_ == 0)
        oss << "\tEXIT\\l\\" << std::endl;
    else
    {
        // for all instructions in this basic block except the last LabelInstr
        for (InstrList::Node* instrIter = begin_; instrIter != end_; instrIter = instrIter->next())
            oss << '\t' << instrIter->value_->toString() << "\\l\\" << std::endl; // print instruction
    }

    return oss.str();
}
