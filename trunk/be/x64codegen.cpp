#include "be/x64codegen.h"

#include <iostream>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"

#include "be/x64parser.h"
#include "be/x64codegenhelpers.h"

//------------------------------------------------------------------------------

/*
 * globals
 */

enum Location
{
    INSTRUCTION,
    TYPE,
    OP1,
    OP2,
    END
};

me::InstrNode* currentInstrNode;
Location location;
std::ofstream* x64_ofs = 0;
int lastOp;

//------------------------------------------------------------------------------

namespace be {

/*
 * constructor
 */

X64CodeGen::X64CodeGen(me::Function* function, std::ofstream& ofs)
    : CodeGen(function, ofs)
{
    x64_ofs = &ofs;
}

/*
 * further methods
 */

void X64CodeGen::process()
{
    static int counter = 1;

    ofs_ << "\t.p2align 4,,15\n";
    ofs_ << ".globl swift_" << counter << '\n';
    ofs_ << "\t.type\tswift_" << counter << ", @function\n";
    ofs_ << "swift_" << counter << ":\n";
    ofs_ << ".LFB" << counter << ":\n";

    //ofs_ << '\n';
    //ofs_ << *function_->id_ << ":\n";
    int localStackSize = (function_->spillSlots_ + 1) * 16;
    ofs_ << "\tenter\t$0, $" << localStackSize << '\n';

    me::BBNode* currentNode = 0;
    bool phisInserted = false;

    INSTRLIST_EACH(iter, cfg_->instrList_)
    {
        me::InstrBase* instr = iter->value_;

        // check whether we have a new basic block
        if ( typeid(*instr) == typeid(me::LabelInstr) )
        {
            me::BBNode* oldNode = currentNode;
            currentNode = cfg_->labelNode2BBNode_[iter];

            if (!phisInserted)
                genPhiInstr(oldNode, currentNode);

            currentNode = cfg_->labelNode2BBNode_[iter];
            phisInserted = false;
        }
        else if ( dynamic_cast<me::JumpInstr*>(instr) )
        {
            // generate phi stuff already here
            swiftAssert( typeid(*iter->next()->value_) == typeid(me::LabelInstr),
                    "must be a LabelInstr here");

            // there must be exactly one successor
            if ( currentNode->succ_.size() == 1 )
            {
                genPhiInstr( currentNode, currentNode->succ_.first()->value_ );
                phisInserted = true;
            }
        }
        else if ( typeid(*instr) == typeid(me::PhiInstr) )
            continue;
        else if ( typeid(*instr) == typeid(me::SetParams) )
        {
            // TODO
            continue;
        }
        else if ( typeid(*instr) == typeid(me::SetResults) )
        {
            // TODO
            continue;
        }

        // update globals for x64lex
        currentInstrNode = iter;
        location = INSTRUCTION;

        x64parse();
    }

    if (localStackSize != 0)
        ofs_ << "\taddq\t$" << localStackSize << ", %rsp\n";

    ofs_ << "\tleave\n";
    ofs_ << "\tret\n";

    ofs_ << ".LFE" << counter << ":\n";
    ofs_ << "\t.size\t" << "swift_" << counter << ", .-" << "swift_" << counter << '\n';

    counter++;
    ofs_ << '\n';
}

// helpers

struct TReg
{
    int color_;
    me::Op::Type type_;

    TReg(int color, me::Op::Type type)
        : color_(color)
        , type_(type)
    {}

    std::string toString() const
    {
        std::ostringstream oss;
        oss << "color: " << color_ << " type: " << type_;
        return oss.str();
    }
};

class RegGraph : public Graph<TReg>
{
    virtual std::string name() const
    {
        return "";
    }
};

typedef RegGraph::Node* RGNode;

int meType2beType(me::Op::Type type)
{
    switch (type)
    {
        case me::Op::R_BOOL:  return X64_BOOL;

        case me::Op::R_INT8:  return X64_INT8;
        case me::Op::R_INT16: return X64_INT16;
        case me::Op::R_INT32: return X64_INT32;
        case me::Op::R_INT64: return X64_INT64;

        case me::Op::R_SAT8:  return X64_SAT8;
        case me::Op::R_SAT16: return X64_SAT16;

        case me::Op::R_UINT8:  return X64_UINT8;
        case me::Op::R_UINT16: return X64_UINT16;
        case me::Op::R_UINT32: return X64_UINT32;
        case me::Op::R_UINT64: return X64_UINT64;

        case me::Op::R_USAT8:  return X64_USAT8;
        case me::Op::R_USAT16: return X64_USAT16;

        case me::Op::R_REAL32: return X64_REAL32;
        case me::Op::R_REAL64: return X64_REAL64;

        default:
            swiftAssert(false, "unreachable code");
    }

    return -1;
}


void X64CodeGen::genMove(me::Op::Type type, int r1, int r2)
{
    me::Reg op1(type, -1);
    me::Reg op2(type, -1);
    op1.color_ = r1;
    op2.color_ = r2;

    ofs_  << "\tmov" << suffix(meType2beType(type)) << '\t' << reg2str(&op1) << ", " << reg2str(&op2) << '\n';
}

void X64CodeGen::genPhiInstr(me::BBNode* prevNode, me::BBNode* nextNode)
{
    me::BasicBlock* nextBB = nextNode->value_;

    if ( !nextBB->hasPhiInstr() )
        return;

#ifdef SWIFT_DEBUG
    ofs_ << "\t /* phi functions */\n";
#endif // SWIFT_DEBUG

    /* 
     * Because of the critical edge elimination prevNode's 
     * successors can't have phi functions when prevNode has more than
     * one successor.
     */
    swiftAssert( prevNode->succ_.size() == 1, "must have exactly one successor");

    size_t phiIndex = nextNode->whichPred(prevNode);

    RegGraph rg;
    std::map<int, RegGraph::Node*> inserted;

    // for each phi function in nextBB
    for (me::InstrNode* iter = nextBB->firstPhi_; iter != nextBB->firstOrdinary_; iter = iter->next())
    {
        swiftAssert( typeid(*iter->value_) == typeid(me::PhiInstr), 
                "must be a PhiInstr here" );
        me::PhiInstr* phi = (me::PhiInstr*) iter->value_;

        // get regs
        me::Reg* srcReg = ((me::Reg*) phi->arg_[phiIndex].op_);
        me::Reg* dstReg = phi->result();

        if ( dstReg->isMem() )
            continue; // TODO

        // is this a pointless definition? (should be optimized away)
        if ( !phi->liveOut_.contains(dstReg) )
            continue;

        // is this a move of the dummy value UNDEF?
        me::InstrNode* def = srcReg->def_.instrNode_;
        if ( typeid(*def->value_) == typeid(me::AssignInstr) )
        {
            me::AssignInstr* ai = (me::AssignInstr*) def->value_;
            if (ai->kind_ == '=')
            {
                if ( typeid(*ai->arg_[0].op_) == typeid(me::Undef) )
                    continue; // yes -> so ignore this phi function
            }
        }

        // get colors
        int srcColor = srcReg->color_;
        int dstColor = dstReg->color_;

        // get Types
        me::Op::Type srcType = srcReg->type_;
        me::Op::Type dstType = dstReg->type_;

        /* 
         * check whether these colors have already been inserted 
         * and insert if applicable
         */
        std::map<int, RegGraph::Node*>::iterator srcIter = inserted.find(srcColor);
        if ( srcIter == inserted.end() )
            srcIter = inserted.insert( std::make_pair(srcColor, rg.insert(new TReg(srcColor, srcType))) ).first;

        std::map<int, RegGraph::Node*>::iterator dstIter = inserted.find(dstColor);
        if ( dstIter == inserted.end() )
            dstIter = inserted.insert( std::make_pair(dstColor, rg.insert(new TReg(dstColor, dstType))) ).first;

        srcIter->second->link(dstIter->second);
        //std::cout << srcIter->second->value_->toString() << " -> " << dstIter->second->value_->toString() << std::endl;
    }

    /*
     * while there is an edge e = (r, s) with r != s 
     * where the outdegree of s equals 0 ...
     */
    while (true)
    {
        RegGraph::Relative* iter = rg.nodes_.first();
        while ( iter != rg.nodes_.sentinel() && !iter->value_->succ_.empty() )
            iter = iter->next();

        if ( iter != rg.nodes_.sentinel() )
        {
            RegGraph::Node* n = iter->value_;

            // ... resolve this by a move p -> n
            swiftAssert( n->pred_.size() == 1, 
                    "must have exactly one predecessor" );
            RegGraph::Node* p = n->pred_.first()->value_;

            me::Op::Type type = n->value_->type_;
            genMove(type, p->value_->color_, n->value_->color_);

            rg.erase(n);

            // if p doesn't have a predecessor erase it, too
            if ( p->pred_.empty() )
                rg.erase(p);
        }
        else
            break;
    }

    /*
     * now remove cycles
     */

    // while there are still nodes left
    while ( !rg.nodes_.empty() )
    {
        // take first node
        RegGraph::Relative* relative = rg.nodes_.first();
        RegGraph::Node* node = relative->value_;

        /*
         * remove trivial self loops 
         * without generating any instructions
         */
        if (node->succ_.first()->value_ == node)
        {
            rg.erase(node);
            continue;
        }
            
        /*
         * -> we have a non-trivial cycle:
         * R1 -> R2 -> R3 -> ... -> Rn -> r1
         *
         * -> generate these moves:
         *  mov r1, free
         *  mov r2, r1
         *  mov r3, r2
         *  ...
         *  mov rn, free
         */

        // mov r1, free
        me::Op::Type type = node->value_->type_;

        if (type == me::Op::R_REAL32 || type == me::Op::R_REAL64)
            genMove(type, node->value_->color_, X64RegAlloc::XMM15); // TODO
        else
            genMove(type, node->value_->color_, X64RegAlloc::R15); // TODO

        std::vector<RegGraph::Node*> toBeErased;
        toBeErased.push_back(node);

        // iterate over the cycle
        RegGraph::Node* predIter = node; // current node
        RegGraph::Node* cylcleIter = node->succ_.first()->value_; // start with next node
        while (cylcleIter != node) // until we reach r1 again
        {
            // mov r_current, r_pred
            genMove(type, cylcleIter->value_->color_, predIter->value_->color_);

            // remember for erasion
            toBeErased.push_back(cylcleIter);

            // go to next node
            predIter = predIter->succ_.first()->value_;
            cylcleIter = cylcleIter->succ_.first()->value_;
        }

        // mov free, rn
        if (type == me::Op::R_REAL32 || type == me::Op::R_REAL64)
            genMove(type, X64RegAlloc::XMM15, predIter->value_->color_); // TODO
        else
            genMove(type, X64RegAlloc::R15, predIter->value_->color_); // TODO

        // remove all handled nodes
        for (size_t i = 0; i < toBeErased.size(); ++i)
            rg.erase( toBeErased[i] );
    }

#ifdef SWIFT_DEBUG
    ofs_ << "\t /* end phi functions */\n";
#endif // SWIFT_DEBUG
}

} // namespace be

//------------------------------------------------------------------------------

/*
 * lexer to parser interface
 */

void x64error(const char *s)
{
    printf( "%s: could not parse '%s'\n", s, currentInstrNode->value_->toString().c_str() );
}

int x64lex()
{
    me::InstrBase* currentInstr = currentInstrNode->value_;

    switch (location)
    {
        case INSTRUCTION:
        {
            // set new location
            location = TYPE;

            const std::type_info& instrTypeId = typeid(*currentInstr);

            if ( instrTypeId == typeid(me::LabelInstr) )
            {
                x64lval.label_ = (me::LabelInstr*) currentInstr;
                location = END;
                return X64_LABEL;
            }
            else if (instrTypeId == typeid(me::GotoInstr) )
            {
                x64lval.goto_ = (me::GotoInstr*) currentInstr;
                return X64_GOTO;
            }
            else if (instrTypeId == typeid(me::BranchInstr) )
            {
                me::BranchInstr* bi = (me::BranchInstr*) currentInstr;
                x64lval.branch_ = (me::BranchInstr*) currentInstr;

                me::InstrNode* nextNode = currentInstrNode->next();
                swiftAssert( typeid(*nextNode->value_) == typeid(me::LabelInstr),
                        "must be a LabelInstr" );
                me::LabelInstr* nextLabel = (me::LabelInstr*) nextNode->value_;

                if (bi->trueLabel() == nextLabel)
                    return X64_BRANCH_FALSE;
                else if (bi->falseLabel() == nextLabel)
                    return X64_BRANCH_TRUE;
                else
                    return X64_BRANCH;
            }
            else if ( instrTypeId == typeid(me::AssignInstr) )
            {
                me::AssignInstr* ai = (me::AssignInstr*) currentInstr;
                x64lval.assign_ = ai;

                switch (ai->kind_)
                {
                    case '=': return X64_MOV;
                    case '+': return X64_ADD;
                    case '-': return X64_SUB;
                    case '*': return X64_MUL;
                    case '/': return X64_DIV;
                    case me::AssignInstr::EQ: return X64_EQ;
                    case me::AssignInstr::NE: return X64_NE;
                    case '<': return X64_L;
                    case '>': return X64_G;
                    case me::AssignInstr::LE: return X64_LE;
                    case me::AssignInstr::GE: return X64_GE;
                    default:
                        swiftAssert(false, "unreachable code");
                }
            }
            else if ( instrTypeId == typeid(me::Spill) )
            {
                me::Spill* spill = (me::Spill*) currentInstr;
                x64lval.spill_ = spill;
                return X64_SPILL;
            }
            else if ( instrTypeId == typeid(me::Reload) )
            {
                me::Reload* reload = (me::Reload*) currentInstr;
                x64lval.reload_ = reload;
                return X64_RELOAD;
            }
            else if ( instrTypeId == typeid(me::PhiInstr) )
            {
                swiftAssert(false, "unreachable code");
            }
            else
            {
                swiftAssert( instrTypeId == typeid(me::NOP), "must be a NOP" );
                location = END;
                return X64_NOP;
            }
        }
        case TYPE:
        {
            if ( currentInstr->arg_.empty() )
                return 0;

            // set new location
            location = OP1;
            me::Op::Type type =  currentInstr->arg_[0].op_->type_;
            switch (type)
            {
                case me::Op::R_SPECIAL:
                case me::Op::R_BOOL:  return X64_BOOL;

                case me::Op::R_INT8:  return X64_INT8;
                case me::Op::R_INT16: return X64_INT16;
                case me::Op::R_INT32: return X64_INT32;
                case me::Op::R_INT64: return X64_INT64;
                case me::Op::R_SAT8:  return X64_SAT8;
                case me::Op::R_SAT16: return X64_SAT16;

                case me::Op::R_UINT8:  return X64_UINT8;
                case me::Op::R_UINT16: return X64_UINT16;
                case me::Op::R_UINT32: return X64_UINT32;
                case me::Op::R_UINT64: return X64_UINT64;
                case me::Op::R_USAT8:  return X64_USAT8;
                case me::Op::R_USAT16: return X64_USAT16;

                case me::Op::R_REAL32: return X64_REAL32;
                case me::Op::R_REAL64: return X64_REAL64;

                case me::Op::R_PTR:    return X64_PTR;
                case me::Op::R_STACK:  return X64_STACK;
            }
        }
        case OP1:
        {
            if ( currentInstr->arg_.empty() )
                return 0;

            location = OP2;

            me::Op* op = currentInstr->arg_[0].op_;
            const std::type_info& opTypeId = typeid(*op);

            if ( opTypeId == typeid(me::Undef) )
            {
                x64lval.undef_ = (me::Undef*) op;
                lastOp = X64_UNDEF;
            }
            else if ( opTypeId == typeid(me::Const) )
            {
                x64lval.const_ = (me::Const*) op;
                lastOp = X64_CONST;
            }
            else 
            {
                swiftAssert( opTypeId == typeid(me::Reg), "must be a Reg" );
                me::Reg* reg = (me::Reg*) op;
                x64lval.reg_ = reg;

                if ( !currentInstr->res_.empty() && currentInstr->res_[0].reg_->isMem() )
                    return X64_REG_2;

                if ( !currentInstr->res_.empty() && currentInstr->res_[0].reg_->color_ == reg->color_ )
                    lastOp = X64_REG_1;
                else
                    lastOp = X64_REG_2;
            }
            return lastOp;
        }
        case OP2:
        {
            location = END;

            if ( currentInstr->arg_.size() < 2 )
                return 0;

            me::Op* op = currentInstr->arg_[1].op_;
            const std::type_info& opTypeId = typeid(*op);

            if ( opTypeId == typeid(me::Undef) )
            {
                x64lval.undef_ = (me::Undef*) op;
                lastOp = X64_UNDEF;
            }
            else if ( opTypeId == typeid(me::Const) )
            {
                x64lval.const_ = (me::Const*) op;
                lastOp = X64_CONST;
            }
            else 
            {
                swiftAssert( opTypeId == typeid(me::Reg), "must be a Reg here");
                me::Reg* reg = (me::Reg*) op;
                x64lval.reg_ = reg;

                /*
                 * The following cases should be considered:
                 * - res.color == op2.color                 -> reg1
                 * - op1 == reg, op1.color == op2.color_    -> reg2
                 * - op1 == reg1                            -> reg2
                 * - op1 == const                           -> reg2
                 * else -> reg3
                 */
                if ( currentInstr->res_[0].reg_->color_ == reg->color_ )
                    lastOp = X64_REG_1;
                else if ( lastOp == X64_REG_2 && ((me::Reg*) currentInstr->arg_[0].op_)->color_ == reg->color_ )
                    lastOp = X64_REG_2;
                else if (lastOp == X64_REG_1)
                    lastOp = X64_REG_2;
                else if (lastOp == X64_CONST)
                    lastOp = X64_REG_2;
                else
                    return X64_REG_3;
            }
            return lastOp;
        }
        case END:
            return 0;
    }

    return 0;
}
