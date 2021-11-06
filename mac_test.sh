#!/bin/bash
g++ -std=c++17 -O3 -c cppSfxr.cpp -o cppSfxr.o
g++ -std=c++17 -O3 -c main.cpp -o main.o
g++ -std=c++17 -O3 -o main cppSfxr.o main.o
