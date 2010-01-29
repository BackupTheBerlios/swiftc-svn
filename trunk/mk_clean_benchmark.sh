#!/bin/bash

for file in vec3add vec3cross matmul
do
    rm benchmark/$file/cpp/*.cpp
    rm benchmark/$file/cpp/*.h
    rm benchmark/$file/cpp/*.out
    rm benchmark/$file/swift/*.asm
    rm benchmark/$file/swift/*.dot
    rm benchmark/$file/swift/*.o
    rm benchmark/$file/swift/*.out
    rm benchmark/$file/swift/*.ssa
    rm benchmark/$file/swift/*.swift
done
