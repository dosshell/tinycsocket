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
#elif defined(__linux__) || defined(__sun) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
    defined(__APPLE__) || defined(__MSYS__)
#define TINYCSOCKET_USE_POSIX_IMPL
#else
#pragma message("Warning: Unknown OS, trying POSIX")
#define TINYCSOCKET_USE_POSIX_IMPL
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Then we have some platforms specific definitions
#if defined(TINYCSOCKET_USE_WIN32_IMPL)
#include <basetsd.h>
typedef UINT_PTR tcs_socket;
#elif defined(TINYCSOCKET_USE_POSIX_IMPL)
typedef int tcs_socket;
#endif

struct tcs_sockaddr
{
    uint16_t family;
    union
    {
        struct
        {
            uint16_t port;
            uint32_t addr;
        } af_inet;
        struct
        {
            uint16_t port;
            uint32_t flowinfo;
            uint64_t addr;
            uint32_t scope_id;
        } af_inet6;
    } data;
};

struct tcs_addrinfo
{
    uint16_t family;
    int socktype;
    int protocol;
    uint32_t flags;
    struct tcs_sockaddr address;
};

extern const tcs_socket TCS_NULLSOCKET; /**< An empty socket, you should always define your new sockets to this value */

// Family
extern const uint16_t TCS_AF_UNSPEC; /**< Layer 3 agnostic */
extern const uint16_t TCS_AF_INET;   /**< IPv4 interface */
extern const uint16_t TCS_AF_INET6;  /**< IPv6 interface */

// Type
extern const int TCS_SOCK_STREAM; /**< Use for streaming types like TCP */
extern const int TCS_SOCK_DGRAM;  /**< Use for datagrams types like UDP */

// Protocol
extern const int TCS_IPPROTO_TCP; /**< Use TCP protocol (use with TCS_SOCK_STREAM for normal cases) */
extern const int TCS_IPPROTO_UDP; /**< Use UDP protocol (use with TCS_SOCK_DGRAM for normal cases) */

// Flags
extern const uint32_t TCS_AI_PASSIVE; /**< Use this flag for pure listening sockets */

// Recv flags
extern const int TCS_MSG_WAITALL;
extern const int TCS_MSG_PEEK;
extern const int TCS_MSG_OOB;

// Backlog
extern const int TCS_BACKLOG_SOMAXCONN; /**< Max number of queued sockets when listening */

// How
extern const int TCS_SD_RECEIVE; /**< To shutdown incoming packets for socket */
extern const int TCS_SD_SEND;    /**< To shutdown outgoing packets for socket */
extern const int TCS_SD_BOTH;    /**< To shutdown both incoming and outgoing packets for socket */

// Socket options
extern const int TCS_SOL_SOCKET;   /**< Socket option level */
extern const int TCS_SO_REUSEADDR; /**< This is a tricky one! */
extern const int TCS_SO_RCVBUF;    /**< Byte size of receiving buffer */
extern const int TCS_SO_SNDBUF;    /**< Byte size of receiving buffer */

// Return codes
static const int TCS_SUCCESS = 0;
static const int TCS_ERROR_UNKNOWN = -1;
static const int TCS_ERROR_MEMORY = -2;
static const int TCS_ERROR_INVALID_ARGUMENT = -3;
static const int TCS_ERROR_KERNEL = -4;
static const int TCS_ERROR_ADDRESS_LOOKUP_FAILED = -5;
static const int TCS_ERROR_CONNECTION_REFUSED = -6;
static const int TCS_ERROR_NOT_INITED = -7;
static const int TCS_ERROR_TIMED_OUT = -8;
static const int TCS_ERROR_NOT_IMPLEMENTED = -9;
static const int TCS_ERROR_NOT_CONNECTED = -10;
static const int TCS_ERROR_ILL_FORMED_MESSAGE = -11;
static const int TCS_ERROR_SOCKET_CLOSED = -13;

/**
 * @brief Call this to initialize the library, eg. call this before any other function.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
int tcs_lib_init(void);

/**
 * @brief Call this when you are done with tinycsocket lib to free resources.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
int tcs_lib_free(void);

/**
 * @brief Creates a new socket.
 *
 * @code
 * tcs_socket my_socket = TCS_NULLSOCKET;
 * tcs_create(&my_socket, TCS_AF_INET, TCS_SOCK_STREAM, TCS_IPPROTO_TCP);
 * @endcode
 *
 * @param socket_ctx is your in-out pointer to the socket context, you must initialize the socket to #TCS_NULLSOCKET before use.
 * @param family only supports #TCS_AF_INET for now.
 * @param type specifies the type of the socket, for example #TCS_SOCK_STREAM or #TCS_SOCK_DGRAM.
 * @param protocol specifies the protocol, for example #TCS_IPPROTO_TCP or #TCS_IPPROTO_UDP.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_close()
 * @see tcs_lib_init()
 */
int tcs_create(tcs_socket* socket_ctx, int family, int type, int protocol);

/**
 * @brief Binds the socket to a local address.
 *
 * @param socket_ctx is your in-out socket context.
 * @param address is you address to bind to.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_getaddrinfo()
 */
int tcs_bind(tcs_socket socket_ctx, const struct tcs_sockaddr* address);

/**
 * @brief Connects to a remote address
 *
 * @param socket_ctx is your in-out socket context.
 * @param address is the remote address to connect to.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_shutdown()
 */
int tcs_connect(tcs_socket socket_ctx, const struct tcs_sockaddr* address);

/**
 * @brief Start listen for incoming sockets.
 *
 * @param socket_ctx is your in-out socket context.
 * @param backlog is the maximum number of queued incoming sockets. Use #TCS_BACKLOG_SOMAXCONN to set it to max.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_accept()
 */
int tcs_listen(tcs_socket socket_ctx, int backlog);

/**
 * @brief Accepts a socket from a listen socket.
 *
 * @param socket_ctx is your listening socket you used when you called #tcs_listen().
 * @param child_socket_ctx is you accepted socket. Must have the in value of #TCS_NULLSOCKET.
 * @param address is an optional pointer to a buffer where the underlaying address can be stored.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
int tcs_accept(tcs_socket socket_ctx, tcs_socket* child_socket_ctx, struct tcs_sockaddr* address);

/**
 * @brief Sends data on a socket, blocking
 *
 * @param socket_ctx is your in-out socket context.
 * @param buffer is a pointer to your data you want to send.
 * @param buffer_size is number of bytes of the data you want to send.
 * @param flags is currently not in use.
 * @param bytes_sent is how many bytes that was successfully sent.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_recv()
 */
int tcs_send(tcs_socket socket_ctx, const uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_sent);

/**
 * @brief Sends data to an address, useful with UDP sockets.
 *
 * @param socket_ctx is your in-out socket context.
 * @param buffer is a pointer to your data you want to send.
 * @param buffer_size is number of bytes of the data you want to send.
 * @param flags is currently not in use.
 * @param destination_address is the address to send to.
 * @param bytes_sent is how many bytes that was successfully sent.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_recvfrom()
 * @see tcs_getaddrinfo()
 */
int tcs_sendto(tcs_socket socket_ctx,
               const uint8_t* buffer,
               size_t buffer_size,
               uint32_t flags,
               const struct tcs_sockaddr* destination_address,
               size_t* bytes_sent);

/**
* @brief Receive data from a socket to your buffer
*
* @param socket_ctx is your in-out socket context.
* @param buffer is a pointer to your buffer where you want to store the incoming data to.
* @param buffer_size is the byte size of your buffer, for preventing overflows.
* @param flags is currently not in use.
* @param bytes_received is how many bytes that was successfully written to your buffer.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_send()
*/
int tcs_recv(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_received);

/**
* @brief Receive data from an address, useful with UDP sockets.
*
* @param socket_ctx is your in-out socket context.
* @param buffer is a pointer to your buffer where you want to store the incoming data to.
* @param buffer_size is the byte size of your buffer, for preventing overflows.
* @param flags is currently not in use.
* @param source_address is the address to receive from.
* @param bytes_received is how many bytes that was successfully written to your buffer.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_sendto()
* @see tcs_getaddrinfo()
*/
int tcs_recvfrom(tcs_socket socket_ctx,
                 uint8_t* buffer,
                 size_t buffer_size,
                 uint32_t flags,
                 struct tcs_sockaddr* source_address,
                 size_t* bytes_received);

/**
* @brief Set parameters on a socket
*
* @param socket_ctx is your in-out socket context.
* @param level is the definition level.
* @param option_name is the option name.
* @param option_value is a pointer to the option value.
* @param option_size is the byte size of the data pointed by @p option_value.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
int tcs_setsockopt(tcs_socket socket_ctx,
                   int32_t level,
                   int32_t option_name,
                   const void* option_value,
                   size_t option_size);

/**
* @brief Turn off communication for the socket. Will finish all sends first.
*
* @param socket_ctx is your in-out socket context.
* @param how defines in which direction you want to turn off the communication.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
int tcs_shutdown(tcs_socket socket_ctx, int how);

/**
* @brief Closes the socket, call this when you are done with the socket.
*
* @param socket_ctx is your in-out socket context.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
int tcs_close(tcs_socket* socket_ctx);

/**
* @brief Get addresses you can connect to given a computer name and a port.
*
* Use NULL for @res to get the total number of addresses found.
*
* @param node is your computer identifier: hostname, IPv4 or IPv6 address.
* @param service is your port number. Also some support for common aliases like "http" exist.
* @param hints is a struct with hints, for example if you only are interested in IPv6.
* @param res is a pointer to your array where to store the result.
* @param res_count is number of elements your @res array can store.
* @param used_count will output the number of addresses that was populated in @res.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
int tcs_getaddrinfo(const char* node,
                    const char* service,
                    const struct tcs_addrinfo* hints,
                    struct tcs_addrinfo res[],
                    size_t res_count,
                    size_t* used_count);

/**
 * @brief Connects a socket to a node and a port.
 *
 * @param socket_ctx is your out socket context. Must have been previously created.
 * @param hostname is the name of the host to connect to, for example localhost.
 * @param port is a string representation of the port you want to connect to. Normally an integer, like "5000" but also some support for common aliases like "http" exist.
 * @param family only supports #TCS_AF_INET for now
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_simple_listen()
 * @see tcs_simple_bind()
 */
int tcs_simple_create_and_connect(tcs_socket* socket_ctx, const char* hostname, const char* port, uint16_t family);

/**
* @brief Creates a socket and binds it to a node and a port
*
* @param socket_ctx is your out socket context. Must be of #TCS_NULLSOCKET value.
* @param hostname is the name of the host to bind to, for example "192.168.0.1" or "localhost".
* @param port is a string representation of the port you want to bind to. Normally an integer, like "5000" but also some support for common aliases like "http" exist.
* @param family only supports #TCS_AF_INET for now
* @param protocol specifies the protocol, for example #TCS_IPPROTO_TCP or #TCS_IPPROTO_UDP.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_simple_connect()
*/
int tcs_simple_create_and_bind(tcs_socket* socket_ctx,
                               const char* hostname,
                               const char* port,
                               uint16_t family,
                               int protocol);

/**
* @brief Creates a socket and starts to listen to an address with TCP
*
* @param socket_ctx is your out socket context. Must be of #TCS_NULLSOCKET value.
* @param hostname is the name of the address to listen on, for example "192.168.0.1" or "localhost". Use NULL for all interfaces.
* @param port is a string representation of the port you want to listen to. Normally an integer, like "5000" but also some support for common aliases like "http" exist.
* @param family only supports #TCS_AF_INET for now.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_simple_connect()
*/
int tcs_simple_create_and_listen(tcs_socket* socket_ctx, const char* hostname, const char* port, uint16_t family);

/**
* @brief Receive data until the buffer is filled (normal recv can fill the buffer less than the buffer length).
*
* @param socket_ctx is your in-out socket context.
* @param buffer is a pointer to your buffer where you want to store the incoming data to.
* @param buffer_size is the byte size of your buffer, it will fill the complete buffer.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_send_all()
*/
int tcs_simple_recv_all(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_size);

/**
* @brief Sends the full buffer (normal send is allowed to send only a part of the buffer)
*
* @param socket_ctx is your in-out socket context.
* @param buffer is a pointer to your data you want to send.
* @param buffer_size is the total size of your buffer in bytes.
* @param flags your flags.
*/
int tcs_simple_send_all(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_size, uint32_t flags);

int tcs_simple_recv_netstring(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_size, size_t* bytes_received);

int tcs_simple_send_netstring(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
