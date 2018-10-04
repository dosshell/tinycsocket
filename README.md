Tinycsockets
============

```cpp
  tcs_lib_init();

  tcs_socket listen_socket = TINYCSOCKET_NULLSOCKET;
  tcs_socket accept_socket = TINYCSOCKET_NULLSOCKET;
  tcs_socket client_socket = TINYCSOCKET_NULLSOCKET;

  tcs_simple_listen(&listen_socket, "localhost", "1212", TINYCSOCKET_AF_INET);
  tcs_simple_connect(&client_socket, "localhost", "1212", TINYCSOCKET_AF_INET, TINYCSOCKET_SOCK_STREAM);
  tcs_accept(listen_socket, &accept_socket, NULL, NULL);

  tcs_close(&listen_socket);

  uint8_t recv_buffer[8] = { 0 };
  uint8_t* send_buffer = (uint8_t*)"12345678";

  tcs_send(client_socket, send_buffer, 8, 0, NULL);
  tcs_simple_recv_fixed(accept_socket, recv_buffer, 8);

  memcmp(recv_buffer, send_buffer, 8);

  tcs_close(&client_socket);
  tcs_close(&accept_socket);

  tcs_lib_free();
```

Tinycsockets is a thin cross-platform socket library written in C99. It focuses on a minimal
footprint, fast compilation times and cross-platform. The API is similar to BSD sockets with some
differences. All functions return an error-code. The advantage is that the error handling is simple
to understand and to handle for all plattforms. The disadvantage is that the functions can not be
easily integrated in expressions.

See the example folder for information of how to use tinycsockets.

Currently support plattforms:
- Windows 2000 SP1 and newer (XP, Server 2003, Vista, Server 2008, 7, 8, 8.1 and 10)
- Most POSIX-like systems (for example linux)

Installation instructions
------------

Copy the src folder and add all the files to your project. If you are deploying against Windows
you also need to link to wsock32.lib ws2_32.lib.

However, if you are using cmake you can just add the src folder as a subdir,
`add_subdirectory (tinycsockets/src)` and you are done.

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
  - Yes, you need to initializ and free tinycsockets to support all plattforms. You also need to
  free it as many times as you initialize it. Eg, you can use RAII to initialize and free it.
