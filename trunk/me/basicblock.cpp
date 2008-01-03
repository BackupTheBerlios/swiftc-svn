#include "basicblock.h"

#include <sstream>

#include "utils/assert.h"

namespace me {

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
        // print leading label central and with a horizontal line
        oss << '\t' << begin_->value_->toString() << "|\\" << std::endl; // print instruction

        // for all instructions in this basic block except the first and the last LabelInstr
        for (InstrNode instrIter = begin_->next(); instrIter != end_; instrIter = instrIter->next())
            oss << '\t' << instrIter->value_->toString() << "\\l\\" << std::endl; // print instruction
    }

    return oss.str();
}

} // namespace me
