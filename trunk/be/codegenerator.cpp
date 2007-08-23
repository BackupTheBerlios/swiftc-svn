#include "codegenerator.h"

/*
    init statics
*/

Spiller* CodeGenerator::spiller_ = 0;

/*
    methods
*/

void CodeGenerator::genCode()
{
    spill();
    color();
    coalesce();
}

void CodeGenerator::spill()
{
    spiller_->spill();
}

void CodeGenerator::color()
{
    // start with the first true basic block
    colorRecursive( cfg_->entry_->succ_.first()->value_ );
}

void CodeGenerator::colorRecursive(BBNode* bb)
{
    std::cout << bb->value_->name() << std::endl;
}

void CodeGenerator::coalesce()
{
}
