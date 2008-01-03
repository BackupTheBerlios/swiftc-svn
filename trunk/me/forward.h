#ifndef ME_FORWARD_H
#define ME_FORWARD_H

#include <map>
#include <set>

#include "utils/list.h"
#include "utils/graph.h"

// include this file for useful forward declarations

namespace me {

struct BasicBlock;
typedef Graph<BasicBlock>::Node* BBNode;
typedef List<BBNode> BBList;

struct Function;

struct InstrBase;
typedef List<InstrBase*> InstrList;
typedef InstrList::Node* InstrNode;

struct PseudoReg;
typedef List<PseudoReg*> RegList;
typedef std::map<int, PseudoReg*> RegMap;
typedef std::set<PseudoReg*> RegSet;

} // namespace me

#endif // ME_FORWARD_H
