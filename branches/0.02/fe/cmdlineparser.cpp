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

#include "cmdlineparser.h"

#include <llvm/Pass.h>
#include <llvm/Transforms/IPO.h>

#include <iostream>

namespace swift {

//------------------------------------------------------------------------------

class CmdBase
{
public:

    CmdBase(CmdLineParser& clp);

    virtual void execute() = 0;

protected:

    CmdLineParser& clp_;
};

//------------------------------------------------------------------------------

template <class T> class Cmd;

//------------------------------------------------------------------------------

template<>
class Cmd <class Dump> : public CmdBase
{
public:

    Cmd(CmdLineParser& clp);

    virtual void execute();
};

typedef Cmd<class Dump> DumpCmd;

//------------------------------------------------------------------------------

template<>
class Cmd <class CleanDump> : public CmdBase
{
public:

    Cmd(CmdLineParser& clp);

    virtual void execute();
};

typedef Cmd<class CleanDump> CleanDumpCmd;

//------------------------------------------------------------------------------

template<>
class Cmd <class OptLevel> : public CmdBase
{
public:

    Cmd(CmdLineParser& clp, unsigned optLevel);

    virtual void execute();

private:

    unsigned optLevel_;
};

typedef Cmd<class OptLevel> OptLevelCmd;

//------------------------------------------------------------------------------

template<>
class Cmd <class OptSize> : public CmdBase
{
public:

    Cmd(CmdLineParser& clp);

    virtual void execute();

private:

    unsigned optLevel_;
};

typedef Cmd<class OptSize> OptSizeCmd;

//------------------------------------------------------------------------------



std::string CmdLineParser::usage_ = std::string("Usage: swiftc [options] file");
CmdLineParser::Cmds CmdLineParser::cmds_ = CmdLineParser::Cmds();

CmdLineParser::CmdLineParser(int argc, char** argv)
    : argc_(argc)
    , argv_(argv)
    , filename_(0)
    , result_(true)
    , dump_(false)
    , cleanDump_(false)
    , optSize_(false)
    , unroolLoops_(false)
    , unitAtATime_(false)
    , simplifyLibCalls_(true)
    , optLevel_(0)
    , inlinePass_(0)
{
    // create command data structure
    cmds_["--dump"] = new DumpCmd(*this);
    cmds_["--clean-dump"] = new CleanDumpCmd(*this);
    cmds_["-Os"] = new OptSizeCmd(*this);
    cmds_["-O0"] = new OptLevelCmd(*this, 0);
    cmds_["-O1"] = new OptLevelCmd(*this, 1);
    cmds_["-O2"] = new OptLevelCmd(*this, 2);
    cmds_["-O3"] = new OptLevelCmd(*this, 3);

    // for each argument except the first one which is the program name
    for (int i = 1; i < argc_; ++i)
    {
        Cmds::iterator iter = cmds_.find(argv_[i]);
        if ( iter != cmds_.end() )
            iter->second->execute();
        else
        {
            if (!filename_)
                filename_ = argv_[i];
            else
            {
                std::cerr << "error: multiple filenames given" << std::endl;
                result_ = false;
                return;
            }
        }
    }

    if (filename_ == 0)
        result_ = false;
}

CmdLineParser::~CmdLineParser()
{
    for (Cmds::iterator iter = cmds_.begin(); iter != cmds_.end(); ++iter)
        delete iter->second;
}

const char* CmdLineParser::getFilename() const
{
    return filename_;
}

bool CmdLineParser::result() const
{
    return result_;
}

bool CmdLineParser::dump() const
{
    return dump_;
}

bool CmdLineParser::cleanDump() const
{
    return cleanDump_;
}

bool CmdLineParser::optSize() const
{
    return optSize_;
}

bool CmdLineParser::unroolLoops() const
{
    return unroolLoops_;
}

bool CmdLineParser::unitAtATime() const
{
    return unitAtATime_;
}

bool CmdLineParser::simplifyLibCalls() const
{
    return unitAtATime_;
}

unsigned CmdLineParser::optLevel() const
{
    return optLevel_;
}

llvm::Pass* CmdLineParser::inlinePass() const
{
    return inlinePass_;
}

//------------------------------------------------------------------------------

CmdBase::CmdBase(CmdLineParser& clp)
    : clp_(clp)
{}

//------------------------------------------------------------------------------

DumpCmd::Cmd(CmdLineParser& clp)
    : CmdBase(clp)
{}

void DumpCmd::execute()
{
    clp_.dump_ = true;
}

//------------------------------------------------------------------------------

CleanDumpCmd::Cmd(CmdLineParser& clp)
    : CmdBase(clp)
{}

void CleanDumpCmd::execute()
{
    clp_.cleanDump_ = true;
}

//------------------------------------------------------------------------------

OptLevelCmd::Cmd(CmdLineParser& clp, unsigned optLevel)
    : CmdBase(clp)
    , optLevel_(optLevel)
{}

void OptLevelCmd::execute()
{
    clp_.optLevel_ = optLevel_;

    if (optLevel_ >= 1)
        clp_.unitAtATime_ = true;
    if (optLevel_ >= 2)
        clp_.unroolLoops_ = true;

    delete clp_.inlinePass_;
    if (optLevel_ >= 1)
        clp_.inlinePass_ = llvm::createFunctionInliningPass( optLevel_ > 2 ? 250 : 200 );
}

//------------------------------------------------------------------------------

OptSizeCmd::Cmd(CmdLineParser& clp)
    : CmdBase(clp)
{}

void OptSizeCmd::execute()
{
    clp_.optSize_ = true;
}

//------------------------------------------------------------------------------


} // namespace swift
