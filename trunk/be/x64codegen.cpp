
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

#include "be/x64codegen.h"

#include <iostream>
#include <typeinfo>

#include "me/cfg.h"
#include "me/functab.h"
#include "me/stacklayout.h"

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
me::StackLayout* x64_stacklayout = 0;
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
    x64_stacklayout = function_->stackLayout_;
}

/*
 * further methods
 */

void X64CodeGen::process()
{
    static int counter = 1;
    std::string id;

    if ( function_->isMain() )
        id = "main";
    else
        id = *function_->id_;

    function_->stackLayout_->arangeStackLayout();

    // function prologue
    ofs_ //<< "\t.p2align 4,,15\n"
         << "\t.globl\t" << id << '\n'
         << "\t.type\t" << id << ", @function\n"
         << id << ":\n"
         << ".LFB" << counter << ":\n";

    const me::Colors& usedColors = function_->usedColors_;

    // count number of pushes
    int numPushes = 0;
    if ( usedColors.contains(X64RegAlloc::RBX) ) ++numPushes;
    if ( usedColors.contains(X64RegAlloc::R12) ) ++numPushes;
    if ( usedColors.contains(X64RegAlloc::R13) ) ++numPushes;
    if ( usedColors.contains(X64RegAlloc::R14) ) ++numPushes;
    if ( usedColors.contains(X64RegAlloc::R15) ) ++numPushes;

    // align stack
    numPushes = (numPushes & 0x00000001) ? 8 : 0; // 8 if odd, 0 if even
    int stackSize = function_->stackLayout_->size_;
    if ( (stackSize + numPushes) & 0x0000000F ) // (stackSize + numPushes) % 16
        stackSize = ((stackSize  + numPushes + 15) & 0xFFFFFFF0) - numPushes;

    ofs_ << "\tpushq\t%rbp\n"
         << "\tmovq\t%rsp, %rbp\n";

    if (stackSize)
        ofs_ << "\tsubq\t$" << stackSize << ", %rsp\n";

    // save registers which should be preserved during function calls
    if ( usedColors.contains(X64RegAlloc::RBX) ) ofs_ << "\tpushq\t%rbx\n";
    if ( usedColors.contains(X64RegAlloc::R12) ) ofs_ << "\tpushq\t%r12\n";
    if ( usedColors.contains(X64RegAlloc::R13) ) ofs_ << "\tpushq\t%r13\n";
    if ( usedColors.contains(X64RegAlloc::R14) ) ofs_ << "\tpushq\t%r14\n";
    if ( usedColors.contains(X64RegAlloc::R15) ) ofs_ << "\tpushq\t%r15\n";

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
        else if ( typeid(*instr) == typeid(me::AssignInstr) )
        {
            me::AssignInstr* ai = (me::AssignInstr*) instr;

            if (ai->kind_ == '=')
            {
                if ( typeid(*ai->arg_[0].op_) == typeid(me::Undef) )
                    continue; // ignore defition of undefined vars
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

    /*
     * function epilogue
     */

    // restore saved registers
    if ( usedColors.contains(X64RegAlloc::R15) ) ofs_ << "\tpopq\t%r15\n";
    if ( usedColors.contains(X64RegAlloc::R14) ) ofs_ << "\tpopq\t%r14\n";
    if ( usedColors.contains(X64RegAlloc::R13) ) ofs_ << "\tpopq\t%r13\n";
    if ( usedColors.contains(X64RegAlloc::R12) ) ofs_ << "\tpopq\t%r12\n";
    if ( usedColors.contains(X64RegAlloc::RBX) ) ofs_ << "\tpopq\t%rbx\n";

    // clean up
    ofs_ << "\tmovq\t%rbp, %rsp\n"
         << "\tpopq\t%rbp\n"
         << "\tret\n";

    ofs_ << ".LFE" << counter << ":\n"
         << "\t.size\t" << id << ", .-" << id << '\n';

    ++counter;
    ofs_ << '\n';
}

/*
 * helpers
 */

class RegGraph : public Graph<int>
{
    virtual std::string name() const
    {
        return "";
    }
};

struct Link
{
    me::Op::Type type_;
    bool spilled_;

    Link() {}
    Link(me::Op::Type type, bool spilled)
        : type_(type)
        , spilled_(spilled)
    {}
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
        case me::Op::R_PTR:
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
    ofs_ << "\tmov" << suffix(meType2beType(type)) << '\t' 
         << reg2str(r1, type) << ", " << reg2str(r2, type) << '\n';
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

    /*
     * collect free registers
     */

    me::Colors xmmFree = *X64RegAlloc::getXmmColors();;
    me::Colors intFree = *X64RegAlloc::getIntColors();

    /* 
     * erase all non-spilled regs which are in the live-out 
     * of the last instruction of prevNode
     */
    VARSET_EACH(iter, prevNode->value_->end_->prev_->value_->liveOut_) 
    {
        if ( (*iter)->type_ == me::Op::R_MEM || (*iter)->isSpilled() )
            continue; // ignore stack vars

        if ( (*iter)->isReal() )
        {
            swiftAssert( xmmFree.contains((*iter)->color_), 
                        "colors must be found here" );
            xmmFree.erase( (*iter)->color_ );
        }
        else
        {
            swiftAssert( intFree.contains((*iter)->color_),
                        "colors must be found here" );
            intFree.erase( (*iter)->color_ );
        }
    }

    const me::Colors& usedColors = function_->usedColors_;

    // use these ones only if they are used anyway
    if ( !usedColors.contains(X64RegAlloc::RBX) ) intFree.erase(X64RegAlloc::RBX);
    if ( !usedColors.contains(X64RegAlloc::R12) ) intFree.erase(X64RegAlloc::R12);
    if ( !usedColors.contains(X64RegAlloc::R13) ) intFree.erase(X64RegAlloc::R13);
    if ( !usedColors.contains(X64RegAlloc::R14) ) intFree.erase(X64RegAlloc::R14);
    if ( !usedColors.contains(X64RegAlloc::R15) ) intFree.erase(X64RegAlloc::R15);

    RegGraph rg;
    std::map<int, RegGraph::Node*> inserted;
    typedef std::map< int, std::map<int, Link> > LinkTypes;
    LinkTypes linkTypes;

    // for each phi function in nextBB
    for (me::InstrNode* iter = nextBB->firstPhi_; iter != nextBB->firstOrdinary_; iter = iter->next())
    {
        bool spilled = false;

        swiftAssert( typeid(*iter->value_) == typeid(me::PhiInstr), 
                "must be a PhiInstr here" );
        me::PhiInstr* phi = (me::PhiInstr*) iter->value_;

        // ignore phi functions of MemVars
        if ( typeid(*phi->res_[0].var_) != typeid(me::Reg) )
            continue;

        // get regs
        me::Reg* srcReg = (me::Reg*) phi->arg_[phiIndex].op_;
        me::Reg* dstReg = (me::Reg*) phi->res_[0].var_;

        if ( dstReg->isSpilled() )
        {
            swiftAssert(srcReg->isSpilled(), "must be spilled, too");
            spilled = true;
            std::cout << "TODO" << std::endl;
        }

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

        // get Type
        me::Op::Type type = dstReg->type_;
        swiftAssert(srcReg->type_ == type, "types must match");

        /* 
         * check whether these colors have already been inserted 
         * and insert if applicable
         */
        std::map<int, RegGraph::Node*>::iterator srcIter = inserted.find(srcColor);
        if ( srcIter == inserted.end() )
        {
            srcIter = inserted.insert( std::make_pair(
                        srcColor, rg.insert(new int(srcColor))) ).first;
        }

        std::map<int, RegGraph::Node*>::iterator dstIter = inserted.find(dstColor);
        if ( dstIter == inserted.end() )
        {
            dstIter = inserted.insert( std::make_pair(
                        dstColor, rg.insert(new int(dstColor))) ).first;
        }

        srcIter->second->link(dstIter->second);
        linkTypes[srcColor][dstColor] = Link(type, spilled);
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

            int p_color = *p->value_; // save color
            int n_color = *n->value_; // save color
            me::Op::Type type = linkTypes[p_color][n_color].type_;
            genMove(type, p_color, n_color);

            // p is free now while n is used now
            if ( me::Op::isReal(type) )
            {
                xmmFree.insert(p_color);
                xmmFree.erase (n_color);
            }
            else
            {
                intFree.insert(p_color);
                intFree.erase (n_color);
            }

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

    bool intPop = false;
    bool xmmPop = false;

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
         * R1 -> R2 -> R3 -> ... -> Rn -> R1
         *
         * -> generate these moves:
         *  mov r1, free
         *  mov rn, r1
         *  ...
         *  mov r2, r3
         *  mov r1, r2
         *  mov free, r1
         */

         // start with pred node
        me::Op::Type type = linkTypes[*node->pred_.first()->value_->value_][*node->value_].type_;
        int tmpRegColor;

        if ( me::Op::isReal(type) )
        {
            if ( xmmFree.empty() )
            {
                // mov r1, mem: store in the 128 byte red zone area beyond RSP
                ofs_ << "\tmovdqa\t" 
                    << reg2str(*node->value_, type) 
                    << ", -16(%rsp)\n";
                xmmPop = true;
            }
            else
            {
                // mov r1, free
                tmpRegColor = *xmmFree.begin();
                swiftAssert(*node->value_ != tmpRegColor, "must be different");
                genMove(type, *node->value_, tmpRegColor);
            }
        }
        else
        {
            if ( intFree.empty() )
            {
                // mov r1, mem
                ofs_ << "\tpushq\t" << reg2str(*node->value_, type) << '\n';
                intPop = true;
                std::cout << "not tested" << std::endl;
            }
            else
            {
                // mov r1, free
                tmpRegColor = *intFree.begin();
                swiftAssert(*node->value_ != tmpRegColor, "must be different");
                genMove(type, *node->value_, tmpRegColor);
            }
        }

        std::vector<RegGraph::Node*> toBeErased;

        // iterate over the cycle
        RegGraph::Node* dst = node; // current node
        RegGraph::Node* src = node->pred_.first()->value_; // start with pred node
        while (src != node) // until we reach r1 again
        {
            // mov r_current, r_pred
            genMove(type, *src->value_, *dst->value_);

            // remember for erasion
            toBeErased.push_back(src);

            // go to pred node
            dst = src;
            src = src->pred_.first()->value_;
        }

        int dstColor = *dst->value_;

        /*
         * mov free, rn
         */
        if (intPop)
            ofs_ << "\tpopq\t%" << reg2str(dstColor, type) << '\n';
        else if (xmmPop)
        {
            // stored in the 128 byte red zone area beyond RSP
            ofs_ << "\tmovdqa\t-16(%rsp), " << reg2str(dstColor, type) << '\n';
        }
        else
            genMove(type, tmpRegColor, dstColor);

        toBeErased.push_back(node);

        // remove all handled nodes
        for (size_t i = 0; i < toBeErased.size(); ++i)
            rg.erase( toBeErased[i] );
    } // while there are still nodes left

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
                    case '*': return X64_MUL;
                    case '-': return X64_SUB;
                    case '/': return X64_DIV;
                    case me::AssignInstr::EQ: return X64_EQ;
                    case me::AssignInstr::NE: return X64_NE;
                    case '<': return X64_L;
                    case '>': return X64_G;
                    case me::AssignInstr::LE: return X64_LE;
                    case me::AssignInstr::GE: return X64_GE;
                    case '^': return X64_DEREF;
                    case me::AssignInstr::UNARY_MINUS: return X64_UN_MINUS;

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
            else if ( instrTypeId == typeid(me::Load) )
            {
                me::Load* load = (me::Load*) currentInstr;
                x64lval.load_ = load;
                return X64_LOAD;
            }
            else if ( instrTypeId == typeid(me::Store) )
            {
                me::Store* store = (me::Store*) currentInstr;
                x64lval.store_ = store;
                return X64_STORE;
            }
            else if ( instrTypeId == typeid(me::LoadPtr) )
            {
                me::LoadPtr* loadPtr = (me::LoadPtr*) currentInstr;
                x64lval.loadPtr_ = loadPtr;
                location = OP1;
                return X64_LOAD_PTR;
            }
            else if ( dynamic_cast<me::CallInstr*>(currentInstr) )
            {
                me::CallInstr* call = (me::CallInstr*) currentInstr;
                x64lval.call_ = call;
                location = END;
                return X64_CALL;
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

            me::AssignInstr* ai = dynamic_cast<me::AssignInstr*>(currentInstr);
            me::Op::Type type;
            if ( typeid(*currentInstr) == typeid(me::Load) )
                type = currentInstr->res_[0].var_->type_;
            else if ( ai && ai->isUnary() )
                type = currentInstr->res_[0].var_->type_;
            else
                type = currentInstr->arg_[0].op_->type_;

            switch (type)
            {
                case me::Op::R_BOOL:   return X64_BOOL;

                case me::Op::R_INT8:   return X64_INT8;
                case me::Op::R_INT16:  return X64_INT16;
                case me::Op::R_INT32:  return X64_INT32;
                case me::Op::R_INT64:  return X64_INT64;
                case me::Op::R_SAT8:   return X64_SAT8;
                case me::Op::R_SAT16:  return X64_SAT16;

                case me::Op::R_UINT8:  return X64_UINT8;
                case me::Op::R_UINT16: return X64_UINT16;
                case me::Op::R_UINT32: return X64_UINT32;
                case me::Op::R_UINT64: return X64_UINT64;
                case me::Op::R_USAT8:  return X64_USAT8;
                case me::Op::R_USAT16: return X64_USAT16;

                case me::Op::R_REAL32: return X64_REAL32;
                case me::Op::R_REAL64: return X64_REAL64;

                case me::Op::R_PTR:    return X64_UINT64;
                case me::Op::R_MEM:    return X64_STACK;

                case me::Op::S_INT8:   return X64_SIMD_INT8;
                case me::Op::S_INT16:  return X64_SIMD_INT16;
                case me::Op::S_INT32:  return X64_SIMD_INT32;
                case me::Op::S_INT64:  return X64_SIMD_INT64;
                case me::Op::S_SAT8:   return X64_SIMD_SAT8;
                case me::Op::S_SAT16:  return X64_SIMD_SAT16;

                case me::Op::S_UINT8:  return X64_SIMD_UINT8;
                case me::Op::S_UINT16: return X64_SIMD_UINT16;
                case me::Op::S_UINT32: return X64_SIMD_UINT32;
                case me::Op::S_UINT64: return X64_SIMD_UINT64;
                case me::Op::S_USAT8:  return X64_SIMD_USAT8;
                case me::Op::S_USAT16: return X64_SIMD_USAT16;

                case me::Op::S_REAL32: return X64_SIMD_REAL32;
                case me::Op::S_REAL64: return X64_SIMD_REAL64;
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
            else if ( opTypeId == typeid(me::MemVar) )
            {
                x64lval.memVar_ = (me::MemVar*) op;
                lastOp = X64_MEM_VAR;
            }
            else 
            {
                swiftAssert( typeid(*op) == typeid(me::Reg), "must be a Reg" );
                me::Reg* reg = (me::Reg*) op;
                x64lval.reg_ = reg;

                if ( reg->isSpilled() )
                    lastOp = X64_REG_1;
                else if ( currentInstr->res_.empty() )
                    lastOp = X64_REG_1;
                else if ( typeid(*currentInstr->res_[0].var_) != typeid(me::Reg) )
                    lastOp = X64_REG_1;
                else if ( currentInstr->res_[0].var_->isSpilled() )
                    lastOp = X64_REG_1;
                else if ( currentInstr->res_[0].var_->color_ == reg->color_ )
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
                if ( typeid(*currentInstr) == typeid(me::Store) )
                {
                    me::MemVar* memVar = dynamic_cast<me::MemVar*>(op);
                    if (memVar)
                    {
                        x64lval.memVar_ = memVar;
                        lastOp = X64_MEM_VAR;
                    }
                    else
                    {
                        swiftAssert( typeid(*op) == typeid(me::Reg), 
                                "must be a Reg here");
                        me::Reg* reg = (me::Reg*) op;
                        x64lval.reg_ = reg;
                        lastOp = X64_REG_2;
                    }
                }
                else
                {
                    swiftAssert( typeid(*op) == typeid(me::Reg), "must be a Reg here");
                    me::Reg* reg = (me::Reg*) op;
                    x64lval.reg_ = reg;

                    /*
                    * The following cases should be considered:
                    * - res.color == op2.color                 -> reg1
                    * - op1 == reg, op1.color == op2.color_    -> reg2
                    * - op1 == reg1                            -> reg2
                    * - op1 == const                           -> reg2
                    * - else                                   -> reg3
                    */
                    if ( currentInstr->res_[0].var_->color_ == reg->color_ )
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
            }
            return lastOp;
        }
        case END:
            return 0;
    }

    return 0;
}
