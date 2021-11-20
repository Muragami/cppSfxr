# cppSfxr, a PSG C++ library

**cppSfxr** is a compact Procedural Sound Generation (PSG) library in c++ with both
a C dll wrapper to provide modular loading, and an [luajit](https://luajit.org)
FFI wrapper around that to allow it's use in the [LÖVE](https://love2d.org)
game engine. The core sound generation engine was originally written by
[DrPetter](https://www.drpetter.se/project_sfxr.html) for the 10th Ludum Dare competition.

It is purely a sound generation library, and has no connections to any audio
rendering engines or systems. The advantage of this is that it is easily compiled
anywhere little endian systems allow. A precompiled dynamic link library is provided
for the following platforms (in th dll folder):

* **Windows**: x64cppSfxr.dll, compiled with MSVC, for any Intel AMD64 machine
* **Mac OS X**: x64cppSfxr.dylib, compiled with gcc/clang via brew, for Intel macs.
* **Mac OS X (M1)**: m1cppSfxr.dylib, compiled with gcc/clang via brew, for new Apple Silicon macs.
* **Debian**: x64cppSfxr.so, compiled with gcc, for any Intel AMD64 machine.

While not provided prebuilt, it is also trivial to compile into a small static library
you can link into your application. The code has been compiled against iso:std++17 standard.
