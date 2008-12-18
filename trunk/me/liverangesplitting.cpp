#include "me/liverangesplitting.h"

#include <typeinfo>

namespace me {

/*
 * constructor and destructor
 */

LiveRangeSplitting::LiveRangeSplitting(Function* function)
    : CodePass(function)
{}

LiveRangeSplitting::~LiveRangeSplitting()
{
    RDUMAP_EACH(iter, phis_)
        delete iter->second;
}

/*
 * further methods
 */

void LiveRangeSplitting::process()
{
    BBNode* currentBB;

    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        InstrBase* instr = iter->value_;

        if ( typeid(*instr) == typeid(LabelInstr) )
        {
            currentBB = cfg_->labelNode2BBNode_[iter];
            continue;
        }

        if ( instr->isConstrained() )
            liveRangeSplit(iter, currentBB);
    }

    // HACK: this should be superfluous when there is a good graph implementation
    cfg_->entry_->postOrderIndex_ = 0;     

    // new basic blocks habe been inserted so recompute dominance stuff
    cfg_->calcPostOrder(cfg_->entry_);
    cfg_->calcDomTree();
    cfg_->calcDomFrontier();

    // reconstruct SSA form for the newly inserted phi instructions
    RDUMAP_EACH(iter, phis_)
        cfg_->reconstructSSAForm(iter->second);
}

void LiveRangeSplitting::liveRangeSplit(InstrNode* instrNode, BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;

    // create new basic block
    cfg_->splitBB(instrNode, bbNode);

    /* 
     * insert new phi instruction for each live-in var
     */

    REGSET_EACH(iter, instrNode->value_->liveIn_)
    {
        Reg* reg = *iter;

        // create new result
#ifdef SWIFT_DEBUG
        Reg* newReg = function_->newSSA(reg->type_, &reg->id_);
#else // SWIFT_DEBUG
        Reg* newReg = function_->newSSA(reg->type_);
#endif // SWIFT_DEBUG

        // create phi instruction
        swiftAssert( bbNode->pred_.size() == 1, "must have exactly one predecessor" );
        PhiInstr* phi = new PhiInstr(newReg, 1);

        // init sourceBB and arg
        phi->sourceBBs_[0] = bbNode->pred_.first()->value_;
        phi->arg_[0].op_ = reg;

        // insert instruction and fix pointer
        InstrNode* phiNode = cfg_->instrList_.insert(bb->begin_, phi); 

        /*
         * keep track of def-use stuff
         */

        // do we already have def-use information for this reg?
        RDUMap::iterator iter = phis_.find(reg);
        if ( iter == phis_.end() )
        {
            RegDefUse* rdu = new RegDefUse(); 
            rdu->defs_.append( DefUse(newReg, phiNode, bbNode) ); // newly created definition
            rdu->defs_.append( DefUse(reg, reg->def_.instrNode_, reg->def_.bbNode_) ); // orignal definition
            rdu->uses_ = reg->uses_; // orignal uses
            phis_[reg] = rdu;
        }
        else
        {
            RegDefUse* rdu = iter->second;
            rdu->defs_.append( DefUse(newReg, phiNode, bbNode) ); // newly created definition
        }
    }

    bb->fixPointers();
}

} // namespace me
