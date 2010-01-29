#!/bin/bash 

NUM_ITER=5
BENCH=0
ALL_TYPES=uint8\ uint16\ uint32\ uint64\ real\ real64
REAL_TYPES=real\ real64

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

build_and_benchmark () {
    echo
    echo "### runnig $2 benchmark ###"
    echo

    for TYPE in $1
    do
        # build file names
        file_swift=benchmark/$2/swift/$3_$TYPE.swift
        file_cpp=benchmark/$2/cpp/$3_$TYPE.cpp
        file_main=benchmark/$2/cpp/$3_main_$TYPE.cpp
        file_h=benchmark/$2/cpp/$3_$TYPE.h

        # substitute TYPE with $TYPE
        sed s/TYPE/${TYPE}/g <benchmark/$2/swift/$3.swift.template >temp;  cat temp > $file_swift;    
        sed s/TYPE/${TYPE}/g <benchmark/$2/cpp/$3.cpp.template >temp;      cat temp > $file_cpp; 
        sed s/TYPE/${TYPE}/g <benchmark/$2/cpp/$3_main.cpp.template >temp; cat temp > $file_main;
        sed s/TYPE/${TYPE}/g <benchmark/$2/cpp/$3.h.template >temp;        cat temp > $file_h;   

        # substitute ZERO
        if [[ $TYPE == real ]]; then
            sed s/ZERO/0.0/g  <$file_swift >temp; cat temp > $file_swift;    
        elif [[ $TYPE == real64 ]]; then
            sed s/ZERO/0.0q/g <$file_swift >temp; cat temp > $file_swift;    
        fi

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
}

build_and_benchmark "$ALL_TYPES" vec3add vec3
build_and_benchmark "$REAL_TYPES" vec3cross vec3
build_and_benchmark "$REAL_TYPES" matmul mat
build_and_benchmark "$REAL_TYPES" ifelse vec3
