#!/bin/bash
g++ -m64 -fPIC -std=c++17 -O2 -c ../cppSfxr.cpp -o ../cppSfxr.o
g++ -m64 -fPIC -std=c++17 -O2 -c wrap.cpp -o wrap.o
g++ -m64 -shared -fPIC -std=c++17 -O2 -o cppSfxr.dll ../cppSfxr.o wrap.o
