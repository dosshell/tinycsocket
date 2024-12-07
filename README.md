# Tinycsocket

Crossplatform header-only low-level socket C library.

Repository: https://gitlab.com/dosshell/tinycsocket/

Online documentation: https://tinycsocket.readthedocs.io/

Download: [latest](https://gitlab.com/dosshell/tinycsocket/-/raw/master/include/tinycsocket.h) 

```cpp
// Put the following define in exactly one of your c/cpp files before including.
#define TINYCSOCKET_IMPLEMENTATION
#include "tinycsocket.h"

int main(int argc, const char* argv[])
{
    tcs_lib_init();

    TcsSocket client_socket = TCS_NULLSOCKET;
    tcs_create(&client_socket, TCS_TYPE_TCP_IP4);
    tcs_connect(client_socket, "example.com", 80);

    uint8_t send_buffer[] = "GET / HTTP/1.1\nHost: example.com\n\n";
    tcs_send(client_socket, send_buffer, sizeof(send_buffer), TCS_MSG_SENDALL, NULL);

    uint8_t recv_buffer[8192] = {0};
    size_t bytes_received = 0;
    tcs_receive(client_socket, recv_buffer, 8192, TCS_NO_FLAGS, &bytes_received);

    tcs_shutdown(client_socket, TCS_SD_BOTH);
    tcs_destroy(&client_socket);

    tcs_lib_free();
}
```

Tinycsocket is a thin cross-platform header-only low-level socket library written in C99 (with C++ compatibility).

It focuses on a minimal footprint to get crossplatform sockets, fast compilations and to be intuitive to use.

When working with sockets, you notice that almost all of the popular desktop and server operating systems implement "BSD sockets".
But the implementations differs and they have their own extensions. If you make `#ifdef` for all differences (which many are non obvious),
make a general error handling and implement the different poll systems... you get tinycsocket! Oh, and also add some common low-level
functions outside BSD sockets, such as list all available network interfaces on your machine.

The API is a superset of the BSD sockets API with some differences. All
functions return an error-code. The advantage is that the error handling is
simple to understand and to handle for all platforms. The disadvantage is that
the functions can not be easily integrated in expressions. Note that the `TcsSocket` type
is your native OS socket type. You can call any OS specific code that expects native sockets or vice versa.

This library does NOT provide any event system or any high level abstractions.

See the example folder for information of how to use tinycsocket.

Currently support platforms:
- Windows NT 5.0 SP1 (Windows 2000 SP1) or newer.
- POSIX.1-2001 compliant systems (Linux, FreeBSD and Solaris etc.)

Note that MacOS is currently not tested or officially supported (but it "should" work (TM)).

# Installation instructions

The library supports both header-only and the cmake build system. When using cmake
build system you do not need to define the implementation definition.

## Use the header only
Download the latest header from the include directory in this repository.

Download [latest](https://gitlab.com/dosshell/tinycsocket/-/raw/master/include/tinycsocket.h)

You need to define `TINYCSOCKET_IMPLEMENTATION` in exactly one translation unit (c/cpp file) before
including the header file. You can put this in a separate translation unit to not pollute your namespace with OS socket symbols.

In exactly one translation unit:
```cpp
#define TINYCSOCKET_IMPLEMENTATION
#include "tinycsocket.h"
```

In the others:
```cpp
#include "tinycsocket.h"
```

## I want to use CMake
If you are using cmake version 3.11 or newer, you can easily add tinycsocket to your build system.
This is how I use it most of the times.

Here is an example of a CMakeLists.txt file that let you link to tinycsocket:
```cmake
include(FetchContent)
FetchContent_Declare(tinycsocket
    GIT_REPOSITORY https://gitlab.com/dosshell/tinycsocket.git
    GIT_TAG v0.3  # Use the latest version tag, or master if you want the break your build system in the future
)
FetchContent_GetProperties(tinycsocket)
if(NOT tinycsocket_POPULATED)
    FetchContent_Populate(tinycsocket)
    add_subdirectory(${tinycsocket_SOURCE_DIR} ${tinycsocket_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# You can now link to tinycsocket from your target
add_executable(my-cmake-project main.cpp)
target_link_libraries(my-cmake-project PRIVATE tinycsocket)
```
And that's it. You can now include tinycsocket.h in your target and start using it.

## I just want the lib files and link by my self
You can also build this project to get a lib directory and an include directory.
Generate a build-system out of tinycsocket with cmake and build the install
target. Don't forget that if you are targeting Windows you also need to link to
wsock32.lib, ws2_32.lib and iphlpapi.lib.

The following commands will create these include- and lib folders in a folder
named install:

```sh
git clone https://gitlab.com/dosshell/tinycsocket.git
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:Path=../install ../tinycsocket
cmake --build . --target INSTALL --config Release
```
You can now remove the build directory and the tinycsocket directory if you
like.

# Tested platforms in CI

These platforms are always tested for every commit.

------------
| OS                       | Compiler                    | Comments     |
|--------------------------|-----------------------------|--------------|
| Windows 2022             | MSVC v19.40                 | WINVER=0x502 |
| Windows 2022             | x86_64-w64-mingw32-gcc v9.3 | WINVER=0x502 |
| Windows 2022             | x86_64-w64-mingw32-g++ v9.3 | WINVER=0x502 |
| Windows 2022             | i686-w64-mingw32-gcc v9.3   | WINVER=0x501 |
| Windows 2022             | i686-w64-mingw32-g++ v9.3   | WINVER=0x501 |
| Windows 2022             | x86_64-w64-mingw32-gcc v9.3 | WINVER=0x603 |
| Windows 2022             | x86_64-w64-mingw32-g++ v9.3 | WINVER=0x603 |
| Windows 2022             | i686-w64-mingw32-gcc v9.3   | WINVER=0x603 |
| Windows 2022             | i686-w64-mingw32-g++ v9.3   | WINVER=0x603 |
| Linux Alpine 3.12 x86-64 | gcc v9.3                    |              |
| Linux Alpine 3.12 x86-64 | g++ v9.3                    |              |
| Linux Alpine 3.12 x86    | gcc v9.3                    |              |
| Linux Alpine 3.12 x86    | g++ v9.3                    |              |
