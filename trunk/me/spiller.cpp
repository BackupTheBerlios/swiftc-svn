/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

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
    VDUMAP_EACH(iter, spills_)
        delete iter->second;

    VDUMAP_EACH(iter, reloads_)
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
        Var* var             = laterReloads_[i].var_;
        InstrNode* instrNode = laterReloads_[i].instrNode_;
        BBNode* bbNode       = laterReloads_[i].bbNode_;

        insertSpillIfNecessarry(var, bbNode);
        insertReload(bbNode, var, instrNode);
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
            Var* phiArg = (Var*) phi->arg_[j].op_;

            // check whether a reload uses this as use
            VDUMAP_EACH(iter, reloads_)
            {
                VarDefUse* vdu = iter->second;

                DEFUSELIST_EACH(iter, vdu->uses_)
                {
                    DefUse& use = iter->value_;
                    if ( use.var_ == phiArg && use.instrNode_ == phiNode ) 
                    {
                        DefUseList::Node* eraseIter = iter;
                        iter = iter->prev();
                        vdu->uses_.erase(eraseIter);
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
        phi->arg_[arg].op_ = spillMap_[ (Var*) phi->arg_[arg].op_ ];
    }

    // register phiSpill uses
    for (size_t i = 0; i < phiSpills_.size(); ++i)
    {
        Var* orignal = phiSpills_[i].var_;
        InstrNode* phiNode = phiSpills_[i].instrNode_;
        InstrBase* phi = phiNode->value_;
        BBNode* bbNode = phiSpills_[i].bbNode_;

        for (size_t j = 0; j < phi->arg_.size(); ++j)
        {
            Var* phiArg = (Var*) phi->arg_[j].op_;
            spills_[orignal]->uses_.append( DefUse(phiArg, phiNode, bbNode) );
        }
    }

    // register all spills as uses for the reloads_ set
    VDUMAP_EACH(iter, spills_)
    {
        Var* var = iter->first;
        swiftAssert( var->typeCheck(typeMask_), "wrong var type" );
        VarDefUse* vdu = iter->second;

        DEFUSELIST_EACH(defIter, vdu->defs_)
        {
            DefUse& def = defIter->value_;
            VDUMap::iterator vduIter = reloads_.find(var);

            if ( vduIter != reloads_.end() )
                vduIter->second->uses_.append( DefUse(var, def.instrNode_, def.bbNode_) );
        }
    }

    /*
     * rewire and reconstruct SSA form properly
     */

    VDUMAP_EACH(iter, spills_)
    {
        VarDefUse* vdu = iter->second;
        cfg_->reconstructSSAForm(vdu);
    }

    VDUMAP_EACH(iter, reloads_)
    {
        VarDefUse* vdu = iter->second;
        cfg_->reconstructSSAForm(vdu);
    }
}

/*
 * insertion of spills and reloads
 */

Var* Spiller::insertSpill(BBNode* bbNode, Var* var, InstrNode* appendTo)
{
    swiftAssert( var->typeCheck(typeMask_), "wrong var type" );
    BasicBlock* bb = bbNode->value_;

    // create a new memory location
    Var* mem = function_->newSpilledSSAReg(var->type_);
    SpillMap::iterator iter = spillMap_.find(var);

    Spill* spill = new Spill(mem, var);
    InstrNode* spillNode = cfg_->instrList_.insert(appendTo, spill);
    bb->fixPointers();

    // insert into spill map if var is the first spill
    if (iter == spillMap_.end())
    {
        // yes -> so create new entry
        spillMap_.insert( std::make_pair(var, mem) );
        swiftAssert( spills_.find(var) == spills_.end(), "must be found here" );

        VarDefUse* vdu = new VarDefUse();
        vdu->defs_.append( DefUse(mem, spillNode, bbNode) ); // newly created definition
        spills_[var] = vdu;
    }
    else
    {
        // nope -> so use the one already there
        swiftAssert( spills_.find(var) != spills_.end(), "must be found here" );
        VarDefUse* vdu = spills_.find(var)->second;
        vdu->defs_.append( DefUse(mem, spillNode, bbNode) ); // newly created definition
    }

    bb->fixPointers();

    return mem;
}

void Spiller::insertReload(BBNode* bbNode, Var* var, InstrNode* appendTo)
{
    swiftAssert( var->typeCheck(typeMask_), "wrong var type" );
    swiftAssert( spillMap_.find(var) != spillMap_.end(), "must be in the spillMap_" )

    Var* mem = spillMap_[var];
    swiftAssert( mem->isSpilled(), "must be a memory var" );

    Var* newVar = function_->cloneNewSSA(var);

    Reload* reload = new Reload(newVar, mem);
    InstrNode* reloadNode = cfg_->instrList_.insert(appendTo, reload);
    bbNode->value_->fixPointers();

    // keep account of new use
    spills_.find(var)->second->uses_.append( DefUse(mem, reloadNode, bbNode) );

    /*
     * collect def-use infos
     */
    VDUMap::iterator iter = reloads_.find(var);

    // is this the first reload of var?
    if ( iter == reloads_.end() )
    {
        // yes -> so create new entry
        VarDefUse* vdu = new VarDefUse();
        vdu->defs_.append( DefUse(newVar, reloadNode, bbNode) ); // newly created definition
        vdu->defs_.append( DefUse(var, var->def_.instrNode_, var->def_.bbNode_) ); // orignal definition
        vdu->uses_ = var->uses_;
        reloads_[var] = vdu;
    }
    else
    {
        // nope -> so use the one already there
        VarDefUse* vdu = iter->second;
        vdu->defs_.append( DefUse(newVar, reloadNode, bbNode) ); // newly created definition
    }

    bbNode->value_->fixPointers();
}

/*
 * distance calculation via Belady
 */

int Spiller::distance(BBNode* bbNode, Var* var, InstrNode* instrNode)
{
    JumpInstr* ji = dynamic_cast<JumpInstr*>(instrNode->value_);
    if (ji)
    {
        int min = infinity();

        for (size_t i = 0; i < ji->numTargets_; ++i)
        {
            BBNode* target = ji->bbTargets_[i];
            BBSet walked;
            walked.insert(target);

            int dist = distanceHere(target, var, ji->instrTargets_[i], walked);
            min = (dist < min) ? dist : min;
        }

        return min;
    }
    // else

    BBSet walked;
    walked.insert(bbNode);
    return distanceHere( bbNode, var, instrNode->next(), walked );
}

int Spiller::distanceHere(BBNode* bbNode, Var* var, InstrNode* instrNode, BBSet walked)
{
    swiftAssert( var->typeCheck(typeMask_), "wrong var type" );
    InstrBase* instr = instrNode->value_;

    if (instr->isVarUsed(var))
        return 0;
    // else

    return distanceRec(bbNode, var, instrNode, walked);
}

int Spiller::distanceRec(BBNode* bbNode, Var* var, InstrNode* instrNode, BBSet walked)
{
    swiftAssert( var->typeCheck(typeMask_), "wrong var type" );
    InstrBase* instr = instrNode->value_;
    BasicBlock* bb = bbNode->value_;

    // is var live at instr?
    if ( !instr->liveIn_.contains(var) ) 
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
                results[i] = distanceHere( target, var, ji->instrTargets_[i], walked );
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
            result = distanceHere( succ, var, instrNode->next(), walked );
        }

    }
    else 
    {
        // -> so we must have a "normal" successor instruction
        result = distanceHere( bbNode, var, instrNode->next(), walked );
    }

    // do not count a LabelInstr or a PhiInstr
    int inc = 1;
    
    if (    typeid(*instr) == typeid(LabelInstr)
         || typeid(*instr) == typeid(PhiInstr))
        inc = 0;

    // add up the distance and do not calculate around
    result = (result == infinity()) ? infinity() : result + inc;

    return result;
}

/*
 * local spilling
 */

Var* Spiller::varFind(Spiller::DistanceBag& ds, Var* var)
{
    swiftAssert( var->typeCheck(typeMask_), "wrong var type" );
    DISTANCEBAG_EACH(iter, ds)
    {
        if ( iter->var_ == var)
            return var;
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
     * passed should contain all vars live in at bb 
     * and the results of phi operations in bb
     */
    VarSet passedAll = bb->liveIn_;

    // erase those vars in passed which should not be considered during this pass
    VarSet passed;
    VARSET_EACH(iter, passedAll)
    {
        Var* var = *iter;

        if ( var->typeCheck(typeMask_) )
            passed.insert(var);
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
    VarSet inVars;

    swiftAssert( in_.find(bbNode) == in_.end(), "already inserted" );
    VarSet& inB = in_.insert( std::make_pair(bbNode, VarSet()) ).first->second;
    DistanceBag currentlyInRegs;

    // put in all vars from passed and calculate the distance to its next use
    VARSET_EACH(iter, passed)
    {
        /*
         * because inVars doesn't contain vars of phi's arg, 
         * we can start with firstOrdinary_->prev() since distance starts with
         * the next instruction
         */
        currentlyInRegs.insert( VarAndDistance(*iter, 
                    distance(bbNode, *iter, bb->firstOrdinary_->prev())) );
    }

    /*
     * use the first numRegs_ registers if the set is too large
     */
    discardFarest(currentlyInRegs);

    // copy over to inVars
    DISTANCEBAG_EACH(varIter, currentlyInRegs)
        inVars.insert(varIter->var_);

    /*
     * currentlyInRegs holds all vars which are at the current point in vars. 
     * Initially it is assumed that all vars in currentlyInRegs can be kept in vars.
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
         * check whether all vars in arg are in regs 
         * and count necessary reloads
         */
        int numReloads = 0;
        for (size_t i = 0; i < instr->arg_.size(); ++i)
        {
            Reg* var = instr->arg_[i].op_->isReg(typeMask_);
            if (!var)
                continue;

            /*
             * is this var currently not in a real register?
             */

            if ( varFind(currentlyInRegs, var) == 0 )
            {
                /*
                 * insert reload instruction
                 */

                // check whether var hast been discaded but is actually in passed
                if ( !inB.contains(var) && passed.contains(var) )
                {
                    // mark for later reload insertion
                    laterReloads_.push_back( DefUse(var, lastInstrNode, bbNode) );
                }
                else
                    insertReload(bbNode, var, lastInstrNode);

                currentlyInRegs.insert( 
                        VarAndDistance(var, distance(bbNode, var, iter)) );

                // keep account of the number of needed reloads here
                ++numReloads;
            }
            else
            {
                /*
                 * manage inB and inVars
                 */
                VarSet::iterator varIter = inVars.find(var);
                if ( varIter != inVars.end() )
                {
                    // var is used before discarded
                    inB.insert(*varIter);
                    inVars.erase(varIter);
                }
            }
        }

        // count how many of instr->res_ have a proper type
        int numLhs = 0;
        for (size_t i = 0;  i < instr->res_.size(); ++i)
        {
            Var* var = instr->res_[i].var_;

            if ( var->typeCheck(typeMask_) )
                ++numLhs;
        }

        swiftAssert(size_t(numLhs) <= numRegs_, "not enough vars");
        // numRemove -> number of vars which must be removed from currentlyInRegs
        int numRemove = std::max( numLhs + numReloads + int(currentlyInRegs.size()) - int(numRegs_), 0 );

        /*
         * insert spills
         */
        for (int i = 0; i < numRemove; ++i)
        {
            Var* toBeSpilled = currentlyInRegs.begin()->var_;

            // manage inB and inVars
            VarSet::iterator varIter = inVars.find(toBeSpilled);
            if ( varIter != inVars.end() )
                inVars.erase(varIter); // var is not used before -> don't spill
            else
            {
                // insert spill instruction if toBeSpilled is used afterwards
                if ( instr->liveOut_.contains(toBeSpilled) )
                    insertSpill(bbNode, toBeSpilled, lastInstrNode);
            }

            // remove first var
            currentlyInRegs.erase( currentlyInRegs.begin() );
        }

        /*
         * update distances
         */

        DistanceBag newBag;
        DISTANCEBAG_EACH(varIter, currentlyInRegs)
        {
            int dist = varIter->distance_;
            subOne(dist);

            // recalculate distance if we have reached the next use
            if (dist < 0)
                dist = distance(bbNode, varIter->var_, iter);

            newBag.insert( VarAndDistance(varIter->var_, dist) );
        }

        currentlyInRegs = newBag;

        // add results to currentlyInRegs
        for (size_t i = 0; i < instr->res_.size(); ++i)
        {
            Var* var = instr->res_[i].var_;

            if ( var->typeCheck(typeMask_) )
            {
                if ( !instr->liveOut_.contains(var) )
                    continue; // we don't need vars for pointless definitions

                currentlyInRegs.insert( 
                        VarAndDistance(var, distance(bbNode, var, iter)) );
            }
        }
    } // for each instr

    // put in remaining vars from inVars to inB
    inB.insert( inVars.begin(), inVars.end() );
    
    swiftAssert( out_.find(bbNode) == out_.end(), "already inserted" );
    VarSet& outB = out_.insert( std::make_pair(bbNode, VarSet()) ).first->second;

    DISTANCEBAG_EACH(varIter, currentlyInRegs)
        outB.insert(varIter->var_);
    /*
     * use this for debugging for inB and outB
     */

#if 0
    std::cout << "!!!" << bb->name() << std::endl;

    std::cout << "\tinB:" << std::endl;
    VARSET_EACH(iter, inB)
        std::cout << "\t\t" << (*iter)->toString() << std::endl;

    std::cout << "\toutB:" << std::endl;
    VARSET_EACH(iter, outB)
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
    VarSet& inB = in_[bbNode];

    /*
     * handle phi functions
     */

    VarSet phiArgs;
    
    // for each phi function
    for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
                "must be a phi instruction here");
        PhiInstr* phi = (PhiInstr*) iter->value_;
        Var* phiRes = phi->result();
        
        if ( !phiRes->typeCheck(typeMask_) )
            continue;
        
        bool phiSpill;
        VarSet::iterator inBIter = inB.find(phiRes);

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
            VarSet& preOut = out_[prePhiNode];

            // get phi argument
            swiftAssert( dynamic_cast<Var*>(phi->arg_[i].op_), "must be a Var here" );
            Var* phiArg = (Var*) phi->arg_[i].op_;
            swiftAssert( !phiArg->isSpilled(), "must not be a memory location" );

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
                insertSpillIfNecessarry(phiArg, prePhiNode);
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
            swiftAssert( typeid(*phiRes) == typeid(Reg), "must be a Reg" );
            ((Reg*) phiRes)->isSpilled_ = true;

            // add to spills_
            VarDefUse* vdu = new VarDefUse();
            vdu->defs_.append( DefUse(phiRes, iter, bbNode) );
            swiftAssert( phiRes->typeCheck(typeMask_), "wrong var type" );
            spills_[phiRes] = vdu;
            spillMap_[phiRes] = phiRes; // phi-spills map to them selves

            for (size_t i = 0; i < phi->arg_.size(); ++i)
            {
                Var* var = (Reg*) phi->arg_[i].op_;
                phiSpills_.push_back( DefUse(var, iter, bbNode) );
            }
        }
    }

    // for each predecessor of the current node
    CFG_RELATIVES_EACH(predIter, bbNode->pred_)
    {
        BBNode* predNode = predIter->value_;
        BasicBlock* pred = predNode->value_;
        VarSet& outP = out_[predNode];

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
        VarVec reloads( inB.size() );
        VarVec::iterator end = std::set_difference( 
                inB.begin(), inB.end(), outP.begin(), outP.end(), reloads.begin() );
        reloads.erase( end, reloads.end() );// truncate properly

        // for each reload
        for (size_t i = 0;  i < reloads.size(); ++i)
        {
            // create and insert reload
            Var* var = reloads[i];
            swiftAssert( var->typeCheck(typeMask_), "wrong var type" );

            // ensure that every reload is dominated by a spill
            insertSpillIfNecessarry(var, predNode);
            insertReload( bbAppend, var, appendTo );
        }
    } // for each predecessor

    // for each child of bb in the dominator tree
    BBLIST_EACH(bbIter, bb->domChildren_)
    {
        BBNode* domChild = bbIter->value_;
        combine(domChild);
    }
}


// helper

namespace {

bool isSpilled(Var* var, BBNode* bbNode)
{
    BasicBlock* bb = bbNode->value_;

    // is it phi-spilled?
    for (InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; iter = iter->next())
    {
        swiftAssert( typeid(*iter->value_) == typeid(PhiInstr), 
                "must be a PhiInstr" );
        PhiInstr* phi = (PhiInstr*) iter->value_;
        Var* res = phi->result();

        if (res == var)
            return true; // found
    }

    // is it spilled by an ordinary spill?
    for (InstrNode* iter = bb->firstOrdinary_; iter != bb->end_; iter = iter->next())
    {
        if ( typeid(*iter->value_) != typeid(Spill) )
            continue;

        Spill* spill = (Spill*) iter->value_;

        if ( ((Reg*) spill->arg_[0].op_) == var )
            return true; // found
    }

    // not spilled in this basic block
    return false;
}

}

void Spiller::insertSpillIfNecessarry(Var* var, BBNode* bbNode)
{
    if ( spillMap_.find(var) != spillMap_.end() )
    {
        // -> in this case we need to check whether we have a dominating spill

        /*
         * go up dominance tree until we found the first dominating block 
         * which has a spill of var
         */
        while ( bbNode != cfg_->entry_ && !isSpilled(var, bbNode) )
            bbNode = cfg_->idoms_[bbNode->postOrderIndex_];

        if (bbNode == cfg_->entry_)
        {
            if ( isSpilled(var, bbNode) )
                return;
        }
        else 
            return;
    }

    // no dominating spill found -> insert one
    InstrNode* appendTo = var->def_.instrNode_;
    if ( typeid(*appendTo->value_) == typeid(PhiInstr) )
    {
        appendTo = var->def_.bbNode_->value_->firstOrdinary_->prev();
    }
    insertSpill(var->def_.bbNode_, var, appendTo);
}

} // namespace me
