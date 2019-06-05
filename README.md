Tinycsockets
============

```cpp
#include "tinycsocket.h"

int main(int argc, const char* argv[])
{
  tcs_lib_init();

  tcs_socket listen_socket = TCS_NULLSOCKET;
  tcs_socket accept_socket = TCS_NULLSOCKET;
  tcs_socket client_socket = TCS_NULLSOCKET;

  tcs_simple_create_and_listen(&listen_socket, "localhost", "1212", TCS_AF_INET);
  
  tcs_create(&socket, TCS_AF_INET, TCS_SOCK_STREAM, TCS_IPPROTO_TCP);
  tcs_simple_connect(&client_socket, "localhost", "1212", TCS_AF_INET, TCS_SOCK_STREAM);
  
  tcs_accept(listen_socket, &accept_socket, NULL, NULL);
  tcs_close(&listen_socket);

  uint8_t recv_buffer[8] = { 0 };
  uint8_t* send_buffer = (uint8_t*)"12345678";

  tcs_simple_send_all(client_socket, send_buffer, 8, 0);
  tcs_simple_recv_all(accept_socket, recv_buffer, 8);

  memcmp(recv_buffer, send_buffer, 8);

  tcs_close(&client_socket);
  tcs_close(&accept_socket);

  tcs_lib_free();
  
  return 0;
}
```

Tinycsockets is a thin cross-platform socket library written in C99. It focuses
on a minimal footprint, cross-platform and to also provide simple lowlevel utils
(for example tcs_simple_connect(...) which resolves and connects to a hostname).
The API is similar to BSD sockets with some differences. All functions return an
error-code. The advantage is that the error handling is simple to understand and
to handle for all plattforms. The disadvantage is that the functions can not be
easily integrated in expressions.

See the example folder for information of how to use tinycsockets.

Currently support plattforms:
- Windows 2000 SP1 and newer (XP, Server 2003, Vista, Server 2008, 7, 8, 8.1 and 10)
- POSIX.1-2001 systems

Installation instructions
------------

If you are using a cmake project, it is recommended to include tinycsockets to
your build system. Clone this repo and add `add_subdirectory (tinycsockets/src)`
to your CMakeLists.txt.

```cmake
...
add_subdirectory(tinycsockets/src)
target_link_libraries(your_target PRIVATE tinycsockets)
```


You can also build this project to get a lib directory and an include directoy.
Generate a build-system out of tinycsockets with cmake and build the install
target. Don't forget that if you are targeting Windows you also need to link to
wsock32.lib and ws2_32.lib.

The following commands will create these include- and lib folders in a folder named
install:

```
git clone git@gitlab.com:dosshell/tinycsockets.git
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:Path=../install ../tinycsockets
cmake --build . --target INSTALL --config Release
```
You can now remove the build directory and the tinycsockets directory if you
like.
