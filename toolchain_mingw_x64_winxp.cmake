# Used when cross-compiling to windows
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_C_COMPILER "/usr/bin/x86_64-w64-mingw32-gcc-posix")
set(CMAKE_CXX_COMPILER "/usr/bin/x86_64-w64-mingw32-g++-posix")
set(CMAKE_AR "/usr/bin/x86_64-w64-mingw32-gcc-ar-posix")
set(CMAKE_EXE_LINKER_FLAGS "-static" CACHE STRING "" FORCE)
add_definitions(-DNTDDI_VERSION=0x05020000 -D_WIN32_WINNT=0x0502 -DWINVER=0x0502)
