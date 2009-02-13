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
#include "me/cfg.h" 
#include "me/op.h"
#include "me/ssa.h"

namespace me {

//------------------------------------------------------------------------------

// forward declarations
struct Function;
struct Struct;
struct Member;

//------------------------------------------------------------------------------

/**
 * Function has in, inout and out going parameters and, of course, an identifier.
 */
struct Function
{
    std::string* id_;
    int regCounter_; ///< Counter which gives new reg in SSA form number. Size is increasing.
    int varCounter_; ///< Counter which gives new var numbers. Size is decreasing.
    CFG  cfg_;

    /// Indicates whether a LivenessAnalysis CodePass has already been performed.
    bool firstLiveness_;

    /// Indicates whether a DefUseCalc CodePass has already been performed.
    bool firstDefUse_;

    InstrNode* lastLabelNode_;
    InstrList instrList_;

    RegMap in_;
    RegMap inout_;
    RegMap out_;
    RegMap vars_;

    typedef std::map<Reg*, int> Reg2Color;
    Reg2Color reg2Color_;

    int spillSlots_;


    /*
     * constructor and destructor
     */

    Function(std::string* id);
    ~Function();

    /*
     * further methods
     */

#ifdef SWIFT_DEBUG

    /**
     * @brief This method creates a new temp Reg.
     * This is a var which is guaranteed to be defined only once.
     *
     * @param type The type of the Reg.
     * @param id The name of the original var.
     */
    Reg* newSSA(Op::Type type, std::string* id = 0);

    /**
     * @brief This method creates a new var Reg.
     * This will be transforemd to SSA form later on.
     *
     * @param type The type of the Reg.
     * @param id The name of the original var.
     */
    Reg* newVar(Op::Type type, std::string* id);

    /**
     * @brief This method creates a new var in a memory location.
     *
     * @param type The type of the Reg.
     * @param id The name of the original var.
     */
    Reg* newMemSSA(Op::Type type, std::string* id = 0);

#else // SWIFT_DEBUG

    /**
     * @brief This method creates a new temp Reg.
     * This is a var which is guaranteed to be defined only once.
     *
     * @param type The type of the Reg.
     */
    Reg* newSSA(Op::Type type);

    /**
     * @brief This method creates a new var Reg.
     * This will be transforemd to SSA form later on.
     *
     * @param type the type of the Reg.
     */
    Reg* newVar(Op::Type type);

    /**
     * @brief This method creates a new var in a memory location.
     *
     * @param type The type of the Reg.
     */
    Reg* newMemSSA(Op::Type type);

#endif // SWIFT_DEBUG

    inline void insert(Reg* reg);

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

    typedef std::map<int, Struct*> StructMap;
    StructMap structs_;

    /*
     * constructor and destructor
     */

    FunctionTable(const std::string& filename);
    ~FunctionTable();

    /*
     * further methods
     */

    Function* insertFunction(std::string* id);


#ifdef SWIFT_DEBUG

    /**
     * @brief This method creates a new Reg which is only defined once.
     *
     * @param type the type of the Reg
     */
    Reg* newSSA(Op::Type type, std::string* id = 0);

    /**
     * @brief This method creates a new var Reg.
     *
     * @param type the type of the Reg
     * @param id the name of the original var
     */
    Reg* newVar(Op::Type type, std::string* id);

    /**
     * @brief This method creates a new memory variable.
     *
     * @param type the type of the Reg
     * @param id the name of the original var
     */
    Reg* newMemSSA(Op::Type type, std::string* id = 0);

#else // SWIFT_DEBUG

    /**
     * @brief This method creates a new temp Reg.
     *
     * @param type the type of the Reg
     */
    Reg* newSSA(Op::Type type);

    /**
     * @brief This method creates a new var Reg.
     * 
     * @param type the type of the Reg
     */
    Reg* newVar(Op::Type type);

    /**
     * @brief This method creates a new memory variable.
     *
     * @param type the type of the Reg
     */
    Reg* newMemSSA(Op::Type type);

#endif // SWIFT_DEBUG

    Reg* lookupReg(int id);

    void appendInstr(InstrBase* instr);
    void appendInstrNode(InstrNode* node);
    InstrNode* getLastLabelNode();

    void buildUpME();

    void dumpSSA();
    void dumpDot();

    void genCode();

    /*
     * struct handling
     */

    void enterStruct(Struct* _struct);
    void leaveStruct();
    Struct* currentStruct();

#ifdef SWIFT_DEBUG

    Struct* newStruct(const std::string& id);
    Member* appendMember(Op::Type type, const std::string& id);
    Member* appendMember(Struct* _struct, const std::string& id);

#else // SWIFT_DEBUG

    Struct* newStruct();
    Member* appendMember(Op::Type type);
    //Member* appendMember(Struct* _struct);

#endif // SWIFT_DEBUG

};

typedef FunctionTable FuncTab;
extern FuncTab* functab;

} // namespace me

#endif // ME_FUNCTAB_H
