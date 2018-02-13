/*
 * Copyright 2018 Markus Lindelöw
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TINY_C_SOCKETS_H_
#define TINY_C_SOCKETS_H_

#include <stddef.h>
#include <stdint.h>

#if defined(WIN32) || defined(__MINGW32__)
#define TINYCSOCKET_USE_WIN32_IMPL
#elif defined(__linux__) || defined(__sun) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__APPLE__) || defined(__MSYS__)
#define TINYCSOCKET_USE_POSIX_IMPL
#else
#pragma message("Warning: Unknown OS, trying POSIX")
#define TINYCSOCKET_USE_POSIX_IMPL
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(TINYCSOCKET_USE_WIN32_IMPL)
#include <basetsd.h>
typedef UINT_PTR tcs_socket;
typedef int socklen_t;

struct tcs_addrinfo
{
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    char* ai_canonname;
    struct tcs_sockaddr* ai_addr;
    struct tcs_addrinfo* ai_next;
};

#elif defined(TINYCSOCKET_USE_POSIX_IMPL)
typedef int tcs_socket;
typedef int socklen_t;

struct tcs_addrinfo
{
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    struct tcs_sockaddr* ai_addr;
    char* ai_canonname;
    struct tcs_addrinfo* ai_next;
};

#endif

// TODO: This needs to be plattform specific
// This should work on linux and windows for now
// Invistagte if MacOS/BSD uses uint8_t as sa_family_t

typedef unsigned short int sa_family_t;

#define _SS_MAXSIZE__ 128
#define _SS_ALIGNSIZE__ (sizeof(int64_t))

#define _SS_PAD1SIZE__ (_SS_ALIGNSIZE__ - sizeof(sa_family_t))
#define _SS_PAD2SIZE__ (_SS_MAXSIZE__ - (sizeof(sa_family_t) + _SS_PAD1SIZE__ + _SS_ALIGNSIZE__))

struct tcs_sockaddr
{
    sa_family_t ss_family;
    char __ss_pad1[_SS_PAD1SIZE__];
    int64_t __ss_align;
    char __ss_pad2[_SS_PAD2SIZE__];
};

extern const tcs_socket TINYCSOCKET_NULLSOCKET;

// TODO: Problem with optimizing when they are in another translation unit? LTO?

// Domain
extern const int TINYCSOCKET_AF_INET;

// Type
extern const int TINYCSOCKET_SOCK_STREAM;
extern const int TINYCSOCKET_SOCK_DGRAM;

// Protocol
extern const int TINYCSOCKET_IPPROTO_TCP;
extern const int TINYCSOCKET_IPPROTO_UDP;

// Flags
extern const int TINYCSOCKET_AI_PASSIVE;

// Backlog
extern const int TINYCSOCKET_BACKLOG_SOMAXCONN;

// How
extern const int TINYCSOCKET_SD_RECIEVE;
extern const int TINYCSOCKET_SD_SEND;
extern const int TINYCSOCKET_SD_BOTH;

// Socket options
extern const int TINYCSOCKET_SO_REUSEADDR;

// Return codes
static const int TINYCSOCKET_SUCCESS = 0;
static const int TINYCSOCKET_ERROR_UNKNOWN = -1;
static const int TINYCSOCKET_ERROR_MEMORY = -2;
static const int TINYCSOCKET_ERROR_INVALID_ARGUMENT = -3;
static const int TINYCSOCKET_ERROR_KERNEL = -4;
static const int TINYCSOCKET_ERROR_ADDRESS_LOOKUP_FAILED = -5;
static const int TINYCSOCKET_ERROR_CONNECTION_REFUSED = -6;
static const int TINYCSOCKET_ERROR_NOT_INITED = -7;
static const int TINYCSOCKET_ERROR_TIMED_OUT = -8;
static const int TINYCSOCKET_ERROR_NOT_IMPLEMENTED = -9;
static const int TINYCSOCKET_ERROR_NOT_CONNECTED = -10;
static const int TINYCSOCKET_ERROR_ILL_FORMED_MESSAGE = -11;

int tcs_lib_init();

int tcs_lib_free();

int tcs_create(tcs_socket* socket_ctx, int domain, int type, int protocol);

int tcs_bind(tcs_socket socket_ctx,
                const struct tcs_sockaddr* address,
                socklen_t address_length);

int tcs_connect(tcs_socket socket_ctx,
                const struct tcs_sockaddr* address,
                socklen_t address_length);

int tcs_listen(tcs_socket socket_ctx, int backlog);

int tcs_accept(tcs_socket socket_ctx,
                tcs_socket* child_socket_ctx,
                struct tcs_sockaddr* address,
                socklen_t* address_length);

int tcs_send(tcs_socket socket_ctx,
                const uint8_t* buffer,
                size_t buffer_length,
                uint32_t flags,
                size_t* bytes_sent);

int tcs_sendto(tcs_socket socket_ctx,
                const uint8_t* buffer,
                size_t buffer_length,
                uint32_t flags,
                const struct tcs_sockaddr* destination_address,
                size_t destination_address_length,
                size_t* bytes_sent);

int tcs_recv(tcs_socket socket_ctx,
                uint8_t* buffer,
                size_t buffer_length,
                uint32_t flags,
                size_t* bytes_recieved);

int tcs_recvfrom(tcs_socket socket_ctx,
                    uint8_t* buffer,
                    size_t buffer_length,
                    uint32_t flags,
                    struct tcs_sockaddr* source_address,
                    size_t* source_address_length,
                    size_t* bytes_recieved);

int tcs_setsockopt(tcs_socket socket_ctx,
                    int32_t level,
                    int32_t option_name,
                    const void* option_value,
                    socklen_t option_length);

int tcs_shutdown(tcs_socket socket_ctx, int how);

int tcs_free(tcs_socket* socket_ctx);

int tcs_getaddrinfo(const char* node,
                    const char* service,
                    const struct tcs_addrinfo* hints,
                    struct tcs_addrinfo** res);

int tcs_freeaddrinfo(struct tcs_addrinfo** addressinfo);

#ifdef __cplusplus
}
#endif

#endif
