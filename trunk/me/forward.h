#ifndef ME_FORWARD_H
#define ME_FORWARD_H

#include <map>
#include <set>

#include "utils/list.h"
#include "utils/graph.h"

// include this file for useful forward declarations

namespace me {

struct BasicBlock;
typedef Graph<BasicBlock>::Node BBNode;
typedef List<BBNode*> BBList;
typedef std::set<BBNode*> BBSet;

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
typedef std::set<Reg*> RegSet;

} // namespace me

#endif // ME_FORWARD_H
