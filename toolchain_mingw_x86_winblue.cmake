# Used when cross-compiling to windows
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER "/usr/bin/i686-w64-mingw32-gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/i686-w64-mingw32-g++")
set(CMAKE_AR "/usr/bin/i686-w64-mingw32-ar")
add_definitions(-DNTDDI_VERSION=0x06030000 -D_WIN32_WINNT=0x0603 -DWINVER=0x0603)
