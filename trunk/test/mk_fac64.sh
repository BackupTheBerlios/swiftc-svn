#!/bin/bash

./swift test/fac64.swift
gcc -c test/fac64.c -o test/fac64.o
as test/fac64.swift.asm -o test/fac64.swift.o
gcc test/fac64.o test/fac64.swift.o -o test/fac64
./test/fac64 $1
