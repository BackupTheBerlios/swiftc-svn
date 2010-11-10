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

echo -e '\E[32mAuto-generating built-in types \E[37m'

# build appropriate array for bitcasts
int_to_real[ 0]=""                                               #index
int_to_real[ 1]="def simd bitcast_to_real() -> real result; end"     #int
int_to_real[ 2]="def simd bitcast_to_real() -> real result; end"     #uint

int_to_real[ 3]=""                                               #int8
int_to_real[ 4]=""                                               #int16
int_to_real[ 5]="def simd bitcast_to_real32() -> real32 result; end" #int32
int_to_real[ 6]="def simd bitcast_to_real64() -> real64 result; end" #int64
int_to_real[ 7]=""                                               #sat8
int_to_real[ 8]=""                                               #sat16

int_to_real[ 9]=""                                               #uint8
int_to_real[10]=""                                               #uint16
int_to_real[11]="def simd bitcast_to_real32() -> real32 result; end" #uint32
int_to_real[12]="def simd bitcast_to_real64() -> real64 result; end" #uint64
int_to_real[13]=""                                               #usat8
int_to_real[14]=""                                               #usat16

i=0
for TYPE in \
    index int uint \
     int8  int16  int32  int64  sat8  sat16 \
    uint8 uint16 uint32 uint64 usat8 usat16
do
    # build file name
    FILE=$TYPE.swift

    # echo "    Generating $FILE"

    # generate remark
    echo "# auto-generated file for the built-in type $TYPE derived from the file int.swift" > $FILE
    echo >> $FILE

    # substitute int with $TYPE
    sed s/INT/${TYPE}/g <int_template.swift >temp
    # substitute for proper bitcast
    sed s/BITCAST/"${int_to_real[i]}"/g <temp >temp2

    # append to the proper file
    cat temp2 >> $FILE

    # remove temporary file
    rm temp temp2

    let i=i+1
done

#
# auto generate real types from real.swift
#

# build appropriate array for bitcasts
real_to_int[0]=int
real_to_int[1]=int32
real_to_int[2]=int64

i=0
for TYPE in real real32 real64
do
    # build file name
    FILE=$TYPE.swift

    # echo "    Generating $FILE"

    # generate remark
    echo "# auto-generated file for the built-in type $TYPE derived from the file real.swift" > $FILE
    echo >> $FILE

    # substitute real with $TYPE
    sed s/REAL/${TYPE}/g <real_template.swift >temp
    # substitute for proper bitcast
    sed s/INT/${real_to_int[i]}/g <temp >temp2

    # append to the proper file
    cat temp2 >> $FILE

    # remove temporary files
    rm temp temp2

    # inc loop counter
    let i=i+1
done
