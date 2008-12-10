#include "me/spiller.h"

#include <algorithm>
#include <limits>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

/*
 * TODO
 *
 * Here is room for optimization:
 *
 *           ----------------
 *           |              |
 *           ----------------
 *                  /\
 *                 /  \
 *                /    \
 *               /      \
 *              /        \
 *           spill a   spill a
 *
 * This can be pushed to the parent basic block.
 */

namespace me {

//------------------------------------------------------------------------------
//-helpers----------------------------------------------------------------------
//------------------------------------------------------------------------------

#define RDUMAP_EACH(iter, rdus) \
    for (RDUMap::iterator (iter) = (rdus).begin(); (iter) != (rdus).end(); ++(iter))

#define DEFLIST_EACH(iter, defList) \
    for (DefList::Node* (iter) = (defList).first(); (iter) != (defList).sentinel(); (iter) = (iter)->next())

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

    // now the remaining phi spilled reloads can be inserted;
    for (size_t i = 0; i < phiSpilledReloads_.size(); ++i)
    {
        BBNode* bbNode = phiSpilledReloads_[i].bbNode_;
        Reg* reg = phiSpilledReloads_[i].reg_;
        swiftAssert( reg->spillReg(typeMask_), "wrong reg type" );
        insertReload( bbNode, reg, bbNode->value_->getBackReloadLocation() );
    }

    // now substitute phi spills args
    for (size_t i = 0; i < substitutes_.size(); ++i)
    {
        PhiInstr* phi = substitutes_[i].phi_;
        size_t arg = substitutes_[i].arg_;
        phi->rhs_[arg].op_ = spillMap_[ (Reg*) phi->rhs_[arg].op_ ];
    }

    // register all spills as uses for the reloads_ set
    RDUMAP_EACH(iter, spills_)
    {
        Reg* reg = iter->first;
        swiftAssert( reg->spillReg(typeMask_), "wrong reg type" );
        RegDefUse* rdu = iter->second;

        DEFLIST_EACH(defIter, rdu->defs_)
        {
            Def& def = defIter->value_;
            RDUMap::iterator rduIter = reloads_.find(reg);

            if ( rduIter != reloads_.end() )
                rduIter->second->uses_.append( DefUse(def.instrNode_, def.bbNode_) );
        }
    }

    /*
     * rewire and reconstruct SSA form properly
     */

    RDUMAP_EACH(iter, spills_)
    {
        RegDefUse* rdu = iter->second;
        reconstructSSAForm(rdu);
    }

    RDUMAP_EACH(iter, reloads_)
    {
        RegDefUse* rdu = iter->second;
        reconstructSSAForm(rdu);
    }
}

/*
 * insertion of spills and reloads
 */

Reg* Spiller::insertSpill(BBNode* bbNode, Reg* reg, InstrNode* appendTo)
{
    swiftAssert( reg->spillReg(typeMask_), "wrong reg type" );
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
        rdu->defs_.append( Def(mem, spillNode, bbNode) ); // newly created definition
        spills_[reg] = rdu;
    }
    else
    {
        // nope -> so use the one already there
        swiftAssert( spills_.find(reg) != spills_.end(), "must be found here" );
        RegDefUse* rdu = spills_.find(reg)->second;
        rdu->defs_.append( Def(mem, spillNode, bbNode) ); // newly created definition
    }

    return mem;
}

void Spiller::insertReload(BBNode* bbNode, Reg* reg, InstrNode* appendTo)
{
    swiftAssert( reg->spillReg(typeMask_), "wrong reg type" );
    if ( spillMap_.find(reg) == spillMap_.end() )
    {
        // remember for later insertion
        phiSpilledReloads_.push_back( PhiSpilledReload(bbNode, reg) );
        return;
    }

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
    spills_.find(reg)->second->uses_.append( DefUse(reloadNode, bbNode) );

    /*
     * collect def-use infos
     */
    RDUMap::iterator iter = reloads_.find(reg);

    // is this the first reload of reg?
    if ( iter == reloads_.end() )
    {
        // yes -> so create new entry
        RegDefUse* rdu = new RegDefUse();
        rdu->defs_.append( Def(mem, reloadNode, bbNode) ); // newly created definition
        rdu->defs_.append( Def(reg, reg->def_.instrNode_, reg->def_.bbNode_) ); // orignal definition
        rdu->uses_ = UseList(reg->uses_);
        reloads_[reg] = rdu;
    }
    else
    {
        // nope -> so use the one already there
        RegDefUse* rdu = iter->second;
        rdu->defs_.append( Def(mem, reloadNode, bbNode) ); // newly created definition
    }
}

/*
 * distance calculation via Belady
 */

int Spiller::distance(BBNode* bbNode, Reg* reg, InstrNode* instrNode, BBSet walked)
{
    swiftAssert( reg->spillReg(typeMask_), "wrong reg type" );
    InstrBase* instr = instrNode->value_;

    if (instr->isRegUsed(reg))
        return 0;
    // else

    return distanceRec(bbNode, reg, instrNode, walked);
}

int Spiller::distanceRec(BBNode* bbNode, Reg* reg, InstrNode* instrNode, BBSet walked)
{
    swiftAssert( reg->spillReg(typeMask_), "wrong reg type" );
    InstrBase* instr = instrNode->value_;
    BasicBlock* bb = bbNode->value_;

    // is reg live at instr?
    if ( instr->liveIn_.find(reg) == instr->liveIn_.end() ) 
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

            if ( walked.find(target) != walked.end() )
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

        if ( walked.find(succ) != walked.end() )
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
    swiftAssert( reg->spillReg(typeMask_), "wrong reg type" );
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

        if ( reg->spillReg(typeMask_) )
            passed.insert(reg);
    }

    // for each PhiInstr in bb
    for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
            "must be a PhiInstr here" );

        PhiInstr* phi = (PhiInstr*) iter->value_;

        if ( !phi->result()->spillReg(typeMask_) )
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
        // because inRegs doesn't contain regs of phi's rhs, we can start with firstOrdinary_
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
         * check whether all regs on the rhs are in regs 
         * and count necessary reloads
         */
        int numReloads = 0;
        for (size_t i = 0; i < instr->rhs_.size(); ++i)
        {
            if (typeid(*instr->rhs_[i].op_) != typeid(Reg))
                continue;

            Reg* reg = (Reg*) instr->rhs_[i].op_;

            if ( !reg->spillReg(typeMask_) )
                continue;

            /*
             * is this reg currently not in a real register?
             */

            if ( regFind(currentlyInRegs, reg) == 0 )
            {
                /*
                 * insert reload instruction
                 */
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

        // count how many of instr->lhs_ have a proper type
        int numLhs = 0;
        for (size_t i = 0;  i < instr->lhs_.size(); ++i)
        {
            Reg* reg = instr->lhs_[i].reg_;

            if ( reg->spillReg(typeMask_) )
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
        for (size_t i = 0; i < instr->lhs_.size(); ++i)
        {
            Reg* reg = instr->lhs_[i].reg_;

            if ( reg->spillReg(typeMask_) )
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
        
        if ( !phiRes->spillReg(typeMask_) )
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
        for (size_t i = 0; i < phi->rhs_.size(); ++i)
        {
            BBNode* prePhiNode = prePhiRelative->value_;
            BasicBlock* prePhi = prePhiNode->value_;
            RegSet& preOut = out_[prePhiNode];

            // get phi argument
            swiftAssert( typeid(*phi->rhs_[i].op_) == typeid(Reg),
                    "must be a Reg here" );
            Reg* phiArg = (Reg*) phi->rhs_[i].op_;
            swiftAssert( !phiArg->isMem(), "must not be a memory location" );

            if ( phiSpill && preOut.find(phiArg) != preOut.end() )
            {
                // -> we have a phi spill and phiArg in preOut
                preOut.erase(phiArg);

                // insert spill to predecessor basic block and replace phi arg
                phi->rhs_[i].op_ = insertSpill( prePhiNode, phiArg, prePhi->getBackSpillLocation() );
            }
            else if ( phiSpill && preOut.find(phiArg) == preOut.end() )
            {
                /*
                 * mark for substitution which must be done 
                 * after global spill insertion
                 */
                substitutes_.push_back( Substitute(phi, i) );
            }
            else if( !phiSpill && preOut.find(phiArg) == preOut.end() )
            {   
                // -> we have no phi spill and phiArg not in preOut
                // insert reload to predecessor basic block
                insertReload( prePhiNode, phiArg, prePhi->getBackReloadLocation() ); // insert reload
            }
            else
            {
                // -> we have no phi spill and phiArg is in preOut
                // remove it from preOut since no spill is needed there
                preOut.erase(phiArg);
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
            rdu->defs_.append( Def(phiRes, iter, bbNode) );
            rdu->uses_.append( DefUse(iter, bbNode) );
            swiftAssert( phiRes->spillReg(typeMask_), "wrong reg type" );
            spills_[phiRes] = rdu;
        }
    }

    // for each predecessor of the current node
    CFG_RELATIVES_EACH(predIter, bbNode->pred_)
    {
        BBNode* predNode = predIter->value_;
        BasicBlock* pred = predNode->value_;
        RegSet& outP = out_[predNode];

        /*
         * find out where to append spills/reloads
         */

        InstrNode* appendTo;
        BBNode* bbAppend;
        bool here;

        if ( bbNode->pred_.size() == 1 )
        {
            // place reloads in this block on the top
            bbAppend = bbNode;
            appendTo = bb->getSpillLocation();
            here = true;
        }
        else
        {
            swiftAssert(predNode->succ_.size() == 1, "must exactly have one successor");
            // append to last non phi instruction belonging to pred
            bbAppend = predNode;
            appendTo = pred->getBackSpillLocation();
            here = false;
        }


        /*
         * handle spills
         */

        //// build vector which holds all necessary spills
        //RegVec spills( outP.size() );
        //RegVec::iterator end = std::set_difference( 
                //outP.begin(), outP.end(), inB.begin(), inB.end(), spills.begin() );
        //spills.erase( end, spills.end() ); // truncate properly

        //if (spills.size() != 0)
        //{
            //// for each spill
            //for (size_t i = 0;  i < spills.size(); ++i)
            //{
                //Reg* reg = spills[i];
                //swiftAssert( reg->spillReg(typeMask_), "wrong reg type" );
                //insertSpill(bbAppend, reg, appendTo);
                //std::cout << appendTo->next()->value_->toString() << std::endl;
            //} // for each spill
        //}

        if (here)
            appendTo = bb->getReloadLocation();
        else
            appendTo = pred->getBackReloadLocation();

        /*
         * handle reloads
         */

        // build vector which holds all necessary reloads
        RegVec reloads( inB.size() );
        RegVec::iterator end = std::set_difference( 
                inB.begin(), inB.end(), outP.begin(), outP.end(), reloads.begin() );
        reloads.erase( end, reloads.end() );// truncate properly

        if (reloads.size() != 0)
        {
            // for each reload
            for (size_t i = 0;  i < reloads.size(); ++i)
            {
                // create and insert reload
                Reg* reg = reloads[i];
                swiftAssert( reg->spillReg(typeMask_), "wrong reg type" );
                insertReload(bbAppend, reg, appendTo);
            }
        }
    } // for each predecessor

    // for each child of bb in the dominator tree
    BBLIST_EACH(bbIter, bb->domChildren_)
    {
        BBNode* domChild = bbIter->value_;
        combine(domChild);
    }
}

/*
 * SSA reconstruction and rewiring
 */

void Spiller::reconstructSSAForm(RegDefUse* rdu)
{
    BBSet defBBs;

    /*
     * calculate iterated dominance frontier for all defining basic blocks
     */
    DEFLIST_EACH(iter, rdu->defs_)
        defBBs.insert(iter->value_.bbNode_);

    BBSet iDF = cfg_->calcIteratedDomFrontier(defBBs);

    // for each use
    USELIST_EACH(iter, rdu->uses_)
    {
        DefUse& du = iter->value_;
        BBNode* bbNode = du.bbNode_;
        InstrNode* instrNode = du.instrNode_;
        InstrBase* instr = instrNode->value_;

        // for each arg for which instr->rhs_[i] in defs
        for (size_t i = 0; i < instr->rhs_.size(); ++i)
        {
            if ( typeid(*instr->rhs_[i].op_) != typeid(Reg) )
                continue;

            Reg* useReg = (Reg*) instr->rhs_[i].op_;

#ifdef SWIFT_DEBUG
            bool found = false;
#endif // SWIFT_DEBUG

            DEFLIST_EACH(iter, rdu->defs_)
            {
                Reg* defReg = iter->value_.reg_;
                swiftAssert( defReg->spillReg(typeMask_), "wrong reg type" );

                // substitute arg with proper definition
                if (useReg == defReg)
                {
#ifdef SWIFT_DEBUG
                    found = true;
#endif // SWIFT_DEBUG
                    instr->rhs_[i].op_ = findDef(i, instrNode, bbNode, rdu, iDF);
                }
            }

            swiftAssert(found, "no arg found in defs");
        }
    }
}

Reg* Spiller::findDef(size_t p, InstrNode* instrNode, BBNode* bbNode, RegDefUse* rdu, BBSet& iDF)
{
    if ( typeid(*instrNode->value_) == typeid(PhiInstr) )
    {
        PhiInstr* phi = (PhiInstr*) instrNode->value_;
        bbNode = phi->sourceBBs_[p];
        instrNode = bbNode->value_->end_->prev();
    }

    BasicBlock* bb = bbNode->value_;

    while (true)
    {
        // iterate backwards over all instructions without phi instructions
        while (instrNode != bb->begin_)
        {
            InstrBase* instr = instrNode->value_; 

            // defines instr one of rdu?
            for (size_t i = 0; i < instr->lhs_.size(); ++i)
            {
                if ( typeid(*instr->lhs_[i].reg_) != typeid(Reg) )
                    continue;

                Reg* instrReg = (Reg*) instr->lhs_[i].reg_;

                DEFLIST_EACH(iter, rdu->defs_)
                {
                    Reg* defReg = iter->value_.reg_;
                    swiftAssert( defReg->spillReg(typeMask_), "wrong reg type" );

                    if (instrReg == defReg)
                        return instrReg; // yes -> return the defined reg
                }
            }

            // move backwards
            instrNode = instrNode->prev();
        }

        // is this basic block in the iterated dominance frontier?
        BBSet::iterator bbIter = iDF.find(bbNode);
        if ( bbIter != iDF.end() )
        {
            // -> place phi function
            swiftAssert(bbNode->pred_.size() > 1, 
                    "current basic block must have more than 1 predecessors");
            Reg* reg = rdu->defs_.first()->value_.reg_;
            swiftAssert( reg->spillReg(typeMask_), "wrong reg type" );

            // create new result
#ifdef SWIFT_DEBUG
            Reg* newReg = function_->newSSA(reg->type_, &reg->id_);
#else // SWIFT_DEBUG
            Reg* newReg = function_->newSSA(reg->type_);
#endif // SWIFT_DEBUG
            newReg->color_ = Reg::MEMORY_LOCATION;

            // create phi instruction
            PhiInstr* phi = new PhiInstr( newReg, bbNode->pred_.size() );

            // init sourceBBs
            size_t counter = 0;
            CFG_RELATIVES_EACH(predIter, bbNode->pred_)
            {
                phi->sourceBBs_[counter] = predIter->value_;
                ++counter;
            }

            instrNode = cfg_->instrList_.insert(bb->begin_, phi); 
            bb->firstPhi_ = instrNode;

            // register new definition
            rdu->defs_.append( Def(newReg, instrNode, bbNode) );

            for (size_t i = 0; i < phi->rhs_.size(); ++i)
                phi->rhs_[i].op_ = findDef(i, instrNode, bbNode, rdu, iDF);

            return phi->result();
        }

        swiftAssert( cfg_->entry_ != bbNode, "unreachable code");

        // go up dominance tree -> update bbNode, bb and instrNode
        bbNode = cfg_->idoms_[bbNode->postOrderIndex_];
        bb = bbNode->value_;
        instrNode = bb->end_->prev();
    }

    return 0;
}

} // namespace me
