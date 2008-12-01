#include "me/spiller.h"

#include <algorithm>
#include <limits>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

namespace me {

typedef std::vector<Reg*> RegVec;

#define REGDEFSET_EACH(iter, defs) \
    for (RegDefSet::iterator (iter) = (defs).begin(); (iter) != (defs).end(); ++(iter))

//------------------------------------------------------------------------------

/*
 * distance stuff
 */

// TODO
#define NUM_REGS 8

/// Needed for sorting via the distance method.
struct RegAndDistance 
{
    Reg* reg_;
    int distance_;

    /*
     * constructors
     */

    RegAndDistance() {}

    RegAndDistance(Reg* reg, int distance)
        : reg_(reg)
        , distance_(distance)
    {}

    /// copy constructor
    RegAndDistance(const RegAndDistance& rd)
        : reg_(rd.reg_)
        , distance_(rd.distance_)
    {}

    /*
     * operator
     */

    /// needed by std::sort
    bool operator < (const RegAndDistance& r) const
    {
        return distance_ > r.distance_; // sort farest first
    }
};

// TODO perhaps use two synced sets for better performance:
// - one sorted by distance
// - one sorted by reg

typedef std::multiset<RegAndDistance> DistanceBag;
/// Use this macro in order to easily visit all elements of a RegSet
#define DISTANCEBAG_EACH(iter, db) \
    for (DistanceBag::iterator (iter) = (db).begin(); (iter) != (db).end(); ++(iter))

Reg* regFind(DistanceBag& ds, Reg* reg)
{
    DISTANCEBAG_EACH(iter, ds)
    {
        if ( iter->reg_ == reg)
            return reg;
    }

    return 0;
}

void discardFarest(DistanceBag& ds)
{
    while (ds.size() > NUM_REGS)
        ds.erase( ds.begin() );
}

inline int infinity() 
{
    return std::numeric_limits<int>::max();
}

void subOne(int& i)
{
    if (i != infinity())
        --i;
}

//------------------------------------------------------------------------------

/*
 * constructor 
 */

Spiller::Spiller(Function* function)
    : CodePass(function)
    , spillCounter_(-1) // first allowed name
{}

/*
 * methods
 */

void Spiller::process()
{
    spill(cfg_->entry_);
    combine(cfg_->entry_);

    // now the remaining phi spilled relaods can be inserted;
    for (size_t i = 0; i < phiSpilledReloads_.size(); ++i)
    {
        insertReload(phiSpilledReloads_[i].bbNode_, phiSpilledReloads_[i].reg_, phiSpilledReloads_[i].appendTo_, false);
    }

    // rewire and reconstruct SSA form properly
    //reconstructionSSA(spills_);
    //reconstructionSSA(reloads_);
}

int Spiller::distance(BBNode* bbNode, Reg* reg, InstrNode* instrNode) 
{
    InstrBase* instr = instrNode->value_;
    AssignmentBase* ab = dynamic_cast<AssignmentBase*>(instr);

    // is reg used at instr
    if (ab)
    {
        if (ab->isRegUsed(reg))
            return 0;
    }
    // else

    return distanceRec(bbNode, reg, instrNode);
}

int Spiller::distanceRec(BBNode* bbNode, Reg* reg, InstrNode* instrNode)
{
    InstrBase* instr = instrNode->value_;
    BasicBlock* bb = bbNode->value_;

    // is reg live at instr?
    if ( instr->liveIn_.find(reg) == instr->liveIn_.end() ) 
        return infinity(); // no -> return "infinity"
    // else

    int result; // result goes here

    // is the current instruction a JumpInstr?
    if (dynamic_cast<JumpInstr*>(instr)) 
    {
        JumpInstr* ji = (JumpInstr*) instr;

        // collect and compute results of targets
        int results[ji->numTargets_];
        for (size_t i = 0; i < ji->numTargets_; ++i)
            results[i] = distance( ji->bbTargets_[i], reg, ji->instrTargets_[i] );

        result = *std::min_element(results, results + ji->numTargets_);
    }
    // is the current instruction the last ordinary instruction in this bb?
    else if (bb->end_->prev() == instrNode)
    {
        // -> visit the next bb
        swiftAssert(instrNode->next() != cfg_->instrList_.sentinel(), 
            "this may not be the last instruction");
        swiftAssert(bbNode->numSuccs() == 1, "should have exactly one succ");

        result = distance( bbNode->succ_.first()->value_, reg, instrNode->next() );
    }
    else 
    {
        // -> so we must have a "normal" successor instruction
        result = distance( bbNode, reg, instrNode->next() );
    }

    // do not count a LabelInstr
    int inc = (typeid(*instr) == typeid(LabelInstr)) ? 0 : 1;

    // add up the distance and do not calculate around
    result = (result == infinity()) ? infinity() : result + inc;

    return result;
}

/*
 * insertion of spills and reloads
 */

Reg* Spiller::insertSpill(BBNode* bbNode, Reg* reg, InstrNode* appendTo)
{
    // create a new memory location
    Reg* mem = function_->newMem(reg->type_, spillCounter_--);
    SpillMap::iterator iter = spillMap_.find(reg);

    // insert into spill map if reg is the first spill
    if (iter == spillMap_.end())
        spillMap_.insert( std::make_pair(reg, mem) );

    Spill* spill = new Spill(mem, reg);
    spills_.insert( RegDef(bbNode, cfg_->instrList_.insert(appendTo, spill)) );
    bbNode->value_->fixPointers();

    return mem;
}

void Spiller::insertReload(BBNode* bbNode, Reg* reg, InstrNode* appendTo, bool first)
{
    // TODO perhaps sth other than this bool arg would be more elegant
    if ( spillMap_.find(reg) == spillMap_.end() )
    {
        if (first) 
            phiSpilledReloads_.push_back( PhiSpilledReload( bbNode, reg, appendTo ));

        return;
    }

    Reg* mem = spillMap_[reg];
    swiftAssert( mem->isMem(), "must be a memory reg" );

    Reload* reload = new Reload(reg, mem);
    reloads_.insert( RegDef(bbNode, cfg_->instrList_.insert(appendTo, reload)) );
    bbNode->value_->fixPointers();
}

/*
 * local spilling
 */

void Spiller::spill(BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;

    /*
     * passed should contain all regs live in at bb 
     * and the results of phi operations in bb
     */
    RegSet passed = bb->liveIn_;

    // for each PhiInstr in bb
    for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
            "must be a PhiInstr here" );

        PhiInstr* phi = (PhiInstr*) iter->value_;
        
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

    // put in all regs from passed and calculate the distance to its next use
    REGSET_EACH(iter, passed)
    {
        // because inRegs doesn't contain regs of phi's rhs, we can start with firstOrdinary_
        currentlyInRegs.insert( RegAndDistance(*iter, distance(bbNode, *iter, bb->firstOrdinary_)) );
    }

    /*
     * use the first NUM_REGS registers if the set is too large
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
        swiftAssert(dynamic_cast<AssignmentBase*>(iter->value_),
                "must be an AssignmentBase here");

        // points to the current instruction
        InstrBase* instr = iter->value_;
        AssignmentBase* ab = (AssignmentBase*) instr;

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
        for (size_t i = 0; i < ab->numRhs_; ++i)
        {
            if (typeid(*ab->rhs_[i]) != typeid(Reg))
                continue;

            /*
             * is this reg currently not in a real register?
             */
            Reg* reg = (Reg*) ab->rhs_[i];

            if ( regFind(currentlyInRegs, reg) == 0 )
            {
                /*
                 * insert reload instruction
                 */
                insertReload(bbNode, reg, lastInstrNode, true);
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

        // numRemove -> number of regs which must be removed from currentlyInRegs
        int numRemove = std::max( int(ab->numLhs_) + numReloads + int(currentlyInRegs.size()) - NUM_REGS, 0 );

        /*
         * insert spills
         */
        for (int i = 0; i < numRemove; ++i)
        {
            // insert spill instruction
            Reg* toBeSpilled = currentlyInRegs.begin()->reg_;
            insertSpill(bbNode, toBeSpilled, lastInstrNode);

            // remove first reg
            currentlyInRegs.erase( currentlyInRegs.begin() );

            // manage inB and inRegs
            RegSet::iterator regIter = inRegs.find(toBeSpilled);
            if ( regIter != inRegs.end() )
                inRegs.erase(regIter); // reg is not used befor 
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
                dist = distance(bbNode, regIter->reg_, iter);

            newBag.insert( RegAndDistance(regIter->reg_, dist) );
        }

        currentlyInRegs = newBag;

        // add results to currentlyInRegs
        for (size_t i = 0; i < ab->numLhs_; ++i)
        {
            Reg* reg = ab->lhs_[i];
            currentlyInRegs.insert( RegAndDistance(reg, distance(bbNode, reg, iter)) );
        }

    } // for each instr

    // put in remaining regs from inRegs to inB
    inB.insert( inRegs.begin(), inRegs.end() );
    
    swiftAssert( out_.find(bbNode) == out_.end(), "already inserted" );
    RegSet& outB = out_.insert( std::make_pair(bbNode, RegSet()) ).first->second;

    DISTANCEBAG_EACH(regIter, currentlyInRegs)
        outB.insert(regIter->reg_);
    
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
    RegSet phiResults;
    
    // for each phi function
    for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
                "must be a phi instruction here");
        PhiInstr* phi = (PhiInstr*) iter->value_;
        phiResults.insert( phi->result() );
        
        bool phiSpill;
        RegSet::iterator inBIter = inB.find( phi->result() );

        if ( inBIter == inB.end() )
            phiSpill = true;
        else
        {
            phiSpill = false;
            inB.erase(inBIter);
        }

        // for each arg
        CFG::Relative* prePhiRelative = bbNode->pred_.first();
        for (size_t i = 0; i < phi->numRhs_; ++i)
        {
            BBNode* prePhiNode = prePhiRelative->value_;
            BasicBlock* prePhi = prePhiNode->value_;
            InstrNode* lastNonJump = prePhi->getLastNonJump();
            RegSet& preOut = out_[prePhiNode];

            // get phi argument
            swiftAssert( typeid(*phi->rhs_[i]) == typeid(Reg),
                    "must be a Reg here" );
            Reg* phiArg = (Reg*) phi->rhs_[i];
            swiftAssert( !phiArg->isMem(),  "must not be a memory location" );

            if ( phiSpill && preOut.find(phiArg) != preOut.end() )
            {
                // -> we have a phi spill and phiArg in preOut
                preOut.erase(phiArg);

                // insert spill to predecessor basic block and replace phi arg
                phi->rhs_[i] = insertSpill( prePhiNode, phiArg, lastNonJump );
            }
            else if( !phiSpill && preOut.find(phiArg) == preOut.end() )
            {   
                // -> we have no phi spill and phiArg not in preOut
                // insert reload to predecessor basic block
                insertReload( prePhiNode, phiArg, lastNonJump, true ); // insert reload
            }

            // traverse to next predecessor
            prePhiRelative = prePhiRelative->next();
        }
        swiftAssert( prePhiRelative == bbNode->pred_.sentinel(),
                "all nodes must be traversed")

        if (phiSpill)
        {
            // convert phi result to a memory location
            phi->result()->color_ = Reg::MEMORY_LOCATION;
            // add to spills_
            spills_.insert( RegDef(bbNode, iter) );
        }
    }

    // for each predecessor of the current node
    CFG_RELATIVES_EACH(predIter, bbNode->pred_)
    {
        BBNode* predNode = predIter->value_;
        BasicBlock* pred = predNode->value_;
        RegSet& outP = out_[predNode];

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
             // place reloads in this block on the top
            InstrNode* appendTo;
            BBNode* bbAppend;

            if ( !bb->hasPhiInstr() )
            {
                bbAppend = bbNode;
                appendTo = bb->begin_;
            }
            else
            {
                swiftAssert(predNode->succ_.size() == 1, "must exactly have one successor");
                // append to last non phi instruction belonging to pred
                bbAppend = predNode;
                appendTo = pred->getLastNonJump();
            }

            // for each reload
            for (size_t i = 0;  i < reloads.size(); ++i)
            {
                // create and insert reload
                Reg* reg = reloads[i];
                insertReload(bbAppend, reg, appendTo, true);
            }
        }

        /*
         * handle spills
         */

        // build vector which holds all necessary spills
        RegVec spills( outP.size() );
        end = std::set_difference( 
                outP.begin(), outP.end(), inB.begin(), inB.end(), spills.begin() );
        spills.erase( end, spills.end() ); // truncate properly

        if (spills.size() != 0)
        {
            // for each spill
            for (size_t i = 0;  i < spills.size(); ++i)
            {
                Reg* reg = spills[i];
                insertSpill(predNode, reg, pred->getLastNonJump() );
            } // for each spill
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
 * SSA reconstruction
 */

void Spiller::reconstructSSAForm(const RegDefSet& defs)
{
    BBSet f;

    REGDEFSET_EACH(iter, defs)
        f.insert(iter->bbNode_);
    
    BBSet i = cfg_->calcIteratedDomFrontier(f);

}


} // namespace me
