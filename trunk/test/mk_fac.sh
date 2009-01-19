#!/bin/bash

./swift test/fac.swift
gcc -c test/fac.c -o test/fac.o
as test/fac.swift.asm -o test/fac.swift.o
gcc test/fac.o test/fac.swift.o -o test/fac
./test/fac $1
