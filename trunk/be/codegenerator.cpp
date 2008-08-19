#include "be/codegenerator.h"

#include <algorithm>
#include <limits>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

namespace be {


//------------------------------------------------------------------------------

#ifdef SWIFT_DEBUG

#include <sstream>

struct IVar
{
    me::Reg* var_;

    IVar() {}
    IVar(me::Reg* var)
        : var_(var)
    {}

    std::string name() const
    {
        return var_->toString();
    }

    std::string toString() const
    {
        std::ostringstream oss;
        oss << var_->toString() << " : " << var_->color_;
        return oss.str();
    }
};

struct IGraph : public Graph<IVar>
{
    IGraph(const std::string& name)
        : Graph<IVar>(true)
        , name_(name)
    {
        // make the id readable for dot
        for (size_t i = 0; i < name_.size(); ++i)
        {
            if (name_[i] == '#')
                name_[i] = '_';
        }
    }

    virtual std::string name() const
    {
        return std::string("ig_") + name_;
    }

    std::string name_;
};

typedef Graph<IVar>::Node* VarNode;

#endif // SWIFT_DEBUG

//------------------------------------------------------------------------------

/*
 * init statics
 */

Spiller* CodeGenerator::spiller_ = 0;

/*
 * constructor and destructor
 */

CodeGenerator::CodeGenerator(std::ofstream& ofs, me::Function* function)
    : function_(function)
    , cfg_(&function->cfg_)
    , ofs_(ofs)
#ifdef SWIFT_DEBUG
    , ig_( new IGraph(*function->id_) )
#endif // SWIFT_DEBUG
{}

#ifdef SWIFT_DEBUG

CodeGenerator::~CodeGenerator()
{
    delete ig_;
}

#endif // SWIFT_DEBUG

/*
 * methods
 */

void CodeGenerator::genCode()
{
    // init spiller
    spiller_->setFunction(function_);

    // traverse the code generation pipe
    //std::cout << std::endl << *function_->id_ << std::endl;
    livenessAnalysis();
    //spill();
    color();
#ifdef SWIFT_DEBUG
    ig_->dumpDot( ig_->name() );
#endif // SWIFT_DEBUG
    coalesce();
}

/*
 * liveness stuff
 */

void CodeGenerator::livenessAnalysis()
{
#ifdef SWIFT_DEBUG
    // create var nodes
    REGMAP_EACH(iter, function_->vars_)
    {
        me::Reg* var = iter->second;

        VarNode varNode = ig_->insert( new IVar(var) );
        var->varNode_ = varNode;
    }
#endif // SWIFT_DEBUG

    // for each var
    REGMAP_EACH(iter, function_->vars_)
    {
        me::Reg* var = iter->second;

        // for each use of var
        USELIST_EACH(iter, var->uses_)
        {
            me::DefUse& use = iter->value_;
            me::InstrBase* instr = use.instr_->value_;

            if ( typeid(*instr) == typeid(me::PhiInstr) )
            {
                me::PhiInstr* phi = (me::PhiInstr*) instr;

                // find the predecessor basic block
                size_t i = 0;
                while (phi->rhs_[i] != var)
                    ++i;

                swiftAssert(i < phi->numRhs_, "i too large here");
                me::BBNode* pred = phi->sourceBBs_[i];

                // examine the found block
                liveOutAtBlock(pred, var);
            }
            else
                liveInAtInstr(use.instr_, var);
        }

        // clean up
        walked_.clear();
    }

    /* 
     * use this for debugging of liveness stuff
     */
    
#if 0
    std::cout << "INSTRUCTIONS:" << std::endl;
    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        me::InstrBase* instr = iter->value_;
        std::cout << instr->toString() << std::endl;
        std::cout << instr->livenessString() << std::endl;
    }

    std::cout << std::endl << "BASIC BLOCKS:" << std::endl;

    CFG_RELATIVES_EACH(iter, cfg_->nodes_)
    {
        me::BasicBlock* bb = iter->value_->value_;
        std::cout << bb->name() << std::endl;
        std::cout << bb->livenessString() << std::endl;
    }
#endif
}

void CodeGenerator::liveOutAtBlock(me::BBNode* bbNode, me::Reg* var)
{
    me::BasicBlock* bb = bbNode->value_;

    // var is live-out at bb 
    bb->liveOut_.insert(var);

    // if bb not in walked_
    if ( walked_.find(bb) == walked_.end() )
    {
        walked_.insert(bb);
        liveOutAtInstr(bb->end_->prev(), var);
    }
}

void CodeGenerator::liveOutAtInstr(me::InstrNode* instr, me::Reg* var)
{
    // var is live-out at instr
    //if (!phi)
    instr->value_->liveOut_.insert(var);

    // knows whether var is defined in instr
    bool varInLhs = false;

    me::AssignmentBase* ab = dynamic_cast<me::AssignmentBase*>(instr->value_);
    // for each reg v, that ab defines and is not var itself
    if (ab)
    {
        for (size_t i = 0; i < ab->numLhs_; ++i)
        {
            if ( ab->lhs_[i] != var )
            {
#ifdef SWIFT_DEBUG
                me::Reg* res = ab->lhs_[i];

                // add (v, w) to interference graph if it does not already exist
                if (   res->varNode_->succ_.find(var->varNode_) == res->varNode_->succ_.sentinel()
                    && var->varNode_->succ_.find(res->varNode_) == var->varNode_->succ_.sentinel() )
                {
                    var->varNode_->link(res->varNode_);
                }
#endif // SWIFT_DEBUG
            }
            else
                varInLhs = true;
        }
    }

    if (!varInLhs)
        liveInAtInstr(instr, var);
}

void CodeGenerator::liveInAtInstr(me::InstrNode* instr, me::Reg* var)
{
    // var is live-in at instr
    instr->value_->liveIn_.insert(var);
    me::InstrNode* prevInstr = instr->prev();

    // is instr the first statement of basic block?
    if ( typeid(*prevInstr->value_) == typeid(me::LabelInstr) )
    {
        // var is live-out at the leading labelInstr
        prevInstr->value_->liveOut_.insert(var);

        // insert var to this basic block's liveIn
        me::BBNode* bb = function_->cfg_.labelNode2BBNode_[prevInstr];
        bb->value_->liveIn_.insert(var);

        // for each predecessor of bb
        CFG_RELATIVES_EACH(iter, bb->pred_)
            liveOutAtBlock(iter->value_, var);
    }
    else
    {
        // get preceding statement to instr
        me::InstrNode* preInstr = instr->prev();
        liveOutAtInstr(preInstr, var);
    }
}

/*
 * spilling
 */

// TODO
#define NUM_REGS 16

void CodeGenerator::spill()
{

    std::set<me::Reg*> varsCurrentlyInRegs;

    // for each basic block
    for (size_t i = 0; i < cfg_->postOrder_.size(); ++i)
    {
        me::BBNode* bb = cfg_->postOrder_[i]; // get current basic block

        spill(bb);
    }
}

/*
 * needed for sorting via the distance method
 */
struct RegAndDistance 
{
    me::Reg* reg_;
    int distance_;

    RegAndDistance() {}
    RegAndDistance(me::Reg* reg, int distance)
        : reg_(reg)
        , distance_(distance)
    {}

    // needed by std::sort
    bool operator < (const RegAndDistance& r) const
    {
        return distance_ < r.distance_;
    }
};

void CodeGenerator::spill(me::BBNode* bbNode)
{
    me::BasicBlock* bb = bbNode->value_;

    /*
     * passed contains all regs live in at bb 
     * and the results of phi operations in bb
     */
    std::set<me::Reg*> passed = bb->liveIn_;

    // for each PhiInstr in bb
    for (me::InstrNode* iter = bb->firstPhi_; iter != bb->firstOrdinary_; ++iter)
    {
        swiftAssert( typeid(*iter->value_) == typeid(me::PhiInstr*), 
            "must be a PhiInstr here" );

        me::PhiInstr* phi = (me::PhiInstr*) iter->value_;
        
        // add result to the passed set
        passed.insert( phi->result() );
    }
    // passed now holds all proposed values

    /*
     * now we need the set which is allowed to be in real registers
     */
    std::vector<RegAndDistance> inRegs( passed.size() );

    // put in all regs from passed and calculate the distance to its next use
    size_t counter = 0;
    REGSET_EACH(iter, passed)
    {
        inRegs[counter] = RegAndDistance( *iter, distance(bbNode, *iter, bb->firstOrdinary_) );
        ++counter;
    }

    /*
     * sort and use the first NUM_REGS registers
     * if the inRegs set is too large
     */
    std::sort( inRegs.begin(), inRegs.end() );

    if (inRegs.size() > NUM_REGS)
        inRegs.erase( inRegs.begin() + NUM_REGS, inRegs.end() );

    // TODO
}

int CodeGenerator::distance(me::BBNode* bbNode, me::Reg* reg, me::InstrNode* instrNode) 
{
    me::InstrBase* instr = instrNode->value_;

    // do we have a LabelInstr here?
    while ( typeid(*instr) == typeid(me::LabelInstr) )
    {
        // look up new basic block
        bbNode = cfg_->labelNode2BBNode_[instrNode];
        me::BasicBlock* bb = bbNode->value_;
        swiftAssert( bb->begin_ == instrNode, 
            "bb's begin must point to this LabelInstr");
        
        // do not count this and move one further
        instrNode = instrNode->next();

        swiftAssert( cfg_->instrList_.sentinel() != instrNode, 
            "this must not be the sentinel" );
    }

    /*
     * PhiInstr and LabelInstr are nore allowd here
     */
    swiftAssert( typeid(*instr) != typeid(me::PhiInstr), 
        "this mustn't be a PhiInstr here" );
    swiftAssert( typeid(*instr) != typeid(me::LabelInstr), 
        "this mustn't be a LabelInstr here" );

    me::AssignmentBase* ab = dynamic_cast<me::AssignmentBase*>(instr);

    // is reg used at instr
    if (ab)
    {
        if (ab->isRegUsed(reg))
            return 0;
    }
    // else

    return distanceRec(bbNode, reg, instrNode);
}

int CodeGenerator::distanceRec(me::BBNode* bbNode, me::Reg* reg, me::InstrNode* instrNode)
{
    /*
     * TODO here is room for optimization, by elemianting the recursion and
     *  using loops instead.
     */

    me::InstrBase* instr = instrNode->value_;

    // reg is live at instr means: reg in liveIn of instr
    // is reg not live at instr?
    // ----> liveIn
    if ( instr->liveOut_.find(reg) == instr->liveOut_.end() ) 
        return infinity(); // return "infinity"
    // else

    // is the current instruction a JumpInstr?
    if (dynamic_cast<me::JumpInstr*>(instr)) 
    {
        me::JumpInstr* ji = (me::JumpInstr*) instr;

        int results[ji->numTargets_];

        for (size_t i = 0; i < ji->numTargets_; ++i)
            results[i] = distance( bbNode, reg, ji->instrTargets_[i] );

        int min =  *std::min_element(results, results + ji->numTargets_);

        // add up the distance and do not calculate around
        return min == infinity() ? infinity() : min + 1;
    }
    
    // the next instruction can't be the sentinel, reg can't be live then.
    swiftAssert(instrNode->next() != cfg_->instrList_.sentinel(), 
        "this mustn't be the instrList_'s sentinel" );
    
    /*
     * check what kind of instruction we have
     */
    //if ( typeid(*instr) == typeid(me::LabelInstr) )
        //return;

    // do we have an ordinary successor instruction?
    if ( typeid(*instr) != typeid(me::LabelInstr) )
    {
        int result = distanceRec( bbNode, reg, instrNode->next() );

        // add up the distance and do not calculate around
        return result == infinity() ? infinity() : result + 1;
    }
    // else

    swiftAssert( typeid(*instr) == typeid(me::LabelInstr), "must be a LabelInstr here" );

    // move to the LabelInstr
    instrNode = instrNode->next();
    instr = instrNode->value_;
    // for each succossor basic blocks
    //BBLIST_EACH(iter, bbNode->succ_)
    //{
        //instrNode = iter->value_->value_->begin_;

        //// omit leading label instr
        //swiftAssert( typeid(*instrNode->value_) == typeid(me::LabelInstr),
            //"first instruction in a basic block must be a LabelInstr" );
        //instrNode = instrNode->next();

        //int result = distanceRec( reg, instrNode->next() );

        //// add up the distance and do not calculate around
        //return result == infinity()
            //? std::numeric_limits<int>::max()
            //: result + 1;
    //}

    swiftAssert(false, "this code should not be reached");

    return 0;
}

/*
 * coloring
 */

int CodeGenerator::findFirstFreeColorAndAllocate(Colors& colors)
{
    int firstFreeColor = 0;
    for (Colors::iterator iter = colors.begin(); iter != colors.end(); ++iter)
    {
        if (*iter != firstFreeColor)
            break; // found a color
        ++firstFreeColor;
    }

    // either allocate the found color in the set or insert a new slot in the set
    colors.insert(firstFreeColor);

    return firstFreeColor;
}

void CodeGenerator::color()
{
    /*
     * start with the first true basic block
     * and perform a pre-order walk of the dominator tree
     */
    colorRecursive(cfg_->entry_);
}

void CodeGenerator::colorRecursive(me::BBNode* bb)
{
    Colors colors;

    // all vars in liveIn_ have already been colored
    REGSET_EACH(iter, bb->value_->liveIn_)
    {
        int color = (*iter)->color_;
        swiftAssert(color != -1, "color must be assigned here");
        // mark as occupied
        colors.insert(color);
    }

    // for each instruction 
    for (me::InstrNode* iter = bb->value_->firstPhi_; iter != bb->value_->end_; iter = iter->next())
    {
        me::AssignmentBase* ab = dynamic_cast<me::AssignmentBase*>(iter->value_);

        if (ab)
        {
#ifdef SWIFT_DEBUG
            /*
             * In the debug version this set knows vars which were already
             * removed. This allows more precise assertions (see below).
             */
            me::RegSet erased;
#endif // SWIFT_DEBUG

            if ( typeid(*ab) != typeid(me::PhiInstr) )
            {
                // for each var on the right hand side
                for (size_t i = 0; i < ab->numRhs_; ++i)
                {
                    me::Reg* reg = dynamic_cast<me::Reg*>(ab->rhs_[i]);

                    if ( reg && me::InstrBase::isLastUse(iter, reg) )
                    {
                        // -> its the last use of reg
                        Colors::iterator colorIter = colors.find(reg->color_);
#ifdef SWIFT_DEBUG
                        // has this reg already been erased due to a double entry like a = b + b?
                        if ( erased.find(reg) != erased.end() )
                            continue;

                        swiftAssert( colorIter != colors.end(), "color must be found here" );
                        colors.erase(colorIter); // last use of reg
                        erased.insert(reg);
#else // SWIFT_DEBUG
                        if ( colorIter != colors.end() )
                            colors.erase(colorIter); // last use of reg
                        /*
                        * else -> the reg must already been removed which must
                        *      be caused by a double entry like a = b + b
                        */
#endif // SWIFT_DEBUG
                    } // if last use
                } // for each rhs var
            } // if no phi instr

            // for each var on the left hand side -> assign a color for result
            for (size_t i = 0; i < ab->numLhs_; ++i)
            {
                me::Reg* reg = ab->lhs_[i];
                reg->color_ = findFirstFreeColorAndAllocate(colors);

                // pointless definitions should be optimized away
                if (ab->liveOut_.find(reg) == ab->liveOut_.end())
                    colors.erase( colors.find(reg->color_) );
            }
        }
    } // for each instruction

    // for each child of bb in the dominator tree
    for (me::BBList::Node* iter = bb->value_->domChildren_.first(); iter != bb->value_->domChildren_.sentinel(); iter = iter->next())
    {
        me::BBNode* domChild = iter->value_;

        colorRecursive(domChild);
    }
}

void CodeGenerator::coalesce()
{
}

} // namespace be
