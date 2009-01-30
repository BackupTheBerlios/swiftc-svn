#!/bin/bash

# Swift compiler framework
# Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
# 
# This framework is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 3 as published by the Free Software Foundation.
# 
# This framework is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this framework; see the file LICENSE. If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

make clean
cd fe/builtin/
./cleantypes.sh
cd ../..
rm CMakeFiles -rf
pwd
rm contrib/*.swift.*
rm test/*.swift.*
rm doc/*
rm calls mem_leaks tags
rm cmake_install.cmake
rm CMakeCache.txt
rm CMakeFiles -rf

sloccount .
