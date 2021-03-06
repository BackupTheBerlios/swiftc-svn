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

#ifndef ME_FUNCTAB_H
#define ME_FUNCTAB_H

#include <fstream>
#include <iostream>
#include <map>
#include <stack>

#include "utils/list.h"
#include "utils/stringhelper.h"

#include "me/basicblock.h"
#include "me/op.h"
#include "me/ssa.h"

namespace me {

//------------------------------------------------------------------------------

/*
 * forward declarations
 */
struct Aggregate;
struct CFG;
struct Function;
struct Member;
struct StackLayout;
struct Struct;

typedef Set<int> Colors;

//------------------------------------------------------------------------------

/**
 * Function has in, inout and out going parameters and, of course, an identifier.
 */
struct Function
{
    std::string* id_;

    /**
     * Counter which gives new vars in SSA form a number. 
     * Size is increasing.
     */
    int ssaCounter_; 

    /**
     * Counter which gives new var numbers. 
     * Size is decreasing.
     */
    int varCounter_; 

    CFG* cfg_;

    /// Indicates whether a LivenessAnalysis CodePass has already been performed.
    bool firstLiveness_;

    /// Indicates whether a DefUseCalc CodePass has already been performed.
    bool firstDefUse_;

    InstrNode* functionEpilogue_;
    InstrList instrList_;

    VarVec res_;
    VarVec arg_;
    VarMap vars_;

    typedef std::vector<Const*> Consts;
    /** 
     * Here are all \a Const collected for a clean tidy up. 
     * This data structure does not have another purpose.
     */
    Consts consts_;

    typedef std::vector<Undef*> Undefs;
    /** 
     * Here are all \a Undef collected for a clean tidy up. 
     * This data structure does not have another purpose.
     */
    Undefs undefs_;

    typedef std::map<Var*, int> Var2Color;
    Var2Color var2Color_;

    StackLayout* stackLayout_;

    bool ignore_; // TODO needed?
    bool isMain_;
    bool vectorize_;

#ifdef SWIFT_DEBUG
    static bool mainSet_;
#endif // SWIFT_DEBUG

    /// All used colors in this function.
    Colors usedColors_;

    /*
     * constructor and destructor
     */

    Function(std::string* id, size_t stackPlaces, bool ignore, bool vectorize = false);
    ~Function();

    /*
     * further methods
     */

#ifdef SWIFT_DEBUG

    Reg*    newReg(Op::Type type, const std::string* id = 0);
    Reg*    newSSAReg(Op::Type type, const std::string* id = 0);
    Reg*    newSpilledSSAReg(Op::Type type, const std::string* id = 0);
    MemVar* newMemVar(Aggregate* aggregate, const std::string* id = 0);
    MemVar* newSSAMemVar(Aggregate* aggregate, const std::string* id = 0);

#else // SWIFT_DEBUG

    Reg*    newSSAReg(Op::Type type);
    Reg*    newReg(Op::Type type);
    Reg*    newSpilledSSAReg(Op::Type type);
    MemVar* newMemVar(Aggregate* aggregate);
    MemVar* newSSAMemVar(Aggregate* aggregate);

#endif // SWIFT_DEBUG

    Var* cloneNewSSA(Var* var);

    Const* newConst(Op::Type type, size_t numBoxElems = 1);
    Undef* newUndef(Op::Type type);

    void insert(Var* var);

    bool ignore() const;

    void appendArg(Var* arg);
    void appendRes(Var* res);
    void buildFunctionEntry();
    void buildFunctionExit();
    bool isMain() const;
    void setAsMain();

    std::string getId() const;

    /*
     * dump methods
     */

    void dumpSSA(std::ofstream& ofs);
    void dumpDot(const std::string& baseFilename);
};

//------------------------------------------------------------------------------

struct FunctionTable
{
    std::string filename_;

    typedef std::map<std::string*, Function*, StringPtrCmp> FunctionMap;
    FunctionMap functions_;
    Function*   currentFunction_;
    
    typedef std::stack<Struct*> StructStack;
    StructStack structStack_;

    typedef std::vector<Struct*> Structs;
    Structs structs_;

    /*
     * constructor and destructor
     */

    FunctionTable(const std::string& filename);
    ~FunctionTable();

    /*
     * further methods
     */

    Function* insertFunction(std::string* id, bool ignore, bool vectorize = false);

#ifdef SWIFT_DEBUG

    Reg*    newReg(Op::Type type, const std::string* id = 0);
    Reg*    newSSAReg(Op::Type type, const std::string* id = 0);
    Reg*    newSpilledSSAReg(Op::Type type, const std::string* id = 0);
    MemVar* newMemVar(Aggregate* aggregate, const std::string* id = 0);
    MemVar* newSSAMemVar(Aggregate* aggregate, const std::string* id = 0);

#else // SWIFT_DEBUG

    Reg*    newReg(Op::Type type);
    Reg*    newSSAReg(Op::Type type);
    Reg*    newSpilledSSAReg(Op::Type type);
    MemVar* newMemVar(Aggregate* aggregate);
    MemVar* newSSAMemVar(Aggregate* aggregate);

#endif // SWIFT_DEBUG
    

    Var* cloneNewSSA(Var* var);

    Const* newConst(Op::Type type, size_t numBoxElems = 1);
    Undef* newUndef(Op::Type type);

    Var* lookupVar(int id);

    void appendInstr(InstrBase* instr);
    void appendInstrNode(InstrNode* node);
    InstrNode* getFunctionEpilogue();

    void analyzeStructs();
    void buildUpME();

    void dumpSSA();
    void dumpDot();

    void genCode();

    void appendArg(Var* arg);
    void appendRes(Var* res);
    void buildFunctionEntry();
    void buildFunctionExit();

    std::string getId() const;

    void setAsMain();

    /*
     * struct handling
     */

    void enterStruct(Struct* _struct);
    void leaveStruct();
    Struct* currentStruct();

#ifdef SWIFT_DEBUG

    Struct* newStruct(const std::string& id);
    Member* appendMember(Aggregate* aggregate, const std::string& id);

#else // SWIFT_DEBUG

    Struct* newStruct();
    Member* appendMember(Aggregate* aggregate);

#endif // SWIFT_DEBUG

    Struct* vectorize(Struct* _struct);
};

typedef FunctionTable FuncTab;
extern FuncTab* functab;

} // namespace me

#endif // ME_FUNCTAB_H
