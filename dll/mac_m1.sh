#!/bin/bash
g++ -m64 -fPIC -std=c++17 -O3 -c ../cppSfxr.cpp -o ../cppSfxr.o
g++ -m64 -fPIC -std=c++17 -O3 -c ../libSfxr.cpp -o ../libSfxr.o
g++ -m64 -fPIC -std=c++17 -O3 -c wrap.cpp -o wrap.o
g++ -m64 -dynamiclib -fPIC -std=c++17 -O3 -o m1cppSfxr.dylib ../cppSfxr.o ../libSfxr.o wrap.o
