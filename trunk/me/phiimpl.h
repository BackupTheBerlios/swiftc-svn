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

#ifndef ME_PHI_IMPL_H
#define ME_PHI_IMPL_H

#include "utils/map.h"

#include "me/forward.h"
#include "me/op.h"

namespace me {

class PhiImpl
{
protected:

    /*
     * constructor
     */

    PhiImpl(BBNode* prevNode, BBNode* nextNode);

    /*
     * virtual methods
     */

    virtual bool isSpilled() const = 0;

    virtual void fillColors() = 0;
    virtual void eraseColors() = 0;

    virtual void genMove(Op::Type type, int r1, int r2) = 0;
    virtual void genReg2Tmp(Op::Type type, int r) = 0;
    virtual void genTmp2Reg(Op::Type type, int r) = 0;

    virtual void cleanUp() {}

public:

    /*
     * further methods
     */

    void genPhiInstr();

private:

    void collectFreeRegs();
    void buildDependencyGraph();
    void removeChains();
    void removeCycles();

    /*
     * data
     */

protected:

    int typeMask_;
    int freeRegTypeMask_;

private:

    BBNode* prevNode_;
    BBNode* nextNode_;
    BasicBlock* prevBB_;
    BasicBlock* nextBB_;

    class RegGraph : public Graph<int>
    {
        virtual std::string name() const
        {
            return "";
        }
    };

    typedef RegGraph::Node* RGNode;

    RegGraph rg_;

    typedef Map< int, Map<int, Op::Type> > LinkTypes;
    LinkTypes linkTypes_;

protected:

    Colors free_;
};

} // namespace me

#endif // ME_PHI_IMPL_H

