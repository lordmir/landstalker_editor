@ECHO OFF

IF NOT EXIST build\NUL MKDIR build
cmake -S . -B build/
