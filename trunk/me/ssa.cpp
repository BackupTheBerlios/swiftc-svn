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

#include "me/ssa.h"

#include <cstring>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include "utils/stringhelper.h"

#include "me/arch.h"
#include "me/cfg.h"
#include "me/functab.h"
#include "me/op.h"
#include "me/offset.h"
#include "me/struct.h"
#include "me/vectorizer.h"

namespace me {

//------------------------------------------------------------------------------

/*
 * constructors
 */

Res::Res(Var* var, int oldVarNr, int constraint /*= NO_CONSTRAINT*/)
    : var_(var)
    , oldVarNr_(oldVarNr)
    , constraint_(constraint)
{}

Res::Res(Var* var)
    : var_(var)
    , oldVarNr_(var->varNr_)
    , constraint_(NO_CONSTRAINT)
{}

/*
 * further methods
 */

std::string Res::toString() const
{
    return var_->toString();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Arg::Arg(Op* op, int constraint /*= NO_CONSTRAINT*/)
    : op_(op)
    , constraint_(constraint)
{}

/*
 * further methods
 */

std::string Arg::toString() const
{
    return op_->toString();
}

//------------------------------------------------------------------------------

template<class T>
std::string commaList(T begin, T end)
{
    std::ostringstream oss;

    while (begin != end)
    {
        oss << (*begin).toString() << ", ";
        ++begin;
    }

    std::string result = oss.str();

    if ( !result.empty() )
        result = result.substr(0, result.size() - 2);

    return result;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

InstrBase::InstrBase(size_t numLhs, size_t numRhs)
    : res_(numLhs)
    , arg_(numRhs)
    , constrained_(false)
{
    for (size_t i = 0; i < res_.size(); ++i)
        res_[i].constraint_ = NO_CONSTRAINT;

    for (size_t i = 0; i < arg_.size(); ++i)
        arg_[i].constraint_ = NO_CONSTRAINT;
}

InstrBase::~InstrBase()
{}

/*
 * further methods
 */

bool InstrBase::isVarUsed(Var* var) 
{
    // for each var on the arg
    for (size_t i = 0; i < arg_.size(); ++i) 
    {
        if (arg_[i].op_ == var)
            return true;
    }

    // -> not found
    return false;
}

bool InstrBase::isVarDefined(Var* var) 
{
    // for each var on the res
    for (size_t i = 0; i < res_.size(); ++i) 
    {
        if (res_[i].var_ == var)
            return true;
    }

    // -> not found
    return false;
}

struct ResFinder
{
    Var* var_;

    ResFinder(Var* var)
        : var_(var)
    {}

    bool operator () (const Res& res)
    {
        return var_ == res.var_;
    }
};

struct ArgFinder
{
    Op* op_;

    ArgFinder(Op* op)
        : op_(op)
    {}

    bool operator () (const Arg& arg)
    {
        return op_ == arg.op_;
    }
};

Var* InstrBase::findResult(Var* var)
{
    LHS::const_iterator iter = std::find_if( res_.begin(), res_.end(), ResFinder(var) );

    if ( iter == res_.end() )
        return false;

    return iter->var_;
}

Op* InstrBase::findArg(Op* op)
{
    RHS::const_iterator iter = std::find_if( arg_.begin(), arg_.end(), ArgFinder(op) );

    if ( iter == arg_.end() )
        return false;

    return iter->op_;
}

bool InstrBase::isConstrained() const
{
    return constrained_;
}

void InstrBase::constrain()
{
    constrained_ = true;
}

void InstrBase::unconstrainIfPossible()
{
    for (size_t i = 0; i < res_.size(); ++i)
    {
        if (res_[i].constraint_ != NO_CONSTRAINT)
            return;
    }

    for (size_t i = 0; i < arg_.size(); ++i)
    {
        if (arg_[i].constraint_ != NO_CONSTRAINT)
            return;
    }

    // all constraints were removed from this instruction
    constrained_ = false;
}

bool InstrBase::livesThrough(Var* var) const
{
    return liveIn_.contains(var) && liveOut_.contains(var);
}

InstrBase::OpType InstrBase::getOpType(size_t i) const
{
    if ( typeid(*arg_[i].op_) == typeid(Const) )
        return LITERAL;

    swiftAssert( dynamic_cast<Var*>(arg_[i].op_), "must be a Var" );
    Var* var = (Var*) arg_[i].op_;

    if ( !liveOut_.contains(var) )
        return VARIABLE_DEAD;
    else
        return VARIABLE;
}

bool InstrBase::isLastUse(InstrNode* instrNode, Var* var)
{
    InstrBase* instr = instrNode->value_;

    return   instr->liveIn_ .contains(var)  // must be in the live in
        &&  !instr->liveOut_.contains(var); // but not in the live out
}

std::string InstrBase::livenessString() const
{
    std::ostringstream oss;

    // print live in infos
    oss << "\tlive IN:" << std::endl;

    VARSET_EACH(iter, liveIn_)
        oss << "\t\t" << (*iter)->toString() << std::endl;

    // print live out infos
    oss << "\tlive OUT:" << std::endl;

    VARSET_EACH(iter, liveOut_)
        oss << "\t\t" << (*iter)->toString() << std::endl;

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * init statics
 */
int LabelInstr::counter_ = 1;

/*
 * constructor
 */

LabelInstr::LabelInstr()
    : InstrBase(0, 0)
{
    std::ostringstream oss;
    oss << "L" << number2String(counter_);
    label_ = oss.str();

    ++counter_;
}

/*
 * virtual methods
 */

InstrNode* LabelInstr::toSimd(Vectorizer* v)
{
    InstrNode* src = v->currentInstrNode_;

    // has label already been created?
    Vectorizer::Label2Label::iterator dstIter = v->src2dstLabel_.find(src);

    if ( dstIter != v->src2dstLabel_.end() )
        return dstIter->second;
    // else

    // create a new label
    InstrNode* dst = new InstrNode( new LabelInstr() );
    v->src2dstLabel_[src] = dst;

    return dst;
}

std::string LabelInstr::toString() const
{
    return label_;
}

/*
 * further methods
 */

std::string LabelInstr::asmName() const
{
    return std::string(".") + label_;
}

//------------------------------------------------------------------------------

/*
 * constructors
 */

NOP::NOP(Op* op)
    : InstrBase(0, 1)
{
    arg_[0].op_ = op;
}

/*
 * virtual methods
 */

InstrNode* NOP::toSimd(Vectorizer* v)
{
    return new InstrNode( new NOP(arg_[0].op_->toSimd(v)) );
}

std::string NOP::toString() const
{
    std::ostringstream oss;
    oss << "NOP(";
    oss << commaList( arg_.begin(), arg_.end() );

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

PhiInstr::PhiInstr(Var* result, size_t numRhs)
    : InstrBase(1, numRhs) // phi functions always have one result
    , sourceBBs_( new BBNode*[numRhs] )
{
    // init result
    res_[0] = Res(result);

    // fill everthing with zero -- needed for some debugging
    memset(sourceBBs_, 0, sizeof(BBNode*) * numRhs);
}

PhiInstr::~PhiInstr()
{
    delete[] sourceBBs_;
}

/*
 * getters
 */

Var* PhiInstr::result()
{
    return res_[0].var_;
}

const Var* PhiInstr::result() const
{
    return res_[0].var_;
}

int PhiInstr::oldResultNr() const
{
    return res_[0].oldVarNr_;
}

/*
 * virtual methods
 */

InstrNode* PhiInstr::toSimd(Vectorizer* v)
{
    PhiInstr* phi = new PhiInstr( res_[0].var_->toSimd(v), arg_.size() );

    for (size_t i = 0; i < arg_.size(); ++i)
    {
        phi->arg_[i] = Arg( arg_[i].op_->toSimd(v) );
        // remember src BBNode and substitute it later
        phi->sourceBBs_[i] = sourceBBs_[i];
    }

    return new InstrNode(phi);
}

std::string PhiInstr::toString() const
{
    std::ostringstream oss;
    // u03D5 is the unicode sign for a the greek phi letter
    oss << result()->toString() << "\t= \u03D5("; 

    for (size_t i = 0; i < arg_.size() - 1; ++i)
    {
        if (!sourceBBs_[i])
            continue;

        if (arg_[i].op_ && sourceBBs_[i])
            oss << arg_[i].op_->toString() << " (" << sourceBBs_[i]->value_->name() << "), ";
        else
            oss << "-, ";
    }

    if (arg_[arg_.size() - 1].op_ && sourceBBs_[arg_.size() - 1])
        oss << arg_[arg_.size()-1].op_->toString() << '(' << sourceBBs_[arg_.size()-1]->value_->name()  << ')';
    else
        oss << '-';

    oss << ')';

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructors
 */

AssignInstr::AssignInstr(int kind, Var* result, Op* op1, Op* op2 /*= 0*/)
    : InstrBase(1, op2 ? 2 : 1) 
    , kind_(kind)
{
    res_[0] = Res(result);
    arg_[0] = Arg(op1);

    if (op2)
        arg_[1] = Arg(op2);
}

AssignInstr::AssignInstr(int kind)
    : InstrBase(0, 0)
    , kind_(kind)
{}

/*
 * virtual methods
 */

InstrNode* AssignInstr::toSimd(Vectorizer* v)
{
    AssignInstr* simdAssign; 

    if ( arg_.size() == 1 )
    {
        simdAssign = new AssignInstr(kind_, 
                res_[0].var_->toSimd(v), 
                arg_[0]. op_->toSimd(v) );
    }
    else
    {
        swiftAssert( arg_.size() == 2, "must exactly have two arguments" );

        Var* res = res_[0].var_->toSimd(v);

        if ( isComparison() )
            res->type_ = Op::toSimd( arg_[0].op_->type_ );

        simdAssign = new AssignInstr(kind_, res,
                arg_[0]. op_->toSimd(v),
                arg_[1]. op_->toSimd(v) );
    }

    return new InstrNode(simdAssign);
}

std::string AssignInstr::toString() const
{
    std::string opString = getOpString();
    std::ostringstream oss;
    oss << res_[0].var_->toString();

    for (size_t i = 1; i < res_.size(); ++i)
        oss << ", " << res_[i].var_->toString();

    oss << '\t';

    // FIXME not accurate with dummy args

    // is this a binary, an unary instruction or an assignment?
    if (arg_.size() >= 2)
    {
        // it is a binary instruction
        oss << "= " << arg_[0].op_->toString() << " " << opString << " " << arg_[1].op_->toString();

        for (size_t i = 2; i < arg_.size(); ++i)
            oss << ", " << arg_[i].op_->toString();
    }
    else
    {
        if ( kind_ == NOT || kind_ == UNARY_MINUS || kind_ == '^' )
        {
            // it is an unary instruction
            oss << "= " << opString;
        }
        else
        {
            // it is an assignment
            oss << opString;
        }
        oss << ' ' << arg_[0].op_->toString();
    }

    if ( isConstrained() )
        oss << " (c)";

    return oss.str();
}

/*
 * further methods
 */

std::string AssignInstr::getOpString() const
{
    std::string opString;
    switch (kind_)
    {
        case UNARY_MINUS:
            opString = "-";
            break;
        case NOT:
            opString = "NOT";
            break;
        case AND:
            opString = "AND";
            break;
        case ANDN:
            opString = "ANDN";
            break;
        case OR:
            opString = "OR";
            break;
        case XOR:
            opString = "XOR";
            break;
        case EQ:
            opString = "==";
            break;
        case NE:
            opString = "!=";
            break;
        case LE:
            opString = "<=";
            break;
        case GE:
            opString = "\\>=";
            break;
        case '<':
            opString = "\\<"; // must be escaped for graphviz' dot
            break;
        case '>':
            opString = "\\>"; // must be escaped for graphviz' dot
            break;
        default:
            opString = c_;
    }

    return opString;
}

bool AssignInstr::isComparison() const
{
    return (kind_ == EQ  || kind_ == NE  || 
            kind_ == '<' || kind_ == '>' ||
            kind_ == LE  || kind_ == GE);
}

bool AssignInstr::isArithmetic() const
{
    return (kind_ == '+' || kind_ == '-' || 
            kind_ == '*' || kind_ == '/');
}

bool AssignInstr::isUnary() const
{
    return arg_.size() == 1;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Cast::Cast(Reg* result, Reg* reg)
    : InstrBase(1, 1)
{
    res_[0] = Res(result);
    arg_[0] = Arg(reg);
}

/*
 * virtual methods
 */

InstrNode* Cast::toSimd(Vectorizer* v)
{
    return new InstrNode( new Cast(
                (Reg*) res_[0].var_->toSimd(v), 
                (Reg*) arg_[0]. op_->toSimd(v)) );
}

std::string Cast::toString() const
{
    return res_[0].var_->toString() + " = CAST(" + arg_[0].op_->toString() + ")";
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

JumpInstr::JumpInstr(size_t numLhs, size_t numRhs, size_t numTargets)
    : InstrBase(numLhs, numRhs)
    , numTargets_(numTargets)
{
    swiftAssert(numTargets <= 2, 
        "currently only a maximum of 2 targets are supported");
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

GotoInstr::GotoInstr(InstrNode* instrTarget)
    : JumpInstr(0, 0, 1) // always no res, no arg, one target
{
    instrTargets_[0] = instrTarget;

    swiftAssert( typeid(*instrTargets_[0]->value_) == typeid(LabelInstr),
        "must be a node to a LabelInstr here");
}

/*
 * getters
 */

LabelInstr* GotoInstr::label()
{
    return (LabelInstr*) instrTargets_[0]->value_;
}

const LabelInstr* GotoInstr::label() const
{
    return (LabelInstr*) instrTargets_[0]->value_;
}

/*
 * virtual methods
 */

InstrNode* GotoInstr::toSimd(Vectorizer* v)
{
    InstrNode* target;

    Vectorizer::Label2Label::iterator targetIter = 
        v->src2dstLabel_.find( instrTargets_[0] );

    if ( targetIter == v->src2dstLabel_.end() )
    {
        // not found so create a new label in advance
        target = new InstrNode( new LabelInstr() );
        v->src2dstLabel_[ instrTargets_[0] ] = target;
    }
    else
        target = targetIter->second;

    return new InstrNode( new GotoInstr( target) );
}

std::string GotoInstr::toString() const
{
    std::ostringstream oss;
    oss << "GOTO " << label()->toString();

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

BranchInstr::BranchInstr(Op* boolOp, InstrNode* trueLabel, InstrNode* falseLabel)
    : JumpInstr(0, 1, 2) // always no results, one arg, two targets
    , cc_(CC_NOT_SET)
{
    //swiftAssert(boolOp->type_ == Var::R_BOOL, "this is not a boolean pseudo var");
    arg_[0].op_ = boolOp;

    instrTargets_[TRUE_TARGET] = trueLabel;
    instrTargets_[FALSE_TARGET] = falseLabel;

    swiftAssert( typeid(*instrTargets_[0]->value_) == typeid(LabelInstr),
        "must be a node to a LabelInstr here");
    swiftAssert( typeid(*instrTargets_[1]->value_) == typeid(LabelInstr),
        "must be a node to a LabelInstr here");
}

/*
 * virtual methods
 */

InstrNode* BranchInstr::toSimd(Vectorizer* v)
{
    InstrNode* trueNode;
    InstrNode* falseNode;

    Vectorizer::Label2Label::iterator trueIter = 
        v->src2dstLabel_.find( instrTargets_[TRUE_TARGET] );

    Vectorizer::Label2Label::iterator falseIter = 
        v->src2dstLabel_.find( instrTargets_[FALSE_TARGET] );

    if ( trueIter == v->src2dstLabel_.end() )
    {
        // not found so create a new label in advance
        trueNode = new InstrNode( new LabelInstr() );
        v->src2dstLabel_[ instrTargets_[TRUE_TARGET] ] = trueNode;
    }
    else
        trueNode = trueIter->second;

    if ( falseIter == v->src2dstLabel_.end() )
    {
        // not found so create a new label in advance
        falseNode = new InstrNode( new LabelInstr() );
        v->src2dstLabel_[ instrTargets_[FALSE_TARGET] ] = falseNode;
    }
    else
        falseNode = falseIter->second;

    return new InstrNode( new BranchInstr(
                arg_[0].op_->toSimd(v), trueNode, falseNode) );
}

std::string BranchInstr::toString() const
{
    std::ostringstream oss;
    oss << "IF " << arg_[0].op_->toString()
        << " THEN " << trueLabel()->toString()
        << " ELSE " << falseLabel()->toString();

    return oss.str();
}

/*
 * getters
 */

LabelInstr* BranchInstr::trueLabel() 
{
    return (LabelInstr*) instrTargets_[TRUE_TARGET]->value_;
}

const LabelInstr* BranchInstr::trueLabel() const
{
    return (LabelInstr*) instrTargets_[TRUE_TARGET]->value_;
}

LabelInstr* BranchInstr::falseLabel() 
{
    return (LabelInstr*) instrTargets_[FALSE_TARGET]->value_;
}

const LabelInstr* BranchInstr::falseLabel() const
{
    return (LabelInstr*) instrTargets_[FALSE_TARGET]->value_;
}

Op* BranchInstr::getOp()
{
    return arg_[0].op_;
}

const Op* BranchInstr::getOp() const
{
    return arg_[0].op_;
}

Reg* BranchInstr::getMask() 
{
    swiftAssert( res_.size() == 1, "must be 1" );
    swiftAssert( typeid(*res_[0].var_) == typeid(Reg), "must be a Reg here" );
    return (Reg*) res_[0].var_;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Spill::Spill(Var* result, Var* arg)
    : InstrBase(1, 1)
{
    swiftAssert( result->isSpilled(), "result must be a memory var" );
    res_[0] = Res(result);

    swiftAssert( !arg->isSpilled(), "arg must not be a memory var" );
    arg_[0] = Arg(arg);
}

/*
 * virtual methods
 */

InstrNode* Spill::toSimd(Vectorizer* v)
{
    swiftAssert(false, "unreachable code");
    return 0;
}

std::string Spill::toString() const
{
    std::ostringstream oss;
    oss << res_[0].var_->toString() << "\t= spill(" << arg_[0].op_->toString() << ")";

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Reload::Reload(Var* result, Var* arg)
    : InstrBase(1, 1)
{
    swiftAssert( !result->isSpilled(), "result must not be a memory var" );
    res_[0] = Res(result);

    swiftAssert( arg->isSpilled(), "arg must be a memory var" );
    arg_[0] = Arg(arg);
}

/*
 * virtual methods
 */

InstrNode* Reload::toSimd(Vectorizer* v)
{
    swiftAssert(false, "unreachable code");
    return 0;
}

std::string Reload::toString() const
{
    std::ostringstream oss;
    oss << res_[0].var_->toString() << "\t= reload(" 
        << arg_[0].op_->toString() << ")";

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Load::Load(Var* result, Var* location, Reg* index, Offset* offset)
    : InstrBase(1, index ? 2 : 1)
    , offset_(offset)
{
    swiftAssert(location->type_ == Op::R_PTR || location->type_ == Op::R_MEM, 
            "must be an R_PTR");

    res_[0] = Res(result);
    arg_[0] = Arg(location);

    if (index)
        arg_[1] = Arg(index);
}

Load::~Load()
{
    delete offset_;
}
/*
 * virtual methods
 */

InstrNode* Load::toSimd(Vectorizer* v)
{
    Reg* index = (arg_.size() == 2) 
               ? (Reg*) arg_[1].op_->toSimd(v) 
               : 0;

    return new InstrNode( new Load( 
                res_[0].var_->toSimd(v), 
                (Var*) arg_[0].op_->toSimd(v), 
                index, 
                offset_->toSimd()) );
}

std::string Load::toString() const
{
    std::ostringstream oss;
    oss << res_[0].var_->toString() << "\t= Load(" 
        << arg_[0].op_->toString();
    if ( arg_.size() == 2 )
        oss << ", " << arg_[1].op_->toString();

    if (offset_)
        oss << ", " << offset_->toString();

    oss << ')';

    return oss.str();
}

/*
 * further methods
 */

size_t Load::getOffset() const
{
    return offset_ ? offset_->getOffset() : 0;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

LoadPtr::LoadPtr(Reg* result, Var* location, Reg* index, Offset* offset)
    : InstrBase(1, index ? 2 : 1)
    , offset_(offset)
{
    swiftAssert(result->type_ == Op::R_PTR, "must be an R_PTR");
    swiftAssert(location->type_ == Op::R_PTR || location->type_ == Op::R_MEM, 
            "must be an R_PTR");

    res_[0] = Res(result);
    arg_[0] = Arg(location);

    if (index)
        arg_[1] = Arg(index);
}

LoadPtr::~LoadPtr()
{
    delete offset_;
}

/*
 * virtual methods
 */

InstrNode* LoadPtr::toSimd(Vectorizer* v)
{
    swiftAssert(false, "TODO");
    return 0;
}

std::string LoadPtr::toString() const
{
    std::ostringstream oss;
    oss << res_[0].var_->toString() << "\t= LoadPtr(" 
        << arg_[0].op_->toString();

    if ( arg_.size() == 2 )
        oss << ", " << arg_[1].op_->toString();

    if (offset_)
         oss << ", " << offset_->toString(); 
    
    oss << ')';

    return oss.str();
}

/*
 * further methods
 */

size_t LoadPtr::getOffset() const
{
    return offset_ ? offset_->getOffset() : 0;
}

//------------------------------------------------------------------------------

/*
 * constructor and destructor
 */

Store::Store(Op* arg, Var* location, Reg* index, Offset* offset)
    : InstrBase( 
            (location->type_ == Op::R_MEM) ? 1 : 0, 
            index ? 3 : 2 )
    , offset_(offset)
{
    // -> store to an arbitrary location in memory
    swiftAssert(location->type_ == Op::R_PTR || location->type_ == Op::R_MEM, 
            "must be an R_PTR or R_MEM");

    arg_[0] = Arg(arg);
    arg_[1] = Arg(location);
    
    if (index)
        arg_[2] = Arg(index);

    if (location->type_ == Op::R_MEM)
    {
        // -> store to a location on the stack
        res_[0] = Res(location);
    }
}

Store::~Store()
{
    delete offset_;
}

/*
 * virtual methods
 */

InstrNode* Store::toSimd(Vectorizer* v)
{
    Reg* index = (arg_.size() == 3) 
               ? (Reg*) arg_[2].op_->toSimd(v) 
               : 0;

    return new InstrNode( new Store( 
                arg_[0].op_->toSimd(v),
                (Var*) arg_[1].op_->toSimd(v), 
                index, 
                offset_->toSimd()) );
}

std::string Store::toString() const
{
    std::ostringstream oss;

    if ( res_.empty() )
    {
        // -> store to an arbitrary location in memory
        oss << "Store("
            << arg_[0].op_->toString() << ", "
            << arg_[1].op_->toString();

        if ( arg_.size() == 3 )
            oss << ", " << arg_[2].op_->toString();
    }
    else
    {
        // -> store to a location on the stack
        oss << res_[0].var_->toString() << "\t= Store(" 
            << arg_[0]. op_->toString() << ", "
            << arg_[1]. op_->toString();

        if ( arg_.size() == 3 )
            oss << ", " << arg_[2].op_->toString();
    }

    if (offset_)
        oss << ", (" << offset_->toString() << ')';

    oss << ')';

    return oss.str();
}

/*
 * further methods
 */

size_t Store::getOffset() const
{
    return offset_ ? offset_->getOffset() : 0;
}

MemVar* Store::resMemVar()
{
    swiftAssert( typeid(*res_[0].var_) == typeid(MemVar), "must be a MemVar" );
    return (MemVar*) res_[0].var_;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

SetParams::SetParams(size_t numLhs)
    : InstrBase(numLhs, 0)
{}

/*
 * virtual methods
 */

InstrNode* SetParams::toSimd(Vectorizer* v)
{
    SetParams* setParams = new SetParams( res_.size() );
    for (size_t i = 0; i < res_.size(); ++i)
        setParams->res_[i] = Res(res_[i].var_->toSimd(v));//, res_[i].oldVarNr_);

    return new InstrNode(setParams);
}

std::string SetParams::toString() const
{
    swiftAssert( res_.size() >= 1, "must have at least one res" );
    
    std::ostringstream oss;
    oss << commaList( res_.begin(), res_.end() ) << "\t= setParams()";

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

SetResults::SetResults(size_t numRhs)
    : InstrBase(0, numRhs)
{}

/*
 * virtual methods
 */

InstrNode* SetResults::toSimd(Vectorizer* v)
{
    // TODO
    return 0;
}

std::string SetResults::toString() const
{
    swiftAssert( arg_.size() >= 1, "must have at least one arg" );

    std::ostringstream oss;
    oss << "setResults(" << commaList( arg_.begin(), arg_.end() ) << ')';

    return oss.str();
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

CallInstr::CallInstr(size_t numLhs, 
                     size_t numRhs, 
                     const std::string& symbol,
                     bool vararg /*= false*/)
    : InstrBase(numLhs, numRhs)
    , numLhs_(numLhs)
    , numRhs_(numRhs)
    , symbol_(symbol)
    , vararg_(vararg)
{}

/*
 * virtual methods
 */

InstrNode* CallInstr::toSimd(Vectorizer* v)
{
    // TODO
    return 0;
}

std::string CallInstr::toString() const
{
    std::ostringstream oss;

    // use this for a more verbose output
#if 0
    oss << commaList( res_.begin(), res_.end() )
        << " = " << symbol_ << '(' 
        << commaList( arg_.begin(), arg_.end() ) << ')';
#else
    oss << commaList( res_.begin(), res_.begin() + numLhs_ )
        << " = " << symbol_ << '(' 
        << commaList( arg_.begin(), arg_.begin() + numRhs_ ) << ')';
#endif

    return oss.str();
}

/*
 * further methods
 */

bool CallInstr::isVarArg() const
{
    return vararg_;
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Malloc::Malloc(Reg* ptr, Op* size)
    : CallInstr(1, 1, "malloc")
{
    swiftAssert(ptr->type_ == Op::R_PTR, "must be a Ptr here");
    swiftAssert(size->type_ == Op::R_UINT64, "must be a Ptr here");// TODO make this arch independently
    swiftAssert(typeid(*size) != typeid(MemVar), "must be a Ptr here");
    res_[0] = Res(ptr);
    arg_[0] = Arg(size);
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Free::Free(Reg* ptr)
    : CallInstr(1, 1, "free")
{
    swiftAssert(ptr->type_ == Op::R_PTR, "must be a Ptr here");
    arg_[0] = Arg(ptr);
    res_[0] = Res(ptr);
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Memcpy::Memcpy(Reg* src, Reg* dst)
    : CallInstr(0, 2, "memcpy")
{
    swiftAssert(src->type_ == Op::R_PTR, "must be a Ptr here");
    swiftAssert(dst->type_ == Op::R_PTR, "must be a Ptr here");
    arg_[0] = Arg(src);
    arg_[1] = Arg(dst);
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Pack::Pack(Reg* result, Op* op)
    : InstrBase(1, 1)
{
    swiftAssert( me::Op::toSimd(op->type_) == result->type_,
            "result->type_ must be the simd version of reg->type_");
    res_[0] = Res(result);
    arg_[0] = Arg(op);
}

/*
 * virtual methods
 */

InstrNode* Pack::toSimd(Vectorizer* v)
{
    swiftAssert(false, "unreachable code");
    return 0;
}

std::string Pack::toString() const
{
    return res_[0].var_->toString() + "\tPack(" + arg_[0].op_->toString() + ')';
}

//------------------------------------------------------------------------------

/*
 * constructor
 */

Unpack::Unpack(Reg* result, Op* op)
    : InstrBase(1, 1)
{
    swiftAssert( me::Op::toSimd(result->type_) == op->type_,
            "reg->type_ must be the simd version of result->type_");
    res_[0] = Res(result);
    arg_[0] = Arg(op);
}

/*
 * virtual methods
 */

InstrNode* Unpack::toSimd(Vectorizer* v)
{
    swiftAssert(false, "unreachable code");
    return 0;
}

std::string Unpack::toString() const
{
    return res_[0].var_->toString() + "\tUnpack(" + arg_[0].op_->toString() + ')';
}

} // namespace me
