#!/bin/bash
g++ -m64 -s -std=c++17 -O3 -c cppSfxr.cpp -o cppSfxr.o
g++ -m64 -s -std=c++17 -O3 -c libSfxr.cpp -o libSfxr.o
g++ -m64 -s -std=c++17 -O3 -c main.cpp -o main.o
g++ -m64 -s -std=c++17 -O3 -o main.exe cppSfxr.o libSfxr.o main.o
