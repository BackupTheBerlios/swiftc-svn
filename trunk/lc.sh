#!/bin/bash
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
