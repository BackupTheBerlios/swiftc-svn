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

# This script generates the following builtin types from int.swift:
#
# index
#
# int8
# int16
# int32
# int64
# sat8
# sat16
#
# uint
# uint8
# uint16
# uint32
# uint64
# usat8
# usat16


# The following types shall be generated from real.swift:
# real32
# real64

#
# auto generate int types from int.swift
#

echo -e '\E[32mAutogenerating builtin types \E[37m'

for TYPE in \
    index int uint \
     int8  int16  int32  int64  sat8  sat16 \
    uint8 uint16 uint32 uint64 usat8 usat16
do
    # build file name
    FILE=$TYPE.swift

    # echo "    Generating $FILE"

    # generate remark
    echo "# auto-generated file for the builtin type $TYPE derived from the file int.swift" > $FILE
    echo >> $FILE

    # substitute int with $TYPE
    sed s/INT/${TYPE}/g <int_template.swift >temp

    # append to the proper file
    cat temp >> $FILE

    # remove temporary file
    rm temp
done

#
# autogenerate real types from real.swift
#

for TYPE in real real32 real64
do
    # build file name
    FILE=$TYPE.swift

    # echo "    Generating $FILE"

    # generate remark
    echo "# autogenerated file for the builtin type $TYPE derived from the file real.swift" > $FILE
    echo >> $FILE

    # substitute real with $TYPE
    sed s/REAL/${TYPE}/g <real_template.swift >temp

    # append to the proper file
    cat temp >> $FILE

    # remove temporary file
    rm temp
done