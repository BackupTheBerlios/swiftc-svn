#include "codegenerator.h"

#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

//------------------------------------------------------------------------------

#ifdef SWIFT_DEBUG

#include <sstream>

struct IVar
{
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

struct IGraph : public Graph<IVar>
{
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
    spill();
    livenessAnalysis();
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
                PhiInstr* phi = (PhiInstr*) instr;

                // find the predecessor basic block
                size_t i = 0;
                std::cout << phi->numRhs_ << std::endl;
                while (phi->rhs_[i] != var)
                    ++i;

                swiftAssert(i < phi->numRhs_, "i to large here");
                BBNode* pred = phi->sourceBBs_[i];

                // examine the found block
                liveOutAtBlock(pred, var);
            }
            else
                liveInAtInstr(use.instr_, var);
        }

        // clean up
        walked_.clear();
    }
}

void CodeGenerator::liveOutAtBlock(BBNode* bbNode, PseudoReg* var)
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

void CodeGenerator::liveInAtInstr(InstrNode instr, PseudoReg* var)
{
    // var ist live-in at instr
    instr->value_->liveIn_.insert(var);
    InstrNode prevInstr = instr->prev();

    // is instr the first statement of basic block?
    if ( typeid(*prevInstr->value_) == typeid(LabelInstr) )
    {
        // var is live-out at the leading labelInstr
        prevInstr->value_->liveOut_.insert(var);

        BBNode* bb = function_->cfg_.labelNode2BBNode_[prevInstr];
        bb->value_->liveIn_.insert(var);

        // for each predecessor of bb
        CFG_RELATIVES_EACH(iter, bb->pred_)
            liveOutAtBlock(iter->value_, var);
    }
    else
    {
        // get preceding statement to instr
        InstrNode preInstr = instr->prev();
        liveOutAtInstr(preInstr, var);
    }
}

void CodeGenerator::liveOutAtInstr(InstrNode instr, PseudoReg* var)
{
    // var is live-out at instr
    instr->value_->liveOut_.insert(var);

    AssignmentBase* ab = dynamic_cast<AssignmentBase*>(instr->value_);
    // for each reg v, that ab defines
    if (ab)
    {
        for (size_t i = 0; i < ab->numLhs_; ++i)
        {
            if ( ab->lhs_[i] != var )
            {
                // add (v, w) to interference graph
                var->varNode_->link(ab->lhs_[i]->varNode_);
                liveInAtInstr(instr, var);
            }
        }
    }
    else // -> var != result
        liveInAtInstr(instr, var);
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
        and perform a pre-order walk of the dominator tree
    */
    colorRecursive( cfg_->entry_->succ_.first()->value_ );
}

void CodeGenerator::colorRecursive(BBNode* bb)
{
    std::cout << "LIVE-IN: " << bb->value_->name() << std::endl;
    Colors colors;
    REGSET_EACH(iter, bb->value_->liveIn_)
    {
        int color = (*iter)->color_;
        swiftAssert(color != -1, "color must be assigned here");
        colors.insert(color);
        std::cout << "\t" << (*iter)->toString() << ": " << color << std::endl;
    }

    // for each instruction -> start with the first instruction which is followed by the leading LabelInstr
    for (InstrNode iter = bb->value_->begin_->next(); iter != bb->value_->end_; iter = iter->next())
    {
        AssignmentBase* ab = dynamic_cast<AssignmentBase*>(iter->value_);

        if (ab)
        {
            /*
                NOTE for the InstrBase::isLastUse(InstrNode instrNode, PseudoReg* var) used below:
                    instrNode has an predecessor in all cases because iter is initialized with the
                    first instructin which is followed by the leading LabelInstr of this basic
                    block. Thus the first instruction which is considered here will always be
                    preceded by a LabelInstr.

                    Furthermore Literals are no problems here since they are not found via
                    isLastUse.
            */

            // for each var on the right hand side
            for (size_t i = 0; i < ab->numRhs_; ++i)
            {
                PseudoReg* reg = ab->rhs_[i];

                if ( InstrBase::isLastUse(iter, reg) )
                {
                    // -> its the last use of op1
                    Colors::iterator colorIter = colors.find(reg->color_);
                    swiftAssert( colorIter != colors.end(), "color must be found here");
                    colors.erase(colorIter); // last use of op1 so remove
                    /*
                        use "break" here in order to prevent confusionwhich can be
                        caused by double entries with instructions like a = b + b
                    */
                    break;
                }
            }

            // for each var on the left hand side -> assign a color for result
            for (size_t i = 0; i < ab->numLhs_; ++i)
            {
                PseudoReg* reg = ab->lhs_[i];
                reg->color_ = findFirstFreeColorAndAllocate(colors);

                if ( reg->uses_.empty() )
                {
                    /*
                        In the case of a pointless definition a color should be
                        assigned and immediately released afterwards.
                        A pointless definition is for example
                        a = b + c
                        an never use 'a' again. The register allocator would assign a color for 'a'
                        and will never release it since there will be no known last use of 'a'.
                    */
                    Colors::iterator colorIter = colors.find(reg->color_);
                    swiftAssert( colorIter != colors.end(), "color must be found here");
                    colors.erase(colorIter); // last use of op2 so remove
                }
            }
        }
    } // for each instruction

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
