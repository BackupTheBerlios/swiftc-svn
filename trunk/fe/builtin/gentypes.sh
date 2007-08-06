#!/bin/bash

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

for TYPE in \
    index \
     int8  int16  int32  int64  sat8  sat16 \
    uint8 uint16 uint32 uint64 usat8 usat16
do
    # build file name
    FILE=$TYPE.swift

    # generate remark
    echo "# auto-generated file for the builtin type $TYPE derived from the file int.swift" > $FILE
    echo >> $FILE

    # substitute int with $TYPE
    sed s/int/${TYPE}/g <int.swift >temp # the flag 'g' means: replace globally and not only the first occurance

    # append to the proper file
    cat temp >> $FILE

    # remove temporary file
    rm temp
done

#
# auto generate real types from int.swift
#

for TYPE in real32 real64
do
    # build file name
    FILE=$TYPE.swift

    # generate remark
    echo "# auto-generated file for the builtin type $TYPE derived from the file real.swift" > $FILE
    echo >> $FILE

    # substitute real with $TYPE
    sed s/real/${TYPE}/g <real.swift >temp # the flag 'g' means: replace globally and not only the first occurance

    # append to the proper file
    cat temp >> $FILE

    # remove temporary file
    rm temp
done
