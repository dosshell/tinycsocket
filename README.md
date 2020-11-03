Tinycsocket
============
Crossplatform C socket library.

Online documentation: https://tinycsocket.readthedocs.io/

```cpp
#include "tinycsocket.h"

int main(int argc, const char* argv[])
{
    tcs_lib_init();

    TcsSocket client_socket = TCS_NULLSOCKET;
    tcs_simple_create_and_connect(&client_socket, "example.com", "80", TCS_AF_ANY);

    uint8_t send_buffer[] = "GET / HTTP/1.1\nHost: example.com\n\n";
    tcs_simple_send_all(client_socket, send_buffer, sizeof(send_buffer), 0);

    uint8_t recv_buffer[8192] = {0};
    size_t bytes_received = 0;
    tcs_recv(client_socket, recv_buffer, 8192, 0, &bytes_received);
    tcs_close(&client_socket);

    tcs_lib_free();
}
```

Tinycsocket is a thin cross-platform socket library written in C99. It focuses
on a minimal footprint, cross-platform and to also provide simple lowlevel utils
(for example tcs_simple_create_and_connect(...) which resolves and connects to a
hostname). The API is similar to BSD sockets with some differences. All functions
return an error-code. The advantage is that the error handling is simple to
understand and to handle for all plattforms. The disadvantage is that the functions
can not be easily integrated in expressions.

See the example folder for information of how to use tinycsocket.

Currently support plattforms:
- Windows 2000 SP1 and newer (XP, Server 2003, Vista, Server 2008, 7, 8, 8.1 and 10)
- POSIX.1-2001 systems

Installation instructions
------------

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
You can also build this project to get a lib directory and an include directoy.
Generate a build-system out of tinycsocket with cmake and build the install
target. Don't forget that if you are targeting Windows you also need to link to
wsock32.lib and ws2_32.lib.

The following commands will create these include- and lib folders in a folder named
install:

```sh
git clone https://gitlab.com/dosshell/tinycsocket.git
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:Path=../install ../tinycsocket
cmake --build . --target INSTALL --config Release
```
You can now remove the build directory and the tinycsocket directory if you
like.
