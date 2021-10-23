#!/bin/bash
g++ -m64 -s -std=c++17 -O2 -c cppSfxr.cpp -o cppSfxr.o
g++ -m64 -s -std=c++17 -O2 -c main.cpp -o main.o
g++ -m64 -s -std=c++17 -O2 -o main.exe cppSfxr.o main.o
