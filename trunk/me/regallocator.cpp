#include "me/regallocator.h"

#include <algorithm>
#include <limits>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

namespace me {

/*
 * constructor 
 */

RegAllocator::RegAllocator(Function* function)
    : CodePass(function)
    , spillCounter_(-1) // first allowed name
{}

/*
 * methods
 */

/*
 * spilling
 */

// TODO
#define NUM_REGS 2

/*
 * needed for sorting via the distance method
 */
struct RegAndDistance 
{
    Reg* reg_;
    int distance_;

    RegAndDistance() {}
    RegAndDistance(Reg* reg, int distance)
        : reg_(reg)
        , distance_(distance)
    {}
    // copy constructor
    RegAndDistance(const RegAndDistance& rd)
        : reg_(rd.reg_)
        , distance_(rd.distance_)
    {}

    // needed by std::sort
    bool operator < (const RegAndDistance& r) const
    {
        return distance_ > r.distance_; // sort farest first
    }
};

// TODO perhaps use two synced sets for better performance:
// - one sorted by distance
// - one sorted by reg

typedef std::set<RegAndDistance> DistanceSet;
Reg* regFind(DistanceSet& ds, Reg* reg)
{
    for (DistanceSet::iterator iter = ds.begin(); iter != ds.end(); ++iter)
    {
        if ( iter->reg_ == reg)
            return reg;
    }

    return 0;
}

void discardFarest(DistanceSet& ds)
{
    while (ds.size() > NUM_REGS)
        ds.erase( ds.begin() );
}

void subOne(int& i)
{
    if (i != RegAllocator::infinity())
        --i;
}

int RegAllocator::distance(BBNode* bbNode, Reg* reg, InstrNode* instrNode) 
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

int RegAllocator::distanceRec(BBNode* bbNode, Reg* reg, InstrNode* instrNode)
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

void RegAllocator::process()
{
    BB2RegSet in;
    BB2RegSet out;
    spill(cfg_->entry_, in, out);

    /*
     * combine results
     */

    // for each basic block
    CFG_RELATIVES_EACH(iter, cfg_->nodes_)
    {
        BBNode* bbNode = iter->value_;
        BasicBlock* bb = bbNode->value_;
        RegSet& inB = in[bbNode];

        // for each predecessor of the current node
        CFG_RELATIVES_EACH(predIter, bbNode->pred_)
        {
            BBNode* predNode = predIter->value_;
            BasicBlock* pred = predNode->value_;
            RegSet& outP = in[predNode];

            // build vector which holds all necessary reloads
            std::vector<Reg*> reloads( std::max(inB.size(), outP.size()) );
            std::vector<Reg*>::iterator end = std::set_intersection( 
                    inB.begin(), inB.end(), outP.begin(), outP.end(), reloads.begin() );
            reloads.erase( end, reloads.end() );// truncate properly

            if (reloads.size() == 0)
                continue; // nothing to do in this case

            /*
             * case 1: no phi functions -> place reloads in this block on the top
             *
             * case 2: There is a phi function so place the reload in the appropriate
             * predecessor block. Note that this predecessor can only have one successor:
             * This block. The dead edge elimination pass guarantees this.
             */
            InstrNode* appendTo;

            if (bb->firstPhi_ == bb->firstOrdinary_)
            {
                appendTo = bb->begin_;
            }
            else
            {
                swiftAssert(predNode->succ_.size() == 1, "must exactly have one successor");
                appendTo = pred->end_->prev();
            }

            // for each reload
            for (size_t i = 0;  i < reloads.size(); ++i)
            {
                // create and insert reload
                Reg* reg = reloads[i];
                swiftAssert(spillMap_.find(reg) != spillMap_.end(),
                    "var must be found here");
                Reg* mem = spillMap_[reg];
                swiftAssert( mem->isMem(), "must be a memory var" );

                Reload* reload = new Reload(reg, mem);
                cfg_->instrList_.insert(appendTo, reload);
            }
        } // for each predecessor
    } // for each basic block
}

void RegAllocator::spill(BBNode* bbNode, BB2RegSet& in, BB2RegSet& out)
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

    swiftAssert( in.find(bbNode) == in.end(), "already inserted" );
    RegSet& inB = in.insert( std::make_pair(bbNode, RegSet()) ).first->second;
    DistanceSet currentlyInRegs;

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
    for (DistanceSet::iterator regIter = currentlyInRegs.begin(); regIter != currentlyInRegs.end(); ++regIter)
        inRegs.insert(regIter->reg_);

    /*
     * currentlyInRegs holds all vars which are at the current point in regs. 
     * Initially it is assumed that all vars in currentlyInRegs can be kept in regs.
     */

    // traverse all ordinary instructions in bb
    InstrNode* iter = bb->firstOrdinary_;
    while (iter != bb->end_)
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
         * check whether all vars on the rhs are in regs 
         * and count necessary reloads
         */
        int numReloads = 0;
        for (size_t i = 0; i < ab->numRhs_; ++i)
        {
            if (typeid(*ab->rhs_[i]) != typeid(Reg))
                continue;

            /*
             * is this var currently not in a real register?
             */
            Reg* var = (Reg*) ab->rhs_[i];

            if (regFind(currentlyInRegs, var) == 0)
            {
                /*
                 * insert reload instruction
                 */
                swiftAssert(spillMap_.find(var) != spillMap_.end(),
                    "var must be found here");
                Reg* mem = spillMap_[var];
                swiftAssert( mem->isMem(), "must be a memory var" );

                Reload* reload = new Reload(var, mem);
                reloads_.insert( cfg_->instrList_.insert(lastInstrNode, reload) );

                currentlyInRegs.insert( RegAndDistance(var, 0) );

                // keep account of the number of needed reloads here
                ++numReloads;
            }
            else
            {
                /*
                 * manage inB and inRegs
                 */
                RegSet::iterator regIter = inRegs.find(var);
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
            Reg* mem = function_->newMem(toBeSpilled->type_, spillCounter_--);

            // insert into spill map if toBeSpilled is the first spill
            if ( spillMap_.find(toBeSpilled) == spillMap_.end() )
                spillMap_[toBeSpilled] = mem;

            Spill* spill = new Spill(mem, toBeSpilled);
            spills_.insert( cfg_->instrList_.insert(lastInstrNode, spill) );

            // remove first reg
            currentlyInRegs.erase( currentlyInRegs.begin() );

            /*
             * manage inB and inRegs
             */
            RegSet::iterator regIter = inRegs.find(toBeSpilled);
            if ( regIter != inRegs.end() )
                inRegs.erase(regIter); // reg is not used befor 
        }

        // go to next instruction
        iter = iter->next();
        if (iter == bb->end_)
            break; // exit loop here
        
        /*
         * update distances
         */

        DistanceSet newSet;
        for (DistanceSet::iterator regIter = currentlyInRegs.begin(); regIter != currentlyInRegs.end(); ++regIter)
        {
            int dist = regIter->distance_;
            subOne(dist);

            // recalculate distance if we have reached the next use
            if (dist < 0)
                dist = distance(bbNode, regIter->reg_, iter);

            newSet.insert( RegAndDistance(regIter->reg_, dist) );
        }

        currentlyInRegs = newSet;

        // add results to currentlyInRegs
        for (size_t i = 0; i < ab->numLhs_; ++i)
        {
            Reg* reg = ab->lhs_[i];
            currentlyInRegs.insert( RegAndDistance(reg, distance(bbNode, reg, iter)) );
        }

    } // for each instr
    
    swiftAssert( out.find(bbNode) == out.end(), "already inserted" );
    RegSet& outB = out.insert( std::make_pair(bbNode, RegSet()) ).first->second;

    for (DistanceSet::iterator regIter = currentlyInRegs.begin(); regIter != currentlyInRegs.end(); ++regIter)
        outB.insert(regIter->reg_);
    
    // for each child of bb in the dominator tree
    BBLIST_EACH(bbIter, bb->domChildren_)
    {
        BBNode* domChild = bbIter->value_;
        spill(domChild, in, out);
    }
}

} // namespace me
