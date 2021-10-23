#!/bin/bash
g++ -m64 -std=c++17 -O2 -c cppSfxr.cpp -o cppSfxr.o
ar rvs cppSfxr.a cppSfxr.o
