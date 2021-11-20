#!/bin/bash
g++ -m64 -std=c++17 -O2 -c cppSfxr.cpp -o cppSfxr.o
g++ -m64 -std=c++17 -O2 -c libSfxr.cpp -o libSfxr.o
ar rcs cppSfxr.a cppSfxr.o libSfxr.o
