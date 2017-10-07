Tinycsockets
============

Tinycsockets is a thin cross-platform socket library written in C. It focuses on a minimal
footprint and on simple usage.

Currently support:
- Windows

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
tinycsocket_free();
```
See the example folder for information of how to use tinycsockets.


MFAQ (Made up Frequently Asked Questions)
------------

- How do I contribute?
  - You can create a merge-request at gitlab.com/dosshell/tinycsockets. Please note that we use
  semi-linear history.

- Why C and not \**random fancy language*\* ?
  - The focus is cross-platform, hardware and OS. Almost all platforms and OS:s have a C-compiler.
  Many scripting language also supports bind to a C ABI.

- Does it support \**random fancy OS*\* ?
  - No, but please make an MR.

- Can I use this under license *X ?
  - No, but there is probably no need for that. This project uses the MIT license, which is very
  permissive license. You can even sell this code if you want. The unwillingness of relicensing is
  of practical reasons, but also because MIT gives us the authors a good liability protection.
