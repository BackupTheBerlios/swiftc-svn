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

#include "me/livenessanalysis.h"

#include <typeinfo>

namespace me {
    
//------------------------------------------------------------------------------

#ifdef SWIFT_USE_IG

#include <sstream>

struct IVar
{
    Var* var_;

    IVar() {}
    IVar(Var* var)
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

typedef Graph<IVar>::Node VarNode;

#endif // SWIFT_USE_IG

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

LivenessAnalysis::LivenessAnalysis(Function* function)
    : CodePass(function)
#ifdef SWIFT_USE_IG
    , ig_( new IGraph(*function->id_) )
#endif // SWIFT_USE_IG
{}

#ifdef SWIFT_USE_IG

LivenessAnalysis::~LivenessAnalysis()
{
    delete ig_;
}

#endif // SWIFT_USE_IG

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

#ifdef SWIFT_USE_IG
    /*
     * create nodes for the inteference graph
     */
    REGMAP_EACH(iter, function_->vars_)
    {
        Var* var = iter->second;

        VarNode* varNode = ig_->insert( new IVar(var) );
        var->varNode_ = varNode;
    }
#endif // SWIFT_USE_IG

    /*
     * start here
     */

    // for each var
    VARMAP_EACH(iter, function_->vars_)
    {
        Var* var = iter->second;

        // for each use of var
        DEFUSELIST_EACH(iter, var->uses_)
        {
            DefUse& use = iter->value_;
            InstrBase* instr = use.instrNode_->value_;

            if ( typeid(*instr) == typeid(PhiInstr) )
            {
                PhiInstr* phi = (PhiInstr*) instr;

                /*
                 * find the predecessor basic block
                 * NOTE in the case of a double entry there may be more than just one predecessor
                 * basic block 
                 */
                for (size_t i = 0; i < phi->arg_.size(); ++i)
                {
                    if ( phi->arg_[i].op_ == var )
                    {
                        // examine the found block
                        BBNode* pred = phi->sourceBBs_[i];
                        liveOutAtBlock(pred, var);
                    }
                }
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

#ifdef SWIFT_USE_IG
    ig_->dumpDot( ig_->name() );
#endif // SWIFT_USE_IG

    function_->firstLiveness_ = true;
}

void LivenessAnalysis::liveOutAtBlock(BBNode* bbNode, Var* var)
{
    BasicBlock* bb = bbNode->value_;

    // var is live-out at bb 
    bb->liveOut_.insert(var);

    // if bb not in walked_
    if ( !walked_.contains(bbNode) )
    {
        walked_.insert(bbNode);
        liveOutAtInstr(bb->end_->prev(), var);
    }
}

void LivenessAnalysis::liveOutAtInstr(InstrNode* instrNode, Var* var)
{
    InstrBase* instr = instrNode->value_;

    // var is live-out at instr
    instr->liveOut_.insert(var);

    // knows whether var is defined in instr
    bool varInLhs = false;

    for (size_t i = 0; i < instr->res_.size(); ++i)
    {
        if ( instr->res_[i].var_ != var )
        {
#ifdef SWIFT_USE_IG
            Var* res = instr->res_[i].var_;

            // add (v, w) to interference graph if it does not already exist
            if (   res->varNode_->succ_.find(var->varNode_) == res->varNode_->succ_.sentinel()
                && var->varNode_->succ_.find(res->varNode_) == var->varNode_->succ_.sentinel() )
            {
                var->varNode_->link(res->varNode_);
            }
#endif // SWIFT_USE_IG
        }
        else
            varInLhs = true;
    }

    if (!varInLhs)
        liveInAtInstr(instrNode, var);
}

void LivenessAnalysis::liveInAtInstr(InstrNode* instr, Var* var)
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
