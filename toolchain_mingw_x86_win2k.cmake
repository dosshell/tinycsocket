# Used when cross-compiling to windows
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER "/usr/bin/i686-w64-mingw32-gcc-posix")
set(CMAKE_CXX_COMPILER "/usr/bin/i686-w64-mingw32-g++-posix")
set(CMAKE_AR "/usr/bin/i686-w64-mingw32-gcc-ar-posix")
set(CMAKE_EXE_LINKER_FLAGS "-static" CACHE STRING "" FORCE)
add_definitions(-DNTDDI_VERSION=0x05000000 -D_WIN32_WINNT=0x0500 -DWINVER=0x0500)
