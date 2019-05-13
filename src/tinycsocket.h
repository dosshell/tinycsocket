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

// First we have some code to recognize which system we are compiling against
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

// Then we have some platforms specific definitions
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

// TODO: This needs to be platform specific
// This should work on Linux and windows for now
// Investigate if MacOS/BSD uses uint8_t as sa_family_t

// tcs_sockaddr is an opaque type for addresses. (IPv4, IPv6 etc.)

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

extern const tcs_socket TINYCSOCKET_NULLSOCKET; /**< An empty socket, you should always define your new sockets to this value*/

// TODO: Problem with optimizing when they are in another translation unit. LTO?

// Domain
extern const int TINYCSOCKET_AF_INET; /**< IPv4 interface */

// Type
extern const int TINYCSOCKET_SOCK_STREAM; /**< Use for streaming types like TCP */
extern const int TINYCSOCKET_SOCK_DGRAM; /**< Use for datagrams types like UDP */

// Protocol
extern const int TINYCSOCKET_IPPROTO_TCP; /**< Use TCP protocol (use with TINYCSOCKET_SOCK_STREAM for normal cases) */
extern const int TINYCSOCKET_IPPROTO_UDP; /**< Use UDP protocol (use with TINYCSOCKET_SOCK_DGRAM for normal cases) */

// Flags
extern const int TINYCSOCKET_AI_PASSIVE; /**< Use this flag for pure listening sockets */

// Backlog
extern const int TINYCSOCKET_BACKLOG_SOMAXCONN; /**< Max number of queued sockets when listening */

// How
extern const int TINYCSOCKET_SD_RECIEVE; /**< To shutdown incoming packets for socket */
extern const int TINYCSOCKET_SD_SEND; /**< To shutdown outgoing packets for socket */
extern const int TINYCSOCKET_SD_BOTH; /**< To shutdown both incoming and outgoing packets for socket */

// Socket options
extern const int TINYCSOCKET_SO_REUSEADDR; /**< This is a tricky one! */

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

/**
 * @brief Call this to initialize the library, eg. call this before any other function.
 *
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 */
int tcs_lib_init();

/**
 * @brief Call this when you are done with tinycsockets lib to free resources.
 *
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 */
int tcs_lib_free();

/**
 * @brief Creates a new socket.
 *
 * @code
 * tcs_socket my_socket = TINYCSOCKET_NULLSOCKET;
 * tcs_create(&my_socket, TINYCSOCKET_AF_INET, TINYCSOCKET_SOCK_STREAM, TINYCSOCKET_IPPROTO_TCP);
 * @endcode
 *
 * @param socket_ctx is your in-out pointer to the socket context, you must initialize the socket to #TINYCSOCKET_NULLSOCKET before use.
 * @param domain only supports #TINYCSOCKET_AF_INET for now.
 * @param type specifies the type of the socket, for example #TINYCSOCKET_SOCK_STREAM or #TINYCSOCKET_SOCK_DGRAM.
 * @param protocol specifies the protocol, for example #TINYCSOCKET_IPPROTO_TCP or #TINYCSOCKET_IPPROTO_UDP.
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 * @see tcs_close()
 * @see tcs_lib_init()
 */
int tcs_create(tcs_socket* socket_ctx, int domain, int type, int protocol);

/**
 * @brief Binds the socket to a local address.
 *
 * @param socket_ctx is you in-out socket context.
 * @param address is you address to bind to.
 * @param address_length is you byte size of your @p address argument.
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 * @see tcs_getaddrinfo()
 */
int tcs_bind(tcs_socket socket_ctx,
                const struct tcs_sockaddr* address,
                socklen_t address_length);

/**
 * @brief Connects to a remote address
 *
 * @param socket_ctx is your in-out socket context.
 * @param address is the remote address to connect to.
 * @param address_length is the byte size of the @p address argument.
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 * @see tcs_shutdown()
 */
int tcs_connect(tcs_socket socket_ctx,
                const struct tcs_sockaddr* address,
                socklen_t address_length);

/**
 * @brief Start listen for incoming sockets.
 *
 * @param socket_ctx is your in-out socket context.
 * @param backlog is the maximum number of queued incoming sockets. Use #TINYCSOCKET_BACKLOG_SOMAXCONN to set it to max.
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 * @see tcs_accept()
 */
int tcs_listen(tcs_socket socket_ctx, int backlog);

/**
 * @brief Accepts a socket from a listen socket.
 *
 * @param socket_ctx is your listening socket you used when you called #tcs_listen().
 * @param child_socket_ctx is you accepted socket. Must have the in value of #TINYCSOCKET_NULLSOCKET.
 * @param address is an optional pointer to a buffer where the underlaying address can be stored.
 * @param address_length is an optional in-out pointer to a #socklen_t containing the byte size of the address argument.
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 */
int tcs_accept(tcs_socket socket_ctx,
                tcs_socket* child_socket_ctx,
                struct tcs_sockaddr* address,
                socklen_t* address_length);

/**
 * @brief Sends data on a socket, blocking
 *
 * @param socket_ctx is your in-out socket context.
 * @param buffer is a pointer to your data you want to send.
 * @param buffer_length is number of bytes of the data you want to send.
 * @param flags is currently not in use.
 * @param bytes_sent is how many bytes that was successfully sent.
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 * @see tcs_recv()
 */
int tcs_send(tcs_socket socket_ctx,
                const uint8_t* buffer,
                size_t buffer_length,
                uint32_t flags,
                size_t* bytes_sent);

/**
 * @brief Sends data to an address, useful with UDP sockets.
 *
 * @param socket_ctx is your in-out socket context.
 * @param buffer is a pointer to your data you want to send.
 * @param buffer_length is number of bytes of the data you want to send.
 * @param flags is currently not in use.
 * @param destination_address is the address to send to.
 * @param destination_address_length is the byte size of the @p destination_adress argument.
 * @param bytes_sent is how many bytes that was successfully sent.
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 * @see tcs_recvfrom()
 * @see tcs_getaddrinfo()
 */
int tcs_sendto(tcs_socket socket_ctx,
                const uint8_t* buffer,
                size_t buffer_length,
                uint32_t flags,
                const struct tcs_sockaddr* destination_address,
                size_t destination_address_length,
                size_t* bytes_sent);

/**
* @brief Receive data from a socket to your buffer
*
* @param socket_ctx is your in-out socket context.
* @param buffer is a pointer to your buffer where you want to store the incoming data to.
* @param buffer_length is the byte size of your buffer, for preventing overflows.
* @param flags is currently not in use.
* @param bytes_recieved is how many bytes that was successfully written to your buffer.
* @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
* @see tcs_send()
*/
int tcs_recv(tcs_socket socket_ctx,
                uint8_t* buffer,
                size_t buffer_length,
                uint32_t flags,
                size_t* bytes_recieved);

/**
* @brief Sends data to an address, useful with UDP sockets.
*
* @param socket_ctx is your in-out socket context.
* @param buffer is a pointer to your buffer where you want to store the incoming data to.
* @param buffer_length is the byte size of your buffer, for preventing overflows.
* @param flags is currently not in use.
* @param source_address is the address to receive from.
* @param source_address_length is the byte size of the @p source_address argument.
* @param bytes_recieved is how many bytes that was successfully written to your buffer.
* @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
* @see tcs_sendto()
* @see tcs_getaddrinfo()
*/
int tcs_recvfrom(tcs_socket socket_ctx,
                    uint8_t* buffer,
                    size_t buffer_length,
                    uint32_t flags,
                    struct tcs_sockaddr* source_address,
                    size_t* source_address_length,
                    size_t* bytes_recieved);

/**
* @brief Set parameters on a socket
*
* @param socket_ctx is your in-out socket context.
* @param level is the definition level.
* @param option_name is the option name.
* @param option_value is a pointer to the option value.
* @param option_length is the byte size of the data pointed by @p option_value.
* @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
*/
int tcs_setsockopt(tcs_socket socket_ctx,
                    int32_t level,
                    int32_t option_name,
                    const void* option_value,
                    socklen_t option_length);

/**
* @brief Turn off communication for the socket.
*
* @param socket_ctx is your in-out socket context.
* @param how defines in which direction you want to turn off the communication.
* @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
*/
int tcs_shutdown(tcs_socket socket_ctx, int how);

/**
* @brief closes the socket, call this when you are done with the socket.
*
* @param socket_ctx is your in-out socket context.
* @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
*/
int tcs_close(tcs_socket* socket_ctx);

/**
* @brief Get addresses you can connect to given a computer name and a port.
*
* @param node is your computer identifier: hostname, IPv4 or IPv6 address.
* @param service is your port number. Also some support for common aliases like "http" exist.
* @param hints is a struct with hints, for example if you only are interested in IPv6.
* @param res is your output pointer to a linked list of addresses. You need to free this list when you are done with it.
* @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
* @see tcs_freeaddrinfo()
*/
int tcs_getaddrinfo(const char* node,
                    const char* service,
                    const struct tcs_addrinfo* hints,
                    struct tcs_addrinfo** res);

/**
 * @brief Frees your linked address list you acquired from tcs_getaddrinfo
 *
 * @param addressinfo is your linked list you acquired from tcs_getaddrinfo
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 * @see tcs_getaddrinfo()
 */
int tcs_freeaddrinfo(struct tcs_addrinfo** addressinfo);

 /**
 * @brief Creates a socket and connects to a hostname and port
 *
 * @param socket_ctx is your out socket context. Must be of #TINYCSOCKET_NULLSOCKET value.
 * @param hostname is the name of the host to connect to, for example localhost.
 * @param port is a string representation of the port you want to connect to. Normally an integer, like "5000" but also some support for common aliases like "http" exist.
 * @param domain only supports #TINYCSOCKET_AF_INET for now
 * @param protocol specifies the protocol, for example #TINYCSOCKET_IPPROTO_TCP or #TINYCSOCKET_IPPROTO_UDP.
 * @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
 * @see tcs_simple_listen()
 * @see tcs_simple_bind()
 */
int tcs_simple_connect(tcs_socket* socket_ctx, const char* hostname, const char* port, int domain, int protocol);

/**
* @brief Creates a socket and binds it to a node and a port
*
* @param socket_ctx is your out socket context. Must be of #TINYCSOCKET_NULLSOCKET value.
* @param hostname is the name of the host to bind to, for example "192.168.0.1" or "localhost".
* @param port is a string representation of the port you want to bind to. Normally an integer, like "5000" but also some support for common aliases like "http" exist.
* @param domain only supports #TINYCSOCKET_AF_INET for now
* @param protocol specifies the protocol, for example #TINYCSOCKET_IPPROTO_TCP or #TINYCSOCKET_IPPROTO_UDP.
* @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
* @see tcs_simple_connect()
*/
int tcs_simple_bind(tcs_socket* socket_ctx, const char* hostname, const char* port, int domain, int protocol);

/**
* @brief Listens to an address with TCP
*
* @param socket_ctx is your out socket context. Must be of #TINYCSOCKET_NULLSOCKET value.
* @param hostname is the name of the address to listen on, for example "192.168.0.1" or "localhost".
* @param port is a string representation of the port you want to listen to. Normally an integer, like "5000" but also some support for common aliases like "http" exist.
* @param domain only supports #TINYCSOCKET_AF_INET for now
* @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
* @see tcs_simple_connect()
*/
int tcs_simple_listen(tcs_socket* socket_ctx, const char* hostname, const char* port, int domain);

/**
* @brief Receives and fill the buffer width a fixed length of data (normal recv can fill the buffer less than the buffer length)
*
* @param socket_ctx is your in-out socket context.
* @param buffer is a pointer to your buffer where you want to store the incoming data to.
* @param buffer_length is the byte size of your buffer, it will fill the complete buffer.
* @return #TINYCSOCKET_SUCCESS if successful, otherwise the error code.
* @see tcs_send_all()
*/
int tcs_simple_recv_all(tcs_socket socket_ctx, uint8_t* buffer, size_t length);

/**
* @brief Sends the full buffer (normal send is allowed to send only a part of the buffer)
*/
int tcs_simple_send_all(tcs_socket socket_ctx, uint8_t* buffer, size_t length, uint32_t flags);

int tcs_simple_recv_netstring(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_length, size_t* bytes_recieved);

int tcs_simple_send_netstring(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_length);

#ifdef __cplusplus
}
#endif

#endif
