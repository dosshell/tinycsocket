Tinycsockets
============

Tinycsockets is a thin cross-platform socket library written in C99. It focuses on a minimal
footprint, fast compilation times and cross-platform. The API is similar to BSD sockets with some
differences. All functions return an error-code. The advantage is that the error handling is simple
to understand and to handle. The disadvantage is that the functions can not be easily integrated in
expressions.

Currently support plattforms:
- Windows 2000 SP1 and newer (XP, Server 2003, Vista, Server 2008, 7, 8, 8.1 and 10)
- Windows IoT
- MinGW32 and MinGW64
- MSYS2 (msys/gcc)
- POSIX (Linux, Android, FreeBSD, OpenBSD, NetBSD, Solaris, etc)

Usage
------------
TCP Client Example
```C
#include <tinycsocket.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int show_error(const char* error_text)
{
    fprintf(stderr, "%s", error_text);
    return -1;
}

int main(int argc, const char* argv[])
{
    if (tinycsocket_init() != TINYCSOCKET_SUCCESS)
        return show_error("Could not init tinycsockets");

    TinyCSocketCtx client_socket = TINYCSOCKET_NULLSOCKET;

    if (tinycsocket_socket(&client_socket, TINYCSOCKET_AF_INET, TINYCSOCKET_SOCK_STREAM, TINYCSOCKET_IPPROTO_TCP) != TINYCSOCKET_SUCCESS)
        return show_error("Could not create a socket");

    struct TinyCSocketAddressInfo* address_info = NULL;
    if (tinycsocket_getaddrinfo("localhost", "1212", NULL, &address_info) != TINYCSOCKET_SUCCESS)
        return show_error("Could not resolve host");

    bool isConnected = false;
    for (struct TinyCSocketAddressInfo* address_iterator = address_info; address_iterator != NULL; address_iterator = address_iterator->ai_next)
    {
        if (tinycsocket_connect(client_socket, address_iterator->ai_addr, address_iterator->ai_addrlen) == TINYCSOCKET_SUCCESS)
        {
            isConnected = true;
            break;
        }
    }

    if (!isConnected)
        return show_error("Could not connect to server");

    char msg[] = "hello world\n";
    tinycsocket_send(client_socket, msg, sizeof(msg), 0, NULL);

    uint8_t recv_buffer[1024];
    size_t bytes_recieved = 0;
    if (tinycsocket_recv(client_socket, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, &bytes_recieved) != TINYCSOCKET_SUCCESS)
        return show_error("Could not recieve data");

    // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in recieve
    recv_buffer[bytes_recieved] = '\0';
    printf("recieved: %s\n", recv_buffer);

    if (tinycsocket_shutdown(client_socket, TINYCSOCKET_BOTH) != TINYCSOCKET_SUCCESS)
        return show_error("Could not shutdown socket");

    if (tinycsocket_closesocket(&client_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not close the socket");

    if (tinycsocket_free() != TINYCSOCKET_SUCCESS)
        return show_error("Could not free tinycsockets");
}

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
  - Yes, you need to initializ and free tinycsockets to support all plattforms. You also need to
  free it as many times as you initialize it. Eg, you can use RAII to initialize and free it, and
  you do now have to care about if it is already initialized.
