#!/bin/bash

pwd
./swift test/fibonacci.swift
gcc -c test/fibonacci.c -o test/fibonacci.o
as test/fibonacci.swift.asm -o test/fibonacci.swift.o
gcc test/fibonacci.o test/fibonacci.swift.o -o test/fib
./test/fib $1
