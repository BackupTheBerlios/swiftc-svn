#include "codegenerator.h"

#include <typeinfo>

#include "me/cfg.h"
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
            InstrBase* instr = use.instr_->value_;

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
                liveInAtInstr(use.instr_, var);
        }
    }

    // clean up
    walked_.clear();
}

void CodeGenerator::liveOutAtBlock(BBNode* bbNode, PseudoReg* var)
{
    BasicBlock* bb = bbNode->value_;

    // var is live-out at bb
    bb->liveOut_.insert( std::make_pair(var->regNr_, var) );

    // if bb not in var
    if ( walked_.find(bb) == walked_.end() )
    {
        walked_.insert(bb);
        liveOutAtInstr(bb->end_->prev(), var);
    }
}

void CodeGenerator::liveInAtInstr(InstrNode instr, PseudoReg* var)
{
    // var ist live-in at instr
    instr->value_->liveIn_.insert( std::make_pair(var->regNr_, var) );

    // is instr the first statement of basic block?
    if ( typeid(*instr->value_) == typeid(LabelInstr) )
    {
        BBNode* bb = function_->cfg_.labelNode2BBNode_[instr];
        if (bb)
            bb->value_->liveIn_.insert( std::make_pair(var->regNr_, var) );

        // for each predecessor of bb
        CFG_RELATIVES_EACH(iter, bb->pred_)
            liveOutAtBlock(iter->value_, var);
    }
    else
    {
        // get preceding statement to instr
        InstrNode preInstr = instr->prev();
        if ( preInstr != function_->instrList_.sentinel() )
            liveOutAtInstr(preInstr, var);
    }
}

void CodeGenerator::liveOutAtInstr(InstrNode instr, PseudoReg* var)
{
    // var ist live-out at instr
    instr->value_->liveOut_.insert( std::make_pair(var->regNr_, var) );

    // for each reg v, that instr defines
    if ( typeid(*instr->value_) == typeid(AssignInstr) )
    {
        AssignInstr* ai = (AssignInstr*) instr->value_;

        if ( ai->result_ != var )
        {
            // add (v, w) to interference graph
            liveInAtInstr(instr, var);
        }
    }
    // for each reg v, that instr defines
    else if ( typeid(*instr->value_) == typeid(PhiInstr) )
    {
        PhiInstr* phi = (PhiInstr*) instr->value_;

        if ( phi->result_ != var )
        {
            // add (v, w) to interference graph
            liveInAtInstr(instr, var);
        }
    }
}

/*
    coloring
*/

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
    for (InstrNode iter = bb->value_->begin_->next(); iter != bb->value_->end_; iter = iter->next())
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
