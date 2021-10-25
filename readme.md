# cppSfxr, a PSG C++ library

cppSfxr is a compact Procedural Sound Generation (PSG) library in c++ with both
a C dll wrapper to provide modular loading, and an [luajit](https://luajit.org)
FFI wrapper around that to allow it's use in the [LÖVE](https://love2d.org)
game engine. The core sound generation engine was originally written by
[DrPetter]https://www.drpetter.se/project_sfxr.html() for the 10th Ludum Dare competition.
