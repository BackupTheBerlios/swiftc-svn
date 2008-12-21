#ifndef ME_FORWARD_H
#define ME_FORWARD_H

#include <map>

#include "utils/list.h"
#include "utils/graph.h"
#include "utils/set.h"

// include this file for useful forward declarations

namespace me {

struct BasicBlock;
typedef Graph<BasicBlock>::Node BBNode;
typedef List<BBNode*> BBList;
typedef Set<BBNode*> BBSet;

struct Function;

struct InstrBase;
typedef List<InstrBase*> InstrList;
typedef InstrList::Node InstrNode;

struct Op;
struct Literal;
struct Var;
struct Mem;
struct Reg;
typedef List<Reg*> RegList;
typedef std::map<int, Reg*> RegMap;
typedef Set<Reg*> RegSet;
typedef std::vector<Reg*> RegVec;

} // namespace me

#endif // ME_FORWARD_H
