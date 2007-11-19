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
    livenessAnalysis();
    spill();
    color();
    coalesce();
}

/*
    liveness stuff
*/

void CodeGenerator::livenessAnalysis()
{
    // for each var
    REGMAP_EACH(iter, function_->vars_)
    {
        PseudoReg* var = iter->second;

        // for each use of var
        USELIST_EACH(iter, var->uses_)
        {
            DefUse& use = iter->value_;
            BBNode* bb = use.bbNode_;
            InstrBase* instr = use.instr_;

            if ( typeid(*instr) == typeid(PhiInstr) )
            {
                PhiInstr* phi = (PhiInstr*) phi;

                // find the predecessor basic block
                size_t i = 0;
                while (phi->args_[i] != var)
                    ++i;

                BBNode* pred = phi->sourceBBs_[i];

                // examine the found block
                liveOutAtBlock(pred, var);
            }
            else
                liveInAtInstr(instr, var);
        }
    }

    // clean up
    walked_.clear();
}

void CodeGenerator::liveOutAtBlock(BBNode* bb, PseudoReg* var)
{
}

void CodeGenerator::liveInAtInstr (InstrBase* instr, PseudoReg* var)
{
}

void CodeGenerator::liveInOutInstr(InstrBase* instr, PseudoReg* var)
{
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
