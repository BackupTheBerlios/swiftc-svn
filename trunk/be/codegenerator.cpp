#include "codegenerator.h"

#include <typeinfo>

#include "me/functab.h"

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
    /*
        start with the first true basic block
        and perform a post-order walk of the dominator tree
    */
    colorRecursive( cfg_->entry_->succ_.first()->value_ );
}

void CodeGenerator::colorRecursive(BBNode* bb)
{
    // for each instruction -> start with the first instruction which is followed by the leading LabelInstr
    for (InstrList::Node* iter = bb->value_->begin_->next(); iter != bb->value_->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        // for each var on the right hand side
        if ( typeid(*instr) == typeid(AssignInstr) )
        {
            AssignInstr* ai = (AssignInstr*) instr;
        }

        // for the left hand side
    }

    // for each child of bb in the dominator tree
    for (BBList::Node* iter = bb->value_->domChildren_.first(); iter != bb->value_->domChildren_.sentinel(); iter = iter->next())
    {
        BBNode* domChild = iter->value_;

        // omit special exit node
        if ( domChild->value_->isExit() )
            continue;

        colorRecursive(domChild);
    }
}

void CodeGenerator::coalesce()
{
}
