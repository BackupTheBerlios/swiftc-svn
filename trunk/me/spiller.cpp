#include "me/spiller.h"

#include <algorithm>
#include <limits>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

namespace me {

//------------------------------------------------------------------------------
//-helpers----------------------------------------------------------------------
//------------------------------------------------------------------------------

#define DISTANCEBAG_EACH(iter, db) \
    for (DistanceBag::iterator (iter) = (db).begin(); (iter) != (db).end(); ++(iter))

inline int infinity() 
{
    return std::numeric_limits<int>::max();
}

inline void subOne(int& i)
{
    if (i != infinity())
        --i;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Spiller::Spiller(Function* function, size_t numRegs, int typeMask)
    : CodePass(function)
    , numRegs_(numRegs)
    , typeMask_(typeMask)
    , spillCounter_(-1) // first allowed name
{}

Spiller::~Spiller()
{
    RDUMAP_EACH(iter, spills_)
        delete iter->second;

    RDUMAP_EACH(iter, reloads_)
        delete iter->second;
}

/*
 * methods
 */

void Spiller::process()
{
    spill(cfg_->entry_);
    combine(cfg_->entry_);

    for (size_t i = 0; i < laterReloads_.size(); ++i)
    {
        Reg* reg             = laterReloads_[i].reg_;
        InstrNode* instrNode = laterReloads_[i].instrNode_;
        BBNode* bbNode       = laterReloads_[i].bbNode_;

        insertSpillIfNecessarry(reg, bbNode);
        insertReload(bbNode, reg, instrNode);
    }
    
    /*
     * erase phiSpills_ in reloads_' uses only in the debug version
     * in order to prevent the "no arg found in defs" assertion in reconstructSSAForm
     */
#ifdef SWIFT_DEBUG

    for (size_t i = 0; i < phiSpills_.size(); ++i)
    {
        InstrNode* phiNode = phiSpills_[i].instrNode_;
        InstrBase* phi = phiNode->value_;

        for (size_t j = 0; j < phi->arg_.size(); ++j)
        {
            Reg* phiArg = (Reg*) phi->arg_[j].op_;

            // check whether a reload uses this as use
            RDUMAP_EACH(iter, reloads_)
            {
                RegDefUse* rdu = iter->second;

                DEFUSELIST_EACH(iter, rdu->uses_)
                {
                    DefUse& use = iter->value_;
                    if ( use.reg_ == phiArg && use.instrNode_ == phiNode ) 
                    {
                        DefUseList::Node* eraseIter = iter;
                        iter = iter->prev();
                        rdu->uses_.erase(eraseIter);
                    }
                }
            }
        }
    }

#endif // SWIFT_DEBUG

    // now substitute phi spills args
    for (size_t i = 0; i < substitutes_.size(); ++i)
    {
        PhiInstr* phi = substitutes_[i].phi_;
        size_t arg = substitutes_[i].arg_;
        phi->arg_[arg].op_ = spillMap_[ (Reg*) phi->arg_[arg].op_ ];
    }

    // register phiSpill uses
    for (size_t i = 0; i < phiSpills_.size(); ++i)
    {
        Reg* orignal = phiSpills_[i].reg_;
        InstrNode* phiNode = phiSpills_[i].instrNode_;
        InstrBase* phi = phiNode->value_;
        BBNode* bbNode = phiSpills_[i].bbNode_;

        for (size_t j = 0; j < phi->arg_.size(); ++j)
        {
            Reg* phiArg = (Reg*) phi->arg_[j].op_;
            spills_[orignal]->uses_.append( DefUse(phiArg, phiNode, bbNode) );
        }
    }

    // register all spills as uses for the reloads_ set
    RDUMAP_EACH(iter, spills_)
    {
        Reg* reg = iter->first;
        swiftAssert( reg->typeCheck(typeMask_), "wrong reg type" );
        RegDefUse* rdu = iter->second;

        DEFUSELIST_EACH(defIter, rdu->defs_)
        {
            DefUse& def = defIter->value_;
            RDUMap::iterator rduIter = reloads_.find(reg);

            if ( rduIter != reloads_.end() )
                rduIter->second->uses_.append( DefUse(reg, def.instrNode_, def.bbNode_) );
        }
    }

    /*
     * rewire and reconstruct SSA form properly
     */

    RDUMAP_EACH(iter, spills_)
    {
        RegDefUse* rdu = iter->second;
        cfg_->reconstructSSAForm(rdu);
    }

    RDUMAP_EACH(iter, reloads_)
    {
        RegDefUse* rdu = iter->second;
        cfg_->reconstructSSAForm(rdu);
    }
}

/*
 * insertion of spills and reloads
 */

Reg* Spiller::insertSpill(BBNode* bbNode, Reg* reg, InstrNode* appendTo)
{
    swiftAssert( reg->typeCheck(typeMask_), "wrong reg type" );
    BasicBlock* bb = bbNode->value_;

    // create a new memory location
    Reg* mem = function_->newMem(reg->type_, spillCounter_--);
    SpillMap::iterator iter = spillMap_.find(reg);

    Spill* spill = new Spill(mem, reg);
    InstrNode* spillNode = cfg_->instrList_.insert(appendTo, spill);
    bb->fixPointers();

    // insert into spill map if reg is the first spill
    if (iter == spillMap_.end())
    {
        // yes -> so create new entry
        spillMap_.insert( std::make_pair(reg, mem) );
        swiftAssert( spills_.find(reg) == spills_.end(), "must be found here" );

        RegDefUse* rdu = new RegDefUse();
        rdu->defs_.append( DefUse(mem, spillNode, bbNode) ); // newly created definition
        spills_[reg] = rdu;
    }
    else
    {
        // nope -> so use the one already there
        swiftAssert( spills_.find(reg) != spills_.end(), "must be found here" );
        RegDefUse* rdu = spills_.find(reg)->second;
        rdu->defs_.append( DefUse(mem, spillNode, bbNode) ); // newly created definition
    }

    return mem;
}

void Spiller::insertReload(BBNode* bbNode, Reg* reg, InstrNode* appendTo)
{
    swiftAssert( reg->typeCheck(typeMask_), "wrong reg type" );
    swiftAssert( spillMap_.find(reg) != spillMap_.end(), "must be in the spillMap_" )

    Reg* mem = spillMap_[reg];
    swiftAssert( mem->isMem(), "must be a memory reg" );

    // create new result
#ifdef SWIFT_DEBUG
    Reg* newReg = function_->newSSA(reg->type_, &reg->id_);
#else // SWIFT_DEBUG
    Reg* newReg = function_->newSSA(reg->type_);
#endif // SWIFT_DEBUG

    Reload* reload = new Reload(newReg, mem);
    InstrNode* reloadNode = cfg_->instrList_.insert(appendTo, reload);
    bbNode->value_->fixPointers();

    // keep account of new use
    spills_.find(reg)->second->uses_.append( DefUse(mem, reloadNode, bbNode) );

    /*
     * collect def-use infos
     */
    RDUMap::iterator iter = reloads_.find(reg);

    // is this the first reload of reg?
    if ( iter == reloads_.end() )
    {
        // yes -> so create new entry
        RegDefUse* rdu = new RegDefUse();
        rdu->defs_.append( DefUse(newReg, reloadNode, bbNode) ); // newly created definition
        rdu->defs_.append( DefUse(reg, reg->def_.instrNode_, reg->def_.bbNode_) ); // orignal definition
        rdu->uses_ = reg->uses_;
        reloads_[reg] = rdu;
    }
    else
    {
        // nope -> so use the one already there
        RegDefUse* rdu = iter->second;
        rdu->defs_.append( DefUse(newReg, reloadNode, bbNode) ); // newly created definition
    }
}

/*
 * distance calculation via Belady
 */

int Spiller::distance(BBNode* bbNode, Reg* reg, InstrNode* instrNode, BBSet walked)
{
    swiftAssert( reg->typeCheck(typeMask_), "wrong reg type" );
    InstrBase* instr = instrNode->value_;

    if (instr->isRegUsed(reg))
        return 0;
    // else

    return distanceRec(bbNode, reg, instrNode, walked);
}

int Spiller::distanceRec(BBNode* bbNode, Reg* reg, InstrNode* instrNode, BBSet walked)
{
    swiftAssert( reg->typeCheck(typeMask_), "wrong reg type" );
    InstrBase* instr = instrNode->value_;
    BasicBlock* bb = bbNode->value_;

    // is reg live at instr?
    if ( !instr->liveIn_.contains(reg) ) 
        return infinity(); // no -> return "infinity"
    // else

    int result; // result goes here

    // is the current instruction a JumpInstr?
    if ( dynamic_cast<JumpInstr*>(instr) ) 
    {
        JumpInstr* ji = (JumpInstr*) instr;

        // collect and compute results of targets
        int results[ji->numTargets_];
        for (size_t i = 0; i < ji->numTargets_; ++i)
        {
            BBNode* target = ji->bbTargets_[i];

            if ( walked.contains(target) )
            {
                // already walked so put in infinity -> another target will yield a value < inf
                results[i] = infinity();
                continue;
            }
            else
            {
                walked.insert(target);
                results[i] = distance( target, reg, ji->instrTargets_[i], walked );
            }
        }

        result = *std::min_element(results, results + ji->numTargets_);
    }
    // is the current instruction the last ordinary instruction in this bb?
    else if (bb->end_->prev() == instrNode)
    {
        // -> visit the next bb
        swiftAssert(instrNode->next() != cfg_->instrList_.sentinel(), 
            "this may not be the last instruction");
        swiftAssert(bbNode->numSuccs() == 1, "should have exactly one succ");

        BBNode* succ = bbNode->succ_.first()->value_;

        if ( walked.contains(succ) )
        {
            // already walked so put in infinity -> another target will yield a value < inf
            result = infinity();
        }
        else
        {
            walked.insert(succ);
            result = distance( succ, reg, instrNode->next(), walked );
        }

    }
    else 
    {
        // -> so we must have a "normal" successor instruction
        result = distance( bbNode, reg, instrNode->next(), walked );
    }

    // do not count a LabelInstr
    int inc = (typeid(*instr) == typeid(LabelInstr)) ? 0 : 1;

    // add up the distance and do not calculate around
    result = (result == infinity()) ? infinity() : result + inc;

    return result;
}

/*
 * local spilling
 */

Reg* Spiller::regFind(Spiller::DistanceBag& ds, Reg* reg)
{
    swiftAssert( reg->typeCheck(typeMask_), "wrong reg type" );
    DISTANCEBAG_EACH(iter, ds)
    {
        if ( iter->reg_ == reg)
            return reg;
    }

    return 0;
}

void Spiller::discardFarest(DistanceBag& ds)
{
    while (ds.size() > numRegs_)
        ds.erase( ds.begin() );
}

void Spiller::spill(BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;

    /*
     * passed should contain all regs live in at bb 
     * and the results of phi operations in bb
     */
    RegSet passedAll = bb->liveIn_;

    // erase those regs in passed which should not be considered during this pass
    RegSet passed;
    REGSET_EACH(iter, passedAll)
    {
        Reg* reg = *iter;

        if ( reg->typeCheck(typeMask_) )
            passed.insert(reg);
    }

    // for each PhiInstr in bb
    for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
            "must be a PhiInstr here" );

        PhiInstr* phi = (PhiInstr*) iter->value_;

        if ( !phi->result()->typeCheck(typeMask_) )
            continue;
        
        // add result to the passed set
        passed.insert( phi->result() );
    }
    // passed now holds all proposed values

    /*
     * now we need the set which is allowed to be in real registers
     */
    RegSet inRegs;

    swiftAssert( in_.find(bbNode) == in_.end(), "already inserted" );
    RegSet& inB = in_.insert( std::make_pair(bbNode, RegSet()) ).first->second;
    DistanceBag currentlyInRegs;

    BBSet walked;
    walked.insert(bbNode);

    // put in all regs from passed and calculate the distance to its next use
    REGSET_EACH(iter, passed)
    {
        // because inRegs doesn't contain regs of phi's arg, we can start with firstOrdinary_
        currentlyInRegs.insert( RegAndDistance(*iter, 
                    distance(bbNode, *iter, bb->firstOrdinary_, walked)) );
    }

    /*
     * use the first numRegs_ registers if the set is too large
     */
    discardFarest(currentlyInRegs);

    // copy over to inRegs
    DISTANCEBAG_EACH(regIter, currentlyInRegs)
        inRegs.insert(regIter->reg_);

    /*
     * currentlyInRegs holds all regs which are at the current point in regs. 
     * Initially it is assumed that all regs in currentlyInRegs can be kept in regs.
     */

    // traverse all ordinary instructions in bb
    for (InstrNode* iter = bb->firstOrdinary_; iter != bb->end_; iter = iter->next())
    {
        // points to the current instruction
        InstrBase* instr = iter->value_;

        /*
         * points to the last instruction 
         * -> first reloads then spills are appeneded
         * -> so we have:
         *  lastInstrNode
         *  spills
         *  reloads
         *  instr
         */
        InstrNode* lastInstrNode = iter->prev_;

        /*
         * check whether all regs on the arg are in regs 
         * and count necessary reloads
         */
        int numReloads = 0;
        for (size_t i = 0; i < instr->arg_.size(); ++i)
        {
            if (typeid(*instr->arg_[i].op_) != typeid(Reg))
                continue;

            Reg* reg = (Reg*) instr->arg_[i].op_;

            if ( !reg->typeCheck(typeMask_) )
                continue;

            /*
             * is this reg currently not in a real register?
             */

            if ( regFind(currentlyInRegs, reg) == 0 )
            {
                /*
                 * insert reload instruction
                 */

                // check whether reg hast been discaded but is actually in passed
                if ( !inB.contains(reg) && passed.contains(reg) )
                {
                    // mark for later reload insertion
                    laterReloads_.push_back( DefUse(reg, lastInstrNode, bbNode) );
                }
                else
                    insertReload(bbNode, reg, lastInstrNode);

                currentlyInRegs.insert( RegAndDistance(reg, 0) );

                // keep account of the number of needed reloads here
                ++numReloads;
            }
            else
            {
                /*
                 * manage inB and inRegs
                 */
                RegSet::iterator regIter = inRegs.find(reg);
                if ( regIter != inRegs.end() )
                {
                    // reg is used befor discarded
                    inB.insert(*regIter);
                    inRegs.erase(regIter);
                }
            }
        }

        // count how many of instr->res_ have a proper type
        int numLhs = 0;
        for (size_t i = 0;  i < instr->res_.size(); ++i)
        {
            Reg* reg = instr->res_[i].reg_;

            if ( reg->typeCheck(typeMask_) )
                ++numLhs;
        }

        // numRemove -> number of regs which must be removed from currentlyInRegs
        int numRemove = std::max( numLhs + numReloads + int(currentlyInRegs.size()) - int(numRegs_), 0 );

        /*
         * insert spills
         */
        for (int i = 0; i < numRemove; ++i)
        {
            Reg* toBeSpilled = currentlyInRegs.begin()->reg_;

            // manage inB and inRegs
            RegSet::iterator regIter = inRegs.find(toBeSpilled);
            if ( regIter != inRegs.end() )
                inRegs.erase(regIter); // reg is not used befor -> don't spill
            else
            {
                // insert spill instruction
                insertSpill(bbNode, toBeSpilled, lastInstrNode);
            }

            // remove first reg
            currentlyInRegs.erase( currentlyInRegs.begin() );
        }

        /*
         * update distances
         */

        DistanceBag newBag;
        DISTANCEBAG_EACH(regIter, currentlyInRegs)
        {
            int dist = regIter->distance_;
            subOne(dist);

            // recalculate distance if we have reached the next use
            if (dist < 0)
                dist = distance(bbNode, regIter->reg_, iter, walked);

            newBag.insert( RegAndDistance(regIter->reg_, dist) );
        }

        currentlyInRegs = newBag;

        // add results to currentlyInRegs
        for (size_t i = 0; i < instr->res_.size(); ++i)
        {
            Reg* reg = instr->res_[i].reg_;

            if ( reg->typeCheck(typeMask_) )
            {
                currentlyInRegs.insert( 
                        RegAndDistance(reg, distance(bbNode, reg, iter, walked)) );
            }
        }
    } // for each instr

    // put in remaining regs from inRegs to inB
    inB.insert( inRegs.begin(), inRegs.end() );
    
    swiftAssert( out_.find(bbNode) == out_.end(), "already inserted" );
    RegSet& outB = out_.insert( std::make_pair(bbNode, RegSet()) ).first->second;

    DISTANCEBAG_EACH(regIter, currentlyInRegs)
        outB.insert(regIter->reg_);
    /*
     * use this for debugging for inB and outB
     */

#if 0
    std::cout << bb->name() << std::endl;

    std::cout << "\tinB:" << std::endl;
    REGSET_EACH(iter, inB)
        std::cout << "\t\t" << (*iter)->toString() << std::endl;

    std::cout << "\toutB:" << std::endl;
    REGSET_EACH(iter, outB)
        std::cout << "\t\t" << (*iter)->toString() << std::endl;
#endif

    // for each child of bb in the dominator tree
    BBLIST_EACH(bbIter, bb->domChildren_)
    {
        BBNode* domChild = bbIter->value_;
        spill(domChild);
    }
}

/*
 * global combining
 */

void Spiller::combine(BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;
    RegSet& inB = in_[bbNode];

    /*
     * handle phi functions
     */

    RegSet phiArgs;
    
    // for each phi function
    for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
                "must be a phi instruction here");
        PhiInstr* phi = (PhiInstr*) iter->value_;
        Reg* phiRes = phi->result();
        
        if ( !phiRes->typeCheck(typeMask_) )
            continue;
        
        bool phiSpill;
        RegSet::iterator inBIter = inB.find(phiRes);

        if ( inBIter == inB.end() )
            phiSpill = true;
        else
        {
            phiSpill = false;
            inB.erase(inBIter);
        }

        // for each arg
        CFG::Relative* prePhiRelative = bbNode->pred_.first();
        for (size_t i = 0; i < phi->arg_.size(); ++i)
        {
            BBNode* prePhiNode = prePhiRelative->value_;
            BasicBlock* prePhi = prePhiNode->value_;
            RegSet& preOut = out_[prePhiNode];

            // get phi argument
            swiftAssert( typeid(*phi->arg_[i].op_) == typeid(Reg),
                    "must be a Reg here" );
            Reg* phiArg = (Reg*) phi->arg_[i].op_;
            swiftAssert( !phiArg->isMem(), "must not be a memory location" );

            if ( phiSpill && preOut.contains(phiArg) )
            {
                // -> we have a phi spill and phiArg in preOut
                preOut.erase(phiArg);

                // insert spill to predecessor basic block and replace phi arg
                insertSpill( prePhiNode, phiArg, prePhi->getBackSpillLocation() );
                substitutes_.push_back( Substitute(phi, i) );
            }
            else if ( phiSpill && !preOut.contains(phiArg) )
            {
                // -> we have a phi spill and phiArg is not in preOut
                substitutes_.push_back( Substitute(phi, i) );
            }
            else if( !phiSpill && !preOut.contains(phiArg) )
            {   
                // -> we have no phi spill and phiArg not in preOut
                // insert reload to predecessor basic block
                insertReload( prePhiNode, phiArg, prePhi->getBackReloadLocation() );
            }

            // traverse to next predecessor
            prePhiRelative = prePhiRelative->next();
        }
        swiftAssert(prePhiRelative == bbNode->pred_.sentinel(),
                "all nodes must be traversed")

        if (phiSpill)
        {
            // convert phi result to a memory location
            phiRes->color_ = Reg::MEMORY_LOCATION;

            // add to spills_
            RegDefUse* rdu = new RegDefUse();
            rdu->defs_.append( DefUse(phiRes, iter, bbNode) );
            swiftAssert( phiRes->typeCheck(typeMask_), "wrong reg type" );
            spills_[phiRes] = rdu;
            spillMap_[phiRes] = phiRes; // phi-spills map to them selves

            for (size_t i = 0; i < phi->arg_.size(); ++i)
            {
                Reg* reg = (Reg*) phi->arg_[i].op_;
                phiSpills_.push_back( DefUse(reg, iter, bbNode) );
            }
        }
    }

    // for each predecessor of the current node
    CFG_RELATIVES_EACH(predIter, bbNode->pred_)
    {
        BBNode* predNode = predIter->value_;
        BasicBlock* pred = predNode->value_;
        RegSet& outP = out_[predNode];

        /*
         * find out where to append reloads
         */

        InstrNode* appendTo;
        BBNode* bbAppend;

        if ( bbNode->pred_.size() == 1 )
        {
            // place reloads in this block on the top
            bbAppend = bbNode;
            appendTo = bb->getReloadLocation();
        }
        else
        {
            swiftAssert(predNode->succ_.size() == 1, "must exactly have one successor");
            // append to last non phi instruction belonging to pred
            bbAppend = predNode;
            appendTo = pred->getBackReloadLocation();
        }

        /*
         * handle reloads
         */

        // build vector which holds all necessary reloads
        RegVec reloads( inB.size() );
        RegVec::iterator end = std::set_difference( 
                inB.begin(), inB.end(), outP.begin(), outP.end(), reloads.begin() );
        reloads.erase( end, reloads.end() );// truncate properly

        // for each reload
        for (size_t i = 0;  i < reloads.size(); ++i)
        {
            // create and insert reload
            Reg* reg = reloads[i];
            swiftAssert( reg->typeCheck(typeMask_), "wrong reg type" );

            // ensure that every reload is dominated by a spill
            insertSpillIfNecessarry(reg, predNode);
            insertReload( bbAppend, reg, bbAppend->value_->getBackReloadLocation() );
        }
    } // for each predecessor

    // for each child of bb in the dominator tree
    BBLIST_EACH(bbIter, bb->domChildren_)
    {
        BBNode* domChild = bbIter->value_;
        combine(domChild);
    }
}

void Spiller::insertSpillIfNecessarry(Reg* reg, BBNode* bbNode)
{
    /*
     * go up dominance tree until we found the first dominating block 
     * which has reg not in 'in_'
     */
    while ( in_[bbNode].contains(reg) )
        bbNode = cfg_->idoms_[bbNode->postOrderIndex_];

    BasicBlock* bb = bbNode->value_;

    /*
     * check whether reg is already spilled in this block
     */

    // is it phi-spilled?
    for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
                "must be a PhiInstr" );
        PhiInstr* phi = (PhiInstr*) iter->value_;
        Reg* res = phi->result();

        if (res == reg)
            return; // everything's fine here -> no spill needed
    }

    // is it spilled by an ordinary spill?
    for (InstrNode* iter = bb->firstOrdinary_; iter != bb->end_; iter = iter->next())
    {
        if ( typeid(*iter->value_) != typeid(Spill) )
            continue;

        Spill* spill = (Spill*) iter->value_;

        if ( ((Reg*) spill->arg_[0].op_) == reg )
            return; // everything is fine -> there is already a spill
    }

    // -> no spill in this basic block, so insert one

    // find spill location
    InstrNode* appendTo;

    if (reg->def_.bbNode_ == bbNode)
        appendTo = reg->def_.instrNode_;
    else
        appendTo = bb->getSpillLocation();
    
    // insert spill
    insertSpill(bbNode, reg, appendTo);
}

} // namespace me
