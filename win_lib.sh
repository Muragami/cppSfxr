#!/bin/bash
g++ -m64 -s -std=c++17 -O2 -c cppSfxr.cpp -o cppSfxr.o
g++ -m64 -s -std=c++17 -O2 -c libSfxr.cpp -o libSfxr.o
ar rvs cppSfxr.a cppSfxr.o libSfxr.cpp
