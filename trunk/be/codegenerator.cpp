#include "be/codegenerator.h"

#include <limits>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

namespace be {

enum {
    MAX = std::numerical_limits<int>::max()
}

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
    init statics
*/

Spiller* CodeGenerator::spiller_ = 0;

/*
    constructor and destructor
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
    methods
*/

void CodeGenerator::genCode()
{
    // init spiller
    spiller_->setFunction(function_);

    // traverse the code generation pipe
//     std::cout << std::endl << *function_->id_ << std::endl;
    livenessAnalysis();
    spill();
    color();
#ifdef SWIFT_DEBUG
    ig_->dumpDot( ig_->name() );
#endif // SWIFT_DEBUG
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
                me::BBNode pred = phi->sourceBBs_[i];

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

void CodeGenerator::liveOutAtBlock(me::BBNode bbNode, me::Reg* var)
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

void CodeGenerator::liveInAtInstr(me::InstrNode instr, me::Reg* var)
{
    // var ist live-in at instr
    instr->value_->liveIn_.insert(var);
    me::InstrNode prevInstr = instr->prev();

    // is instr the first statement of basic block?
    if ( typeid(*prevInstr->value_) == typeid(me::LabelInstr) )
    {
        // var is live-out at the leading labelInstr
        prevInstr->value_->liveOut_.insert(var);

        me::BBNode bb = function_->cfg_.labelNode2BBNode_[prevInstr];
        bb->value_->liveIn_.insert(var);

        // for each predecessor of bb
        CFG_RELATIVES_EACH(iter, bb->pred_)
            liveOutAtBlock(iter->value_, var);
    }
    else
    {
        // get preceding statement to instr
        me::InstrNode preInstr = instr->prev();
        liveOutAtInstr(preInstr, var);
    }
}

void CodeGenerator::liveOutAtInstr(me::InstrNode instr, me::Reg* var)
{
    // var is live-out at instr
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

/*
    spilling
*/

void CodeGenerator::spill()
{
    // TODO
//     spiller_->spill();


}

int distance(me::Reg* reg, me::InstrNode instrNode) {
    InstrNode instr = instrNode->value_;

    // is reg not live in instr?
    if ( instr->liveOut_.find(reg) == instr->liveOut_.end() )
        return 0;
    // else


}

int distanceRec(me::Reg* reg, me::InstrNode instrNode) {
    InstrNode instr = instrNode->value_;

    // is reg not live in instr?
    if ( instr->liveOut_.find(reg) == instr->liveOut_.end() )
        return MAX;
    // else

    // do we have an ordinary predecessor instruction?
    if (instrNode->pred() != cfg_->instrList_.sentinel()
        && typeid(*instr) != typeid(PhiInstr)
    {
        int result = distanceRec(reg, instrNode->pred());
        // add up the distance and do not calculate around
        return result == MAX ? MAX : result + 1;
    }
    // else

    swiftAssert( typeid(*instr) == typeid(LabelInstr) );
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

void CodeGenerator::color()
{
    /*
        start with the first true basic block
        and perform a pre-order walk of the dominator tree
    */
    colorRecursive( cfg_->entry_->succ_.first()->value_ );
}

void CodeGenerator::colorRecursive(me::BBNode bb)
{
    Colors colors;
    REGSET_EACH(iter, bb->value_->liveIn_)
    {
        int color = (*iter)->color_;
        swiftAssert(color != -1, "color must be assigned here");
        colors.insert(color);
    }

    // for each instruction -> start with the first instruction which is followed by the leading me::LabelInstr
    for (me::InstrNode iter = bb->value_->begin_->next(); iter != bb->value_->end_; iter = iter->next())
    {
        me::AssignmentBase* ab = dynamic_cast<me::AssignmentBase*>(iter->value_);

        if (ab)
        {
            /*
                NOTE for the me::InstrBase::isLastUse(me::InstrNode instrNode, Reg* var) used below:
                    instrNode has an predecessor in all cases because iter is initialized with the
                    first instructin which is followed by the leading me::LabelInstr of this basic
                    block. Thus the first instruction which is considered here will always be
                    preceded by a me::LabelInstr.

                    Furthermore Literals are no problems here since they are not found via
                    isLastUse.
            */

#ifdef SWIFT_DEBUG
            /*
                In the debug version this set knows vars which were already
                removed. This allows more precise assertions (see below).
            */
            me::RegSet erased;
#endif // SWIFT_DEBUG

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
                    if ( colorIter == colors.end() )
                        colors.erase(colorIter); // last use of reg
                    /*
                        else -> the reg must already been removed which must
                            be caused by a double entry like a = b + b
                    */
#endif // SWIFT_DEBUG
                }
            }

            // for each var on the left hand side -> assign a color for result
            for (size_t i = 0; i < ab->numLhs_; ++i)
            {
                me::Reg* reg = ab->lhs_[i];
                reg->color_ = findFirstFreeColorAndAllocate(colors);

                if ( reg->uses_.empty() )
                {
                    /*
                        In the case of a pointless definition a color should be
                        assigned and immediately released afterwards.
                        A pointless definition is for example
                        a = b + c
                        and never use 'a' again. The register allocator would assign a color for 'a'
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
    for (me::BBList::Node* iter = bb->value_->domChildren_.first(); iter != bb->value_->domChildren_.sentinel(); iter = iter->next())
    {
        me::BBNode domChild = iter->value_;

        // omit special exit node
        if ( domChild->value_->isExit() )
            continue;

        colorRecursive(domChild);
    }
}

void CodeGenerator::coalesce()
{
}

} // namespace be
