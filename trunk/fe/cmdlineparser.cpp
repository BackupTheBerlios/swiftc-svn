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

#include <iostream>

namespace swift {

std::string CmdLineParser::usage_ = std::string("Usage: swiftc [options] file");
CmdLineParser::Cmds CmdLineParser::cmds_ = CmdLineParser::Cmds();

CmdLineParser::CmdLineParser(int argc, char** argv)
    : argc_(argc)
    , argv_(argv)
    , filename_(0)
    , result_(true)
    , dump_(false)
{
    // create command data structure
    cmds_["--dump"] = new DumpCmd(*this);

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

} // namespace swift
