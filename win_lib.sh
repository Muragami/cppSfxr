#!/bin/bash -x
g++ -m64 -s -std=c++17 -O3 -c cppSfxr.cpp -o cppSfxr.o
g++ -m64 -s -std=c++17 -O3 -c libSfxr.cpp -o libSfxr.o
ar rcs cppSfxr.a cppSfxr.o libSfxr.o
