#!/bin/bash 

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

#*******************************************************************************
# vec3add
#*******************************************************************************

for TYPE in \
    uint8 uint16 uint32 uint64 \
    real real64
do
    echo "### runnig vec3add benchmark ###"

    # build file names
    file_swift=benchmark/vec3add/swift/vec3_$TYPE.swift
    file_cpp=benchmark/vec3add/cpp/vec3_$TYPE.cpp
    file_main=benchmark/vec3add/cpp/vec3_main_$TYPE.cpp
    file_h=benchmark/vec3add/cpp/vec3_$TYPE.h

    # substitute TYPE with $TYPE
    sed s/TYPE/${TYPE}/g <benchmark/vec3add/swift/vec3.swift.template >temp;  cat temp > $file_swift;    
    sed s/TYPE/${TYPE}/g <benchmark/vec3add/cpp/vec3.cpp.template >temp;      cat temp > $file_cpp; 
    sed s/TYPE/${TYPE}/g <benchmark/vec3add/cpp/vec3_main.cpp.template >temp; cat temp > $file_main;
    sed s/TYPE/${TYPE}/g <benchmark/vec3add/cpp/vec3.h.template >temp;        cat temp > $file_h;   

    # and compile
    echo compiling file $file_swift
    ./swiftc $file_swift
    echo compiling file $file_cpp and $file_main
    g++ $file_cpp $file_main -O3 -fomit-frame-pointer -funsafe-math-optimizations -o $file_cpp.out

    benchmark $file_swift.out
    cpp=$BENCH

    benchmark $file_cpp.out
    swift=$BENCH

    speedup=$(echo "scale=2; $swift / $cpp" | bc)
    echo "---> speedup: $speedup"
    echo
done

#*******************************************************************************
# vec3cross
#*******************************************************************************

for TYPE in real real64
do
    echo "### runnig vec3cross benchmark ###"

    # build file names
    file_swift=benchmark/vec3cross/swift/vec3_$TYPE.swift
    file_cpp=benchmark/vec3cross/cpp/vec3_$TYPE.cpp
    file_main=benchmark/vec3cross/cpp/vec3_main_$TYPE.cpp
    file_h=benchmark/vec3cross/cpp/vec3_$TYPE.h

    # substitute TYPE with $TYPE
    sed s/TYPE/${TYPE}/g <benchmark/vec3cross/swift/vec3.swift.template >temp;  cat temp > $file_swift;    
    sed s/TYPE/${TYPE}/g <benchmark/vec3cross/cpp/vec3.cpp.template >temp;      cat temp > $file_cpp; 
    sed s/TYPE/${TYPE}/g <benchmark/vec3cross/cpp/vec3_main.cpp.template >temp; cat temp > $file_main;
    sed s/TYPE/${TYPE}/g <benchmark/vec3cross/cpp/vec3.h.template >temp;        cat temp > $file_h;   

    # and compile
    echo compiling file $file_swift
    ./swiftc $file_swift
    echo compiling file $file_cpp and $file_main
    g++ $file_cpp $file_main -O3 -fomit-frame-pointer -funsafe-math-optimizations -o $file_cpp.out

    benchmark $file_swift.out
    cpp=$BENCH

    benchmark $file_cpp.out
    swift=$BENCH

    speedup=$(echo "scale=2; $swift / $cpp" | bc)
    echo "---> speedup: $speedup"
    echo
done

rm temp
