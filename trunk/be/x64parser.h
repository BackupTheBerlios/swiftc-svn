#ifndef BE_X64_PARSER_H
#define BE_X64_PARSER_H

#include <fstream>

extern "C" int x64parse();
extern std::ofstream* x64_ofs; 

namespace me {

// it is important to declare all union members of YYSTYPE here
struct Op;
struct Undef;
struct Const;
struct Reg;

struct LabelInstr;
struct GotoInstr;
struct BranchInstr;
struct AssignInstr;
struct Spill;
struct Reload;

} // namespace be

// include auto generated parser header before tokens
#include "x64parser.tab.hpp"

#endif // BE_X64_PARSER_H
