#!/bin/bash 

# ******************************************************************************
# vec3add
# ******************************************************************************

NUM_ITER=5
BENCH=0

benchmark () {
    echo benchmarking $1

    for ((BENCH=0, i=0; i < $NUM_ITER; i++))
    do
        echo -n .
        BENCH=$(echo "scale=2; $BENCH + $(/usr/bin/time -f "%U" "$1" 2>&1)" | bc)
    done

    BENCH=$(echo "scale=2; $BENCH/$NUM_ITER" | bc)
    echo " -> $BENCH"
}

for TYPE in \
    uint8 uint16 uint32 uint64 \
    real real64
do
    # build file names
    FILE=benchmark/vec3add/swift/vec3_$TYPE.swift
    FILEcpp=benchmark/vec3add/cpp/vec3_$TYPE.cpp
    FILEmain=benchmark/vec3add/cpp/vec3_main_$TYPE.cpp
    FILEh=benchmark/vec3add/cpp/vec3_$TYPE.h

    # substitute TYPE with $TYPE
    sed s/TYPE/${TYPE}/g <benchmark/vec3add/swift/vec3.swift.template >temp;  cat temp > $FILE;    
    sed s/TYPE/${TYPE}/g <benchmark/vec3add/cpp/vec3.cpp.template >temp;      cat temp > $FILEcpp; 
    sed s/TYPE/${TYPE}/g <benchmark/vec3add/cpp/vec3_main.cpp.template >temp; cat temp > $FILEmain;
    sed s/TYPE/${TYPE}/g <benchmark/vec3add/cpp/vec3.h.template >temp;        cat temp > $FILEh;   

    # and compile
    echo compiling file $FILE
    ./swiftc $FILE
    echo compiling file $FILEcpp and $FILEmain
    g++ $FILEcpp $FILEmain -O3 -fomit-frame-pointer -funsafe-math-optimizations -o $FILEcpp.out

    benchmark $FILE.out
    cpp=$BENCH

    benchmark $FILEcpp.out
    swift=$BENCH

    speedup=$(echo "scale=2; $swift / $cpp" | bc)
    echo "---> speedup: $speedup"
    echo
done

rm temp
