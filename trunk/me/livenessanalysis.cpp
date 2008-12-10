#include "me/livenessanalysis.h"

#include <typeinfo>

namespace me {
    
//------------------------------------------------------------------------------

#ifdef SWIFT_DEBUG

#include <sstream>

struct IVar
{
    Reg* var_;

    IVar() {}
    IVar(Reg* var)
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
        // make the id readable for graphviz' dot
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
 * constructor and destructor
 */

LivenessAnalysis::LivenessAnalysis(Function* function)
    : CodePass(function)
#ifdef SWIFT_DEBUG
    , ig_( new IGraph(*function->id_) )
#endif // SWIFT_DEBUG
{}

#ifdef SWIFT_DEBUG

LivenessAnalysis::~LivenessAnalysis()
{
    delete ig_;
}

#endif // SWIFT_DEBUG

/*
 * methods
 */

void LivenessAnalysis::process()
{
    /*
     * clean up if necessary
     */
    if (function_->firstLiveness_)
    {
        CFG_RELATIVES_EACH(iter, cfg_->nodes_)
        {
            BasicBlock* bb = iter->value_->value_;

            bb->liveIn_.clear();
            bb->liveOut_.clear();
        }

        INSTRLIST_EACH(iter, cfg_->instrList_)
        {
            InstrBase* instr = iter->value_;
            instr->liveIn_.clear();
            instr->liveOut_.clear();
        }
    }

#ifdef SWIFT_DEBUG
    /*
     * create nodes for the inteference graph
     */
    REGMAP_EACH(iter, function_->vars_)
    {
        Reg* var = iter->second;

        VarNode varNode = ig_->insert( new IVar(var) );
        var->varNode_ = varNode;
    }
#endif // SWIFT_DEBUG

    /*
     * start here
     */

    // for each var
    REGMAP_EACH(iter, function_->vars_)
    {
        Reg* var = iter->second;

        // for each use of var
        USELIST_EACH(iter, var->uses_)
        {
            DefUse& use = iter->value_;
            InstrBase* instr = use.instrNode_->value_;

            if ( typeid(*instr) == typeid(PhiInstr) )
            {
                PhiInstr* phi = (PhiInstr*) instr;

                // find the predecessor basic block
                size_t i = 0;
                while (phi->rhs_[i].op_ != var)
                    ++i;

                swiftAssert(i < phi->rhs_.size(), "i too large here");
                BBNode* pred = phi->sourceBBs_[i];

                // examine the found block
                liveOutAtBlock(pred, var);
            }
            else
                liveInAtInstr(use.instrNode_, var);
        }

        // clean up
        walked_.clear();
    }

    /* 
     * use this for debugging of liveness stuff
     */
    
#if 0
    std::cout << "--- LIVENESS STUFF ---" << std::endl;
    std::cout << "INSTRUCTIONS:" << std::endl;
    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        InstrBase* instr = iter->value_;
        std::cout << instr->toString() << std::endl;
        std::cout << instr->livenessString() << std::endl;
    }

    std::cout << std::endl << "BASIC BLOCKS:" << std::endl;

    CFG_RELATIVES_EACH(iter, cfg_->nodes_)
    {
        BasicBlock* bb = iter->value_->value_;
        std::cout << bb->name() << std::endl;
        std::cout << bb->livenessString() << std::endl;
    }
#endif

#ifdef SWIFT_DEBUG
    ig_->dumpDot( ig_->name() );
#endif // SWIFT_DEBUG

    function_->firstLiveness_ = true;
}

void LivenessAnalysis::liveOutAtBlock(BBNode* bbNode, Reg* var)
{
    BasicBlock* bb = bbNode->value_;

    // var is live-out at bb 
    bb->liveOut_.insert(var);

    // if bb not in walked_
    if ( walked_.find(bb) == walked_.end() )
    {
        walked_.insert(bb);
        liveOutAtInstr(bb->end_->prev(), var);
    }
}

void LivenessAnalysis::liveOutAtInstr(InstrNode* instrNode, Reg* var)
{
    InstrBase* instr = instrNode->value_;

    // var is live-out at instr
    instr->liveOut_.insert(var);

    // knows whether var is defined in instr
    bool varInLhs = false;

    for (size_t i = 0; i < instr->lhs_.size(); ++i)
    {
        if ( instr->lhs_[i].reg_ != var )
        {
#ifdef SWIFT_DEBUG
            Reg* res = instr->lhs_[i].reg_;

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

    if (!varInLhs)
        liveInAtInstr(instrNode, var);
}

void LivenessAnalysis::liveInAtInstr(InstrNode* instr, Reg* var)
{
    // var is live-in at instr
    instr->value_->liveIn_.insert(var);

    // is instr the first statement of basic block?
    if ( typeid(*instr->value_) == typeid(LabelInstr) )
    {
        // var is live-out at the leading labelInstr
        instr->value_->liveOut_.insert(var);

        // insert var to this basic block's liveIn
        BBNode* bb = function_->cfg_.labelNode2BBNode_[instr];
        bb->value_->liveIn_.insert(var);

        // for each predecessor of bb
        CFG_RELATIVES_EACH(iter, bb->pred_)
            liveOutAtBlock(iter->value_, var);
    }
    else
    {
        // get preceding statement to instr
        InstrNode* preInstr = instr->prev();
        liveOutAtInstr(preInstr, var);
    }
}

} // namespace me
