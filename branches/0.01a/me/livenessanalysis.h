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

#ifndef ME_LIVENESS_ANALYSIS
#define ME_LIVENESS_ANALYSIS

#include "me/codepass.h"
#include "me/forward.h"

namespace me {

// forward declaration
class IGraph;

class LivenessAnalysis : public CodePass
{
public:

    /*
     * constructor and destructor
     */

#ifdef SWIFT_DEBUG

    LivenessAnalysis(Function* function, bool dumpIG = false);
    ~LivenessAnalysis();

#else // SWIFT_DEBUG

    LivenessAnalysis(Function* function);

#endif // SWIFT_DEBUG

    /*
     * methods
     */

    /** 
     * @brief Performs the liveness analysis.
     *
     * Invokes \a liveOutAtBlock, \a liveInAtInstr and \a liveOutAtInstr.
     */
    virtual void process();

private:

    void liveOutAtBlock(BBNode* bbNode, Var* var);
    void liveInAtInstr (InstrNode* instrNode, Var* var);
    void liveOutAtInstr(InstrNode* instrNode, Var* var);

private:

    /// Knows during the liveness analysis which basic blocks have already been visted.
    BBSet walked_;

#ifdef SWIFT_DEBUG
    IGraph* ig_;
    bool dumpIG_;
#endif // SWIFT_DEBUG

};

} // namespace me

#endif // ME_LIVENESS_ANALYSIS
