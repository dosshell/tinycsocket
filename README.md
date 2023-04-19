Tinycsocket
============
Crossplatform header-only C socket library.

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

Tinycsocket is a thin cross-platform header-only socket library written in C99. [^1]

It focuses on a minimal footprint, fast compilations and to be intuitive to use.

[^1]: Do not worry about C99 to much. It is compatible with C++, 10+ years old MSVC etc.
If it does not compile or generate warnings in your compiler, please open an issue.

The API is a superset of the BSD sockets API with some differences. All
functions return an error-code. The advantage is that the error handling is
simple to understand and to handle for all platforms. The disadvantage is that
the functions can not be easily integrated in expressions.

See the example folder for information of how to use tinycsocket.

Currently support platforms:
- Windows NT 5.0 SP1 or newer (Windows 2000 SP1 and newer Windows versions)
- POSIX.1-2001 compliant systems (Linux, FreeBSD, Solaris and etc)

Installation instructions
------------
The library supports both header-only and the cmake build system. When using cmake
build system you do not need to define the implementation definition.

### Use the header only
This is how I use it most of the times. Download the latest header from the include directory in this repository.
You find a link in the top of this readme.

You need to define `TINYCSOCKET_IMPLEMENTATION` in exactly one translation unit (c/cpp file) before
including the header file. You can put this in a separate translation unit to improve the already quick recompilation time.

In one translation unit:
```
#define TINYCSOCKET_IMPLEMENTATION
#include "tinycsocket.h"
```

In the others:
```
#include "tinycsocket.h"
```

### I want to use CMake and submodules
If you are using a cmake project, it is recommended to include tinycsocket to
your build system. Add this repo as a submodule and add tinycsocket/src to your
CMakeLists.txt.

```sh
git submodule add https://gitlab.com/dosshell/tinycsocket.git
```

```cmake
add_subdirectory(tinycsocket/src)
target_link_libraries(your_target PRIVATE tinycsocket)
```

You can read more about how to use submodules here: https://git-scm.com/book/en/v2/Git-Tools-Submodules

### I just want the lib files and link by my self
You can also build this project to get a lib directory and an include directory.
Generate a build-system out of tinycsocket with cmake and build the install
target. Don't forget that if you are targeting Windows you also need to link to
wsock32.lib and ws2_32.lib.

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
