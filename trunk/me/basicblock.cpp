#include "basicblock.h"

#include <sstream>

std::string BasicBlock::toString() const
{
    std::ostringstream oss;
    oss << begin_->toString() << " -> " << end_->toString();

    return oss.str();
}

void BasicBlock::toDot(std::ofstream& ofs) const
{
    for (const BBList::Node* iter = succ_.first(); iter != succ_.sentinel(); iter = iter->next())
        ofs << '\t' << begin_->toString() << " -> " << iter->value_->begin_->toString() << std::endl;
}
