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

#ifndef SWIFT_CMDLINEPARSER_H
#define SWIFT_CMDLINEPARSER_H

#include <map>
#include <string>

namespace llvm {
    class Pass;
}

namespace swift {

class CmdBase;

//------------------------------------------------------------------------------

class CmdLineParser
{
public:

    CmdLineParser(int argc, char** argv);
    ~CmdLineParser();

    const char* getFilename() const;
    bool result() const;
    bool dump() const;
    bool cleanDump() const;
    bool optSize() const;
    bool unroolLoops() const;
    bool unitAtATime() const;
    bool simplifyLibCalls() const;
    unsigned optLevel() const;
    llvm::Pass* inlinePass() const;

private:

    int argc_;
    char** argv_;
    const char* filename_;
    bool result_;

    bool dump_;
    bool cleanDump_;
    bool optSize_;
    bool unroolLoops_;
    bool unitAtATime_;
    bool simplifyLibCalls_;
    unsigned optLevel_;
    llvm::Pass* inlinePass_;

    typedef std::map<std::string, CmdBase*> Cmds;
    static Cmds cmds_;
    static std::string usage_;

    template <class T> friend class Cmd;
};

//------------------------------------------------------------------------------

} // namespace swift

#endif // SWIFT_CMDLINEPARSER_H
