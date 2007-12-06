#include "codegenerator.h"

#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

//------------------------------------------------------------------------------

#ifdef SWIFT_DEBUG

#include <sstream>

struct IVar {
    PseudoReg* var_;

    IVar() {}
    IVar(PseudoReg* var)
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

struct IGraph : public Graph<IVar> {
    IGraph(const std::string& name)
        : name_(name)
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

typedef Graph<IVar>::Node VarNode;

#endif // SWIFT_DEBUG

//------------------------------------------------------------------------------

/*
    init statics
*/

Spiller* CodeGenerator::spiller_ = 0;

/*
    constructor and destructor
*/

CodeGenerator::CodeGenerator(std::ofstream& ofs, Function* function)
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
    methods
*/

void CodeGenerator::genCode()
{
    std::cout << *function_->id_ << std::endl;
    livenessAnalysis();
    spill();
    color();
    ig_->dumpDot( ig_->name() );
    coalesce();
}

/*
    liveness stuff
*/

void CodeGenerator::livenessAnalysis()
{
#ifdef SWIFT_DEBUG
    // create var nodes
    REGMAP_EACH(iter, function_->vars_)
    {
        PseudoReg* var = iter->second;

        VarNode* varNode = ig_->insert( new IVar(var) );
        var->varNode_ = varNode;
    }
#endif // SWIFT_DEBUG

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
    bb->liveOut_.insert(var);

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
    instr->value_->liveIn_.insert(var);

    // is instr the first statement of basic block?
    if ( typeid(*instr->value_) == typeid(LabelInstr) )
    {
        BBNode* bb = function_->cfg_.labelNode2BBNode_[instr];
        if (bb)
            bb->value_->liveIn_.insert(var);

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
    instr->value_->liveOut_.insert(var);

    // for each reg v, that instr defines
    if ( typeid(*instr->value_) == typeid(AssignInstr) )
    {
        AssignInstr* ai = (AssignInstr*) instr->value_;

        if ( ai->result_ != var )
        {
            // add (v, w) to interference graph
            var->varNode_->link(ai->result_->varNode_);
            liveInAtInstr(instr, var);
        }
    }
    // for each reg v, that instr defines
    else if ( typeid(*instr->value_) == typeid(PhiInstr) )
    {
        PhiInstr* phi = (PhiInstr*) instr->value_;

        if ( phi->result_ != var )
        {
            var->varNode_->link(phi->result_->varNode_);
            // add (v, w) to interference graph
            liveInAtInstr(instr, var);
        }
    }
}

/*
    coloring
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
    Colors colors;
    REGSET_EACH(iter, bb->value_->liveIn_)
    {
        int color = (*iter)->color_;
        swiftAssert(color != -1, "color must be assigned here");
        colors.insert(color);
    }

    // for each instruction -> start with the first instruction which is followed by the leading LabelInstr
    for (InstrNode iter = bb->value_->begin_->next(); iter != bb->value_->end_; iter = iter->next())
    {
        InstrBase* instr = iter->value_;

        // for each var on the right hand side
        if ( typeid(*instr) == typeid(AssignInstr) )
        {
            AssignInstr* ai = (AssignInstr*) instr;
            // for each var on the right hand side
            if ( ai->liveOut_.find(ai->op1_) != ai->liveOut_.end() )
            {
                // -> its the last use of op1
                Colors::iterator iter = colors.find(ai->op1_->color_);
                swiftAssert( iter != colors.end(), "colors must be found here");
                colors.erase(iter); // last use of op1 so remove
            }
            if ( ai->op2_ && ai->liveOut_.find(ai->op2_) != ai->liveOut_.end() )
            {
                // -> its the last use of op2
                Colors::iterator iter = colors.find(ai->op2_->color_);
                swiftAssert( iter != colors.end(), "colors must be found here");
                colors.erase(iter); // last use of op2 so remove
            }
            // for each var on the left hand side
            // -> assign a color for result
            ai->result_->color_ = findFirstFreeColorAndAllocate(colors);
            std::cout << ai->result_->color_ << std::endl;
        }

        // for each var on the right hand side
        if ( typeid(*instr) == typeid(PhiInstr) )
        {
            PhiInstr* phi = (PhiInstr*) instr;
            // for each var on the right hand side
            for (size_t i = 0; i < phi->argc_; ++i)
            {
                if ( phi->liveOut_.find(phi->args_[i]) != phi->liveOut_.end() )
                {
                    // -> its the last use of args_[i]
                    Colors::iterator iter = colors.find(phi->args_[i]->color_);
                    swiftAssert( iter != colors.end(), "colors must be found here");
                    colors.erase(iter); // last use of op1 so remove
                }
            }
            // for each var on the left hand side
            // -> assign a color for result
            phi->result_->color_ = findFirstFreeColorAndAllocate(colors);
            std::cout << phi->result_->color_ << std::endl;
        }

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
