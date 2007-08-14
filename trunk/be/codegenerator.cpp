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
    calcIG();
    spill();
    color();
    coalesce();
    destructSSA();
}

void CodeGenerator::calcIG()
{
}

void CodeGenerator::spill()
{
    spiller_->spill();
}

void CodeGenerator::color()
{
}

void CodeGenerator::coalesce()
{
}

void CodeGenerator::destructSSA()
{
}
