Tinycsockets
============

Tinycsockets is a thin cross-platform socket library written in C. It focuses on a minimal
footprint, cross-platform and on simple usage.

Currently support plattforms:
- Windows 2000 SP1 and newer (XP, Server 2003, Vista, Server 2008, 7, 8, 8.1 and 10)
- Windows IoT

Usage
------------
```C
#include <tinycsocket.h>

TinyCSocketCtx* socketCtx = NULL;
tinycsocket_create_socket(&socketCtx);

tinycsocket_connect(socketCtx, "localhost", "1212");

const char msg[] = "hello world";
tinycsocket_send_data(socketCtx, msg, sizeof(msg));

tinycsocket_close_socket(&socketCtx);
```
See the example folder for information of how to use tinycsockets.


MFAQ (Made up Frequently Asked Questions)
------------

- How do I contribute?
  - You can create a merge-request at https://gitlab.com/dosshell/tinycsockets. Please note that we
  use semi-linear history.

- Why C and not \**random fancy language*\* ?
  - The focus is cross-platform (hardware and OS). Almost all common platforms and OS:s have a
  C-compiler. Many scripting languages also support bindings to a C ABI.

- Does it support \**random fancy OS not in the list*\* ?
  - No, but please make a merge-request.

- Can I use this under license X ?
  - No, but there is probably no need for that. This project uses the MIT license, which is a very
  permissive license. You can even sell this code if you want to. The unwillingness of relicensing
  is not only of practical reasons, but also because MIT gives us the authors a good liability
  protection.

- I don't get it. Do I need to initliaze tinycsockets or not?
  - No, it will automatically initialize the library when you create your first socket and then
  free it when you destroy your last one. However, if you create and destroy all your sockets
  frequently it is recommended to use the init and the free functions to prevent reinitializing
  the library every time you create a new first socket. Note that not all plattforms need
  initializing and will therefor not be affected.