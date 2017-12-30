// MIT

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
extern "C" {
#endif

#if defined(TINYCSOCKET_USE_WIN32_IMPL)
#include <basetsd.h>
typedef UINT_PTR TinyCSocketCtx;
typedef int socklen_t;

struct TinyCSocketAddressInfo
{
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    char* ai_canonname;
    struct TinyCSocketAddress* ai_addr;
    struct TinyCSocketAddressInfo* ai_next;
};

#elif defined(TINYCSOCKET_USE_POSIX_IMPL)
typedef int TinyCSocketCtx;
typedef int socklen_t;

struct TinyCSocketAddressInfo
{
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    struct TinyCSocketAddress* ai_addr;
    char* ai_canonname;
    struct TinyCSocketAddressInfo* ai_next;
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

struct TinyCSocketAddress
{
    sa_family_t ss_family;
    char __ss_pad1[_SS_PAD1SIZE__];
    int64_t __ss_align;
    char __ss_pad2[_SS_PAD2SIZE__];
};

extern const TinyCSocketCtx TINYCSOCKET_NULLSOCKET;

// TODO: Problem with optimizing when they are in another translation unit? LTO?

// Domain
extern const int TINYCSOCKET_AF_INET;

// Type
extern const int TINYCSOCKET_SOCK_STREAM;
extern const int TINYCSOCKET_SOCK_DGRAM;

// Protocol
extern const int TINYCSOCKET_IPPROTO_TCP;

// Flags
extern const int TINYCSOCKET_AI_PASSIVE;

// Backlog
extern const int TINYCSOCKET_BACKLOG_SOMAXCONN;

// How
static const int TINYCSOCKET_RECIEVE = 1;
static const int TINYCSOCKET_SEND = 2;
static const int TINYCSOCKET_BOTH = 3;

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

int tinycsocket_init();

int tinycsocket_free();

int tinycsocket_socket(TinyCSocketCtx* socket_ctx, int domain, int type, int protocol);

int tinycsocket_bind(TinyCSocketCtx socket_ctx,
                     const struct TinyCSocketAddress* address,
                     socklen_t address_length);

int tinycsocket_connect(TinyCSocketCtx socket_ctx,
                        const struct TinyCSocketAddress* address,
                        socklen_t address_length);

int tinycsocket_listen(TinyCSocketCtx socket_ctx, int backlog);

int tinycsocket_accept(TinyCSocketCtx socket_ctx,
                       TinyCSocketCtx* child_socket_ctx,
                       struct TinyCSocketAddress* address,
                       socklen_t* address_length);

int tinycsocket_send(TinyCSocketCtx socket_ctx,
                     const uint8_t* buffer,
                     size_t buffer_length,
                     uint_fast32_t flags,
                     size_t* bytes_sent);

int tinycsocket_sendto(TinyCSocketCtx socket_ctx,
                       const uint8_t* buffer,
                       size_t buffer_length,
                       uint_fast32_t flags,
                       const struct TinyCSocketAddress* destination_address,
                       size_t destination_address_length,
                       size_t* bytes_sent);

int tinycsocket_recv(TinyCSocketCtx socket_ctx,
                     uint8_t* buffer,
                     size_t buffer_length,
                     uint_fast32_t flags,
                     size_t* bytes_recieved);

int tinycsocket_recvfrom(TinyCSocketCtx socket_ctx,
                         uint8_t* buffer,
                         size_t buffer_length,
                         uint_fast32_t flags,
                         struct TinyCSocketAddress* source_address,
                         size_t* source_address_length,
                         size_t* bytes_recieved);

int tinycsocket_setsockopt(TinyCSocketCtx socket_ctx,
                           int_fast32_t level,
                           int_fast32_t option_name,
                           const void* option_value,
                           socklen_t option_length);

int tinycsocket_shutdown(TinyCSocketCtx socket_ctx, int how);

int tinycsocket_closesocket(TinyCSocketCtx* socket_ctx);

int tinycsocket_getaddrinfo(const char* node,
                            const char* service,
                            const struct TinyCSocketAddressInfo* hints,
                            struct TinyCSocketAddressInfo** res);

int tinycsocket_freeaddrinfo(struct TinyCSocketAddressInfo** addressinfo);

#ifdef __cplusplus
}
#endif

#endif
