#!/bin/bash
make clean
rm CMakeFiles -rf
rm contrib/*.asm
rm test/*.asm
rm doc/*
sloccount .
