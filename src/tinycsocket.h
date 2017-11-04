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

extern const TinyCSocketCtx TINYCSOCKET_NULLSOCKET;

#if defined(TINYCSOCKET_USE_WIN32_IMPL)
#include <basetsd.h>
typedef UINT_PTR TinyCSocketCtx;
#elif defined(TINYCSOCKET_USE_POSIX_IMPL)
typedef int TinyCSocketCtx;
typedef uint_fast64_t ticktype_t;
#endif

static int TINYCSOCKET_RECIEVE = 1;
static int TINYCSOCKET_SEND = 2;
static int TINYCSOCKET_BOTH = 3;

static int TINYCSOCKET_SUCCESS = 0;
static int TINYCSOCKET_ERROR_UNKNOWN = -1;
static int TINYCSOCKET_ERROR_MEMORY = -2;
static int TINYCSOCKET_ERROR_INVALID_ARGUMENT = -3;
static int TINYCSOCKET_ERROR_KERNEL = -4;
static int TINYCSOCKET_ERROR_ADDRESS_LOOKUP_FAILED = -5;
static int TINYCSOCKET_ERROR_CONNECTION_REFUSED = -6;
static int TINYCSOCKET_ERROR_NOT_INITED = -7;
static int TINYCSOCKET_ERROR_TIMED_OUT = -8;
static int TINYCSOCKET_ERROR_NOT_IMPLEMENTED = -9;
static int TINYCSOCKET_ERROR_NOT_CONNECTED = -10;
static int TINYCSOCKET_ERROR_ILL_FORMED_MESSAGE = -11;

int tinycsocket_init();

int tinycsocket_free();

int tinycsocket_socket(TinyCSocketCtx* socket_ctx, int domain, int type, int protocol);

int tinycsocket_bind(TinyCSocketCtx socket_ctx,
                     struct TinyCSocketAddress* address,
                     size_t address_length);

int tinycsocket_connect(TinyCSocketCtx socket_ctx,
                        struct TinyCSocketAddress* address,
                        size_t address_length);

int tinycsocket_listen(TinyCSocketCtx socket_ctx, int backlog);

int tinycsocket_accept(TinyCSocketCtx socket_ctx,
                       TinyCSocketCtx* child_socket_ctx,
                       struct TinyCSocketAddress* address,
                       size_t address_length);

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
                           size_t option_length);

int tinycsocket_shutdown(TinyCSocketCtx socket_ctx, int how);

int tinycsocket_closesocket(TinyCSocketCtx* socket_ctx);

#endif
