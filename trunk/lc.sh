#!/bin/bash
make clean
rm CMakeFiles -rf
rm contrib/*.asm
rm doc/*
sloccount .
