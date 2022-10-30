@ECHO OFF
IF NOT EXIST build\NUL MKDIR build
cmake -S . -B build
cmake --build build --config Release
cmake --install build
