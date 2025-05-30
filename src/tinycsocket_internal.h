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

#ifndef TINYCSOCKET_INTERNAL_H_
#define TINYCSOCKET_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const char* const TCS_VERSION_TXT = "v0.3-dev";
static const char* const TCS_LICENSE_TXT =
    "Copyright 2018 Markus Lindelöw"
    "\n"
    "Permission is hereby granted, free of charge, to any person obtaining a copy"
    "of this software and associated documentation files(the \"Software\"), to deal"
    "in the Software without restriction, including without limitation the rights"
    "to use, copy, modify, merge, publish, distribute, sublicense, and / or sell"
    "copies of the Software, and to permit persons to whom the Software is"
    "furnished to do so, subject to the following conditions:"
    "\n"
    "The above copyright notice and this permission notice shall be included in all"
    "copies or substantial portions of the Software."
    "\n"
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR"
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,"
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE"
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER"
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,"
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE"
    "SOFTWARE.";

// First we have some code to recognize which system we are compiling against
#if defined(WIN32) || defined(__MINGW32__)
#define TINYCSOCKET_USE_WIN32_IMPL
#elif defined(__linux__) || defined(__sun) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
    (defined(__APPLE__) && defined(__MACH__)) || defined(__MSYS__) || defined(__unix__)
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
#ifdef _WINSOCKAPI_
#error winsock.h included instead of winsock2.h. Define "_WINSOCKAPI_" or include this header file before windows.h to fix the problem
#endif
#ifndef __MINGW32__  // MinGW will generate a warning by it self.
#define _WINSOCKAPI_ // Prevent inclusion of winsock.h in windows.h, use winsock2.h
#endif
#include <basetsd.h>
typedef UINT_PTR TcsSocket;
#elif defined(TINYCSOCKET_USE_POSIX_IMPL)
typedef int TcsSocket;
#endif

/**
 * @brief Address Family
 */
typedef enum
{
    TCS_AF_ANY, /**< Layer 3 agnostic */
    TCS_AF_IP4, /**< INET IPv4 interface */
    TCS_AF_IP6, /**< INET IPv6 interface */
    TCS_AF_LENGTH
} TcsAddressFamily;

/**
 * @brief Network Address
 */
struct TcsAddress
{
    TcsAddressFamily family;
    union
    {
        struct
        {
            uint16_t port;    /**< Same byte order as the host */
            uint32_t address; /**< Same byte order as the host */
        } af_inet;
        struct
        {
            uint16_t port;
            uint32_t flowinfo;
            uint64_t address;
            uint32_t scope_id;
        } af_inet6;
    } data;
};

// gcc may trigger bug #53119
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif
static const struct TcsAddress TCS_ADDRESS_NULL = {TCS_AF_ANY, {0, 0}};
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

extern const uint32_t TCS_ADDRESS_ANY_IP4;
extern const uint32_t TCS_ADDRESS_LOOPBACK_IP4;
extern const uint32_t TCS_ADDRESS_BROADCAST_IP4;
extern const uint32_t TCS_ADDRESS_NONE_IP4;

/**
 * @brief Network Interface Information
 */
struct TcsInterface
{
    struct TcsAddress address;
    char name[32];
};

/**
 * @brief Used when sending/receiving an array of buffers.
 *
 * Useful if you want to send two or more data arrays at once, for example a header and a body.
 * Make an array of TcsBuffer and use tcs_sendv() to send them all at once.
*/
struct TcsBuffer
{
#if defined(TINYCSOCKET_USE_WIN32_IMPL)
    unsigned long length;
    uint8_t* buffer;
#elif defined(TINYCSOCKET_USE_POSIX_IMPL)
    uint8_t* buffer;
    size_t length;
#endif
};

extern const TcsSocket TCS_NULLSOCKET; /**< An empty socket, you should always define your new sockets to this value */
static const uint32_t TCS_NO_FLAGS = 0;

// Type
extern const int TCS_SOCK_STREAM; /**< Use for streaming types like TCP */
extern const int TCS_SOCK_DGRAM;  /**< Use for datagrams types like UDP */

// Protocol
extern const int TCS_IPPROTO_TCP; /**< Use TCP protocol (use with TCS_SOCK_STREAM for normal cases) */
extern const int TCS_IPPROTO_UDP; /**< Use UDP protocol (use with TCS_SOCK_DGRAM for normal cases) */

// Simple socket creation
typedef enum
{
    TCS_TYPE_TCP_IP4,
    TCS_TYPE_UDP_IP4,
    TCS_TYPE_TCP_IP6,
    TCS_TYPE_UDP_IP6,
} TcsType;

// Flags
extern const uint32_t TCS_AI_PASSIVE; /**< Use this flag for pure listening sockets */

// Recv flags
extern const uint32_t TCS_MSG_PEEK;
extern const uint32_t TCS_MSG_OOB;
extern const uint32_t TCS_MSG_WAITALL;

// Send flags
extern const uint32_t TCS_MSG_SENDALL;

// Backlog
extern const int TCS_BACKLOG_SOMAXCONN; /**< Max number of queued sockets when listening */

// Socket Direction
typedef enum
{
    TCS_SD_RECEIVE, /**< To shutdown incoming packets for socket */
    TCS_SD_SEND,    /**< To shutdown outgoing packets for socket */
    TCS_SD_BOTH,    /**< To shutdown both incoming and outgoing packets for socket */
} TcsSocketDirection;

// Option levels
extern const int TCS_SOL_SOCKET; /**< Socket option level for socket options */
extern const int TCS_SOL_IP;     /**< IP option level for socket options */

// Socket options
extern const int TCS_SO_BROADCAST;
extern const int TCS_SO_KEEPALIVE;
extern const int TCS_SO_LINGER;
extern const int TCS_SO_REUSEADDR; /**< This is a tricky one for crossplatform independency! */
extern const int TCS_SO_RCVBUF;    /**< Byte size of receiving buffer */
extern const int TCS_SO_RCVTIMEO;
extern const int TCS_SO_SNDBUF; /**< Byte size of receiving buffer */
extern const int TCS_SO_OOBINLINE;

// IP options
extern const int TCS_SO_IP_NODELAY;
extern const int TCS_SO_IP_MEMBERSHIP_ADD;
extern const int TCS_SO_IP_MEMBERSHIP_DROP;
extern const int TCS_SO_IP_MULTICAST_LOOP;

// Use for timeout to wait until infinity happens
extern const int TCS_INF;

// Return codes
typedef enum
{
    TCS_AGAIN = 1,
    TCS_SUCCESS = 0,
    TCS_ERROR_UNKNOWN = -1,
    TCS_ERROR_MEMORY = -2,
    TCS_ERROR_INVALID_ARGUMENT = -3,
    TCS_ERROR_KERNEL = -4,
    TCS_ERROR_ADDRESS_LOOKUP_FAILED = -5,
    TCS_ERROR_CONNECTION_REFUSED = -6,
    TCS_ERROR_NOT_INITED = -7,
    TCS_ERROR_TIMED_OUT = -8,
    TCS_ERROR_NOT_IMPLEMENTED = -9,
    TCS_ERROR_NOT_CONNECTED = -10,
    TCS_ERROR_ILL_FORMED_MESSAGE = -11,
    TCS_ERROR_SOCKET_CLOSED = -12,
    TCS_ERROR_WOULD_BLOCK = -13,
} TcsReturnCode;

struct TcsPool;
struct TcsPollEvent
{
    TcsSocket socket;
    void* user_data;
    bool can_read;
    bool can_write;
    TcsReturnCode error;
};
static const struct TcsPollEvent TCS_NULLEVENT = {0, 0, false, false, TCS_SUCCESS};

/**
 * @brief Platform independent utility function to compose an IPv4 address from 4 bytes.
 * 
 * The order of the bytes are: a.b.c.d. Tinycsocket API will always expose host byte order (little endian).
 * You will _never_ need to think of byte order (except if you are debugging the library OS specific parts of course).
 *
 * @code
 * // Create address 192.168.0.1
 * uint32_t address = 0;
 * tcs_util_ipv4_args(192, 168, 0, 1, &address);
 * @endcode
 * 
 * @param a Is the first byte
 * @param b Is the second byte
 * @param c Is the third byte
 * @param d Is the forth byte
 * @param out_address is your ipv4 address stored as an unsigned 32bit integer. This parameter can not be NULL.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsReturnCode tcs_util_ipv4_args(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint32_t* out_address);

/**
 * @brief Platform independent parsing of a string to an IPv4 address.
 *
 * If the local_port argument is excluded from the string the assign local_port value out the address will be zero.
 * Some example of valid formats are "192.168.0.1" or "127.0.0.1:1212".
 * The address format also supports mixed hex, octal and decimal format. For example "0xC0.0250.0.0x01:0x4bc".
 * 
 * @code
 * TcsSocket socket = TCS_NULLSOCKET;
 * tcs_create(&socket, TCS_TYPE_TCP_IP4);
 * struct TcsAddress address;
 * tcs_util_string_to_address("192.168.0.1:1212", &address);
 * tcs_connect_address(socket, address);
 * @endcode
 * 
 * @param str is a valid pointer to an IPv4 NULL terminated string address, with optional local_port number. Such as "192.168.0.1:1212".
 * @param out_address out pointer to save address.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsReturnCode tcs_util_string_to_address(const char str[], struct TcsAddress* out_address);

/**
 * @brief Platform independent parsing of an IPv4 address to a string.
 *
 * @param address
 * @param out_str
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsReturnCode tcs_util_address_to_string(const struct TcsAddress* address, char out_str[40]);

/**
 * @brief Call this to initialize the library, eg. call this before any other function.
 *
 * You should call #tcs_lib_free() after you are done with the library (before program exit).
 * You need to call #tcs_lib_free the same amount of times as you call #tcs_lib_init().
 * 
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsReturnCode tcs_lib_init(void);

/**
 * @brief Call this when you are done with tinycsocket lib to free resources.
 *
 * You need to call this the same amount of times as you have called #tcs_lib_init().
 * This make it easy to use RAII of you use C++.
 * 
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsReturnCode tcs_lib_free(void);

/**
 * @brief Creates a new socket.
 *
 * @code
 * TcsSocket my_socket = TCS_NULLSOCKET;
 * tcs_create(&my_socket, TCS_TYPE_TCP_IP4);
 * @endcode
 *
 * @param socket_ctx is your in-out pointer to the socket context, you must initialize the socket to #TCS_NULLSOCKET before use.
 * @param socket_type specifies the internet and transport layer, for example #TCS_TYPE_TCP_IP4 or #TCS_TYPE_UDP_IP6.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_destroy()
 * @see tcs_lib_init()
 */
TcsReturnCode tcs_create(TcsSocket* socket_ctx, TcsType socket_type);

/**
 * @brief Creates a new socket with BSD-style options such family, type and protocol.
 *
 * @code
 * TcsSocket my_socket = TCS_NULLSOCKET;
 * tcs_create_ext(&my_socket, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP);
 * @endcode
 *
 * @param socket_ctx is your in-out pointer to the socket context, you must initialize the socket to #TCS_NULLSOCKET before use.
 * @param family only supports #TCS_AF_IP4 for now.
 * @param type specifies the type of the socket, for example #TCS_SOCK_STREAM or #TCS_SOCK_DGRAM.
 * @param protocol specifies the protocol, for example #TCS_IPPROTO_TCP or #TCS_IPPROTO_UDP.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_destroy()
 * @see tcs_lib_init()
 */
TcsReturnCode tcs_create_ext(TcsSocket* socket_ctx, TcsAddressFamily family, int type, int protocol);

/**
 * @brief Binds a socket to local_port on all interfaces.
 *
 * This is similar to:
 * @code
 * struct TcsAddress local_address = {0};
 * local_address.family = TCS_AF_IP4;
 * local_address.data.af_inet.address = TCS_ADDRESS_ANY_IP4;
 * local_address.data.af_inet.local_port = local_port;
 * tcs_bind_address(socket_ctx, &local_address);
 * @endcode
 * 
 * @param socket_ctx is your in-out socket context you want bind.
 * @param local_port is your local portnumber you want to bind to.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_bind_address()
 * @see tcs_listen()
 */
TcsReturnCode tcs_bind(TcsSocket socket_ctx, uint16_t local_port);

/**
 * @brief Binds a socket to a local address.
 *
 * @param socket_ctx is your in-out socket context you want to bind.
 * @param local_address is your local address you want to bind to.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_bind()
 * @see tcs_get_interfaces()
 * @see tcs_listen()
 */
TcsReturnCode tcs_bind_address(TcsSocket socket_ctx, const struct TcsAddress* local_address);

/**
 * @brief Connect a socket to a remote hostname and port.
 *
 * @param socket_ctx is your in-out socket context you want to connect. 
 * @param hostname name of host or ip
 * @param port to connect to
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_bind()
 * @see tcs_get_interfaces()
 * @see tcs_listen()
 */
TcsReturnCode tcs_connect(TcsSocket socket_ctx, const char* hostname, uint16_t port);

/**
 * @brief Connects a socket to a remote address.
 *
 * @param socket_ctx is your in-out socket context.
 * @param address is the remote address to connect to.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_shutdown()
 */
TcsReturnCode tcs_connect_address(TcsSocket socket_ctx, const struct TcsAddress* address);

/**
 * @brief Let a socket start listening for incoming connections.
 *
 * Call #tcs_bind() first to bind to a local address to listening at.
 *
 * @param socket_ctx is your in-out socket context.
 * @param backlog is the maximum number of queued incoming sockets. Use #TCS_BACKLOG_SOMAXCONN to set it to max.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_accept()
 * @see tcs_listen_to()
 */
TcsReturnCode tcs_listen(TcsSocket socket_ctx, int backlog);

/**
 * @brief Bind a socket to a local portnumber and start listening to new connections.
 * 
 * Example usage:
 * @code
 * TcsSocket listen_socket = TCS_NULLSOCKET;
 * tcs_create(&listen_socket, TCS_TYPE_TCP_IP4);
 * tcs_listen_to(listen_socket, 1212);
 * @endcode
 *
 * listen_to() is similar to:
 * @code
 * tcs_set_reuse_address(socket_ctx, true);
 * tcs_bind(socket_ctx, local_port);
 * tcs_listen(socket_ctx, TCS_BACKLOG_SOMAXCONN);
 * @endcode
 * Reuse address is not guaranteed to work on all platforms. No error will be returned if it fails.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_listen()
 * @see tcs_accept()
 */
TcsReturnCode tcs_listen_to(TcsSocket socket_ctx, uint16_t local_port);

/**
 * @brief Accept a socket from a listening socket.
 *
 * The accepted socket will get assigned a random local free port.
 * The listening socket will not be affected by this call.
 * 
 * Example usage:
 * @code
 * TcsSocket listen_socket = TCS_NULLSOCKET;
 * tcs_create(&listen_socket, TCS_TYPE_TCP_IP4);
 * tcs_listen_to(listen_socket, 1212);
 * while (true)
 * {
 *   TcsSocket client_socket = TCS_NULLSOCKET;
 *   tcs_accept(listen_socket, &client_socket, NULL)
 *   // Do stuff with client_socket here
 *   tcs_close(&client_socket);
 * }
 * @endcode
 * 
 * @param socket_ctx is your listening socket you used when you called #tcs_listen_to().
 * @param child_socket_ctx is your accepted socket. Must have the in value of #TCS_NULLSOCKET.
 * @param address is an optional pointer to a buffer where the remote address of the accepted socket can be stored.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsReturnCode tcs_accept(TcsSocket socket_ctx, TcsSocket* child_socket_ctx, struct TcsAddress* address);

/**
 * @brief Sends data on a socket, blocking
 *
 * @param socket_ctx is your in-out socket context.
 * @param buffer is a pointer to your data you want to send.
 * @param buffer_size is number of bytes of the data you want to send.
 * @param flags is currently not in use.
 * @param bytes_sent is how many bytes that was successfully sent.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_receive()
 */
TcsReturnCode tcs_send(TcsSocket socket_ctx,
                       const uint8_t* buffer,
                       size_t buffer_size,
                       uint32_t flags,
                       size_t* bytes_sent);

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
 * @see tcs_receive_from()
 * @see tcs_getaddrinfo()
 */
TcsReturnCode tcs_send_to(TcsSocket socket_ctx,
                          const uint8_t* buffer,
                          size_t buffer_size,
                          uint32_t flags,
                          const struct TcsAddress* destination_address,
                          size_t* bytes_sent);

/**
* @brief Sends several data buffers on a socket as one message.
*
* @param socket_ctx is your in-out socket context.
* @param buffers is a pointer to your array of buffers you want to send.
* @param buffer_count is the number of buffers in your array.
* @param flags is currently not in use.
* @param bytes_sent is how many bytes in total that was successfully sent.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsReturnCode tcs_sendv(TcsSocket socket_ctx,
                        const struct TcsBuffer* buffers,
                        size_t buffer_count,
                        uint32_t flags,
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
TcsReturnCode tcs_receive(TcsSocket socket_ctx,
                          uint8_t* buffer,
                          size_t buffer_size,
                          uint32_t flags,
                          size_t* bytes_received);

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
* @see tcs_send_to()
* @see tcs_getaddrinfo()
*/
TcsReturnCode tcs_receive_from(TcsSocket socket_ctx,
                               uint8_t* buffer,
                               size_t buffer_size,
                               uint32_t flags,
                               struct TcsAddress* source_address,
                               size_t* bytes_received);

/**
* @brief Create a context used for waiting on several sockets.
*
* TcsPool can be used to monitor several sockets for events (reading, writing or error).
* Use tcs_pool_poll() to get a list of sockets ready to interact with.
*
* @code
* tcs_lib_init();
* TCS_SOCKET socket1 = TCS_NULLSOCKET;
* TCS_SOCKET socket2 = TCS_NULLSOCKET;
* tcs_create(&socket1, TCS_TYPE_UDP_IP4);
* tcs_create(&socket2, TCS_TYPE_UDP_IP4);
* tcs_bind(socket, 1000)
* tcs_bind(socket, 1001)
*
* struct TcsPool* pool = NULL;
* tcs_pool_create(&pool);
* tcs_pool_add(pool, socket1, NULL, true, false, false); // Only wait for incoming data
* tcs_pool_add(pool, socket2, NULL, true, false, false);
*
* size_t populated = 0;
* TcsPollEvent ev[2] = {TCS_NULLEVENT, TCS_NULLEVENT};
* tcs_pool_poll(pool, ev, 2, &populated, 1000);  // Will wait 1000 ms for data on port 1000 or 1001
* for (int i = 0; i < populated; ++i)
* {
*     if (ev[i].can_read)
*     {
*         uint8_t recv_buffer[8192] = {0};
*         size_t bytes_received = 0;
*         tcs_receive(ev[i].socket, recv_buffer, 8191, TCS_NO_FLAGS, &bytes_received);
*         recv_buffer[bytes_received] = '\n';
*         printf(recv_buffer);
*     }
* }
* tcs_pool_destory(&pool);
* tcs_destroy(&socket1);
* tcs_destroy(&socket2);
* tcs_lib_free();
* @endcode
*
* @param[out] pool is your out pool context pointer. Initiate a TcsPool pointer to NULL and use the address of this pointer.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_pool_destory()
*/
TcsReturnCode tcs_pool_create(struct TcsPool** pool);

/**
* @brief Frees all resources bound to the pool.
*
* Will set @p pool to NULL when successfully.
*
* @param[in, out] pool is your in-out pool context pointer created with tcs_pool_create()
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_pool_create()
*/
TcsReturnCode tcs_pool_destory(struct TcsPool** pool);

/**
* @brief Add a socket to the pool.
*
* @param[in] pool is your in-out pool context pointer created with tcs_pool_create()
* @param socket_ctx will be added to the pool. Note that you can still use it outside of the pool.
* @param[in] user_data is a pointer of your choice that is associated with the socket. Use NULL if not used.
* @param poll_can_read true if you want to poll @p socket_ctx for to be able to read.
* @param poll_can_write true if you want to poll @p socket_ctx for to be able to write.
* @param poll_error true if you want to poll if any error has happened to @p socket_ctx.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_pool_remove()
*/
TcsReturnCode tcs_pool_add(struct TcsPool* pool,
                           TcsSocket socket_ctx,
                           void* user_data,
                           bool poll_can_read,
                           bool poll_can_write,
                           bool poll_error);

/**
* @brief Remove a socket from the pool.
*
* @param[in] pool is a context pointer created with tcs_pool_create()
* @param socket_ctx will be removed from the pool.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_pool_remove()
*/
TcsReturnCode tcs_pool_remove(struct TcsPool* pool, TcsSocket socket_ctx);

/**
* @brief Remove a socket from the pool.
*
* @param[in] pool is your in-out pool context pointer created with @p tcs_pool_create().
* @param[in, out] events is an array with in-out events. Assign each element to #TCS_NULLEVENT.
* @param events_count number of in elements in your events array. Does not make sense to have more events than number of sockets int the pool. If to short, all events may not be returned.
* @param[out] events_populated will contain the number of events the parameter ev has been populated with by the call.
* @param timeout_in_ms is the maximum wait time for any event. If any event happens before this time, the call will return immediately.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_pool_remove()
*/
TcsReturnCode tcs_pool_poll(struct TcsPool* pool,
                            struct TcsPollEvent* events,
                            size_t events_count,
                            size_t* events_populated,
                            int64_t timeout_in_ms);

/**
* @brief Set parameters on a socket. It is recommended to use tcs_set_xxx instead.
*
* @param socket_ctx is your in-out socket context.
* @param level is the definition level.
* @param option_name is the option name.
* @param option_value is a pointer to the option value.
* @param option_size is the byte size of the data pointed by @p option_value.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsReturnCode tcs_set_option(TcsSocket socket_ctx,
                             int32_t level,
                             int32_t option_name,
                             const void* option_value,
                             size_t option_size);

/**
* @brief Get parameters on a socket. It is recommended to use tcs_get_xxx instead.
*
* @code
* uint8_t c;
* size_t a = sizeof(c);
* tcs_get_option(socket, TCS_SOL_IP, TCS_SO_IP_MULTICAST_LOOP, &c, &a);
* @endcode
*
* @param socket_ctx is your in-out socket context.
* @param level is the definition level.
* @param option_name is the option name.
* @param option_value is a pointer to the option value.
* @param option_size is a pointer the byte size of the data pointed by @p option_value.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsReturnCode tcs_get_option(TcsSocket socket_ctx,
                             int32_t level,
                             int32_t option_name,
                             void* option_value,
                             size_t* option_size);

/**
* @brief Turn off communication with a 3-way handshaking for the socket.
* 
* Use this function to cancel blocking calls (recv, accept etc) from another thread, or use sigaction.
* The socket will finish all queued sends first.
*
* @param socket_ctx is your in-out socket context.
* @param direction defines in which direction you want to turn off the communication.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsReturnCode tcs_shutdown(TcsSocket socket_ctx, TcsSocketDirection direction);

/**
* @brief Closes the socket, call this when you are done with the socket.
*
* @param socket_ctx is your in-out socket context.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsReturnCode tcs_destroy(TcsSocket* socket_ctx);

/**
* @brief Get addresses you can connect to given a computer name.
*
* @param hostname null terminated string
* @param address_family filters which address family you want, for example if you only are interested in IPv6. Use #TCS_AF_ANY to not filter.
* @param found_addresses is a pointer to your array which will be populated with found addresses.
* @param found_addresses_max_length is number of elements your @p found_addresses array can store.
* @param no_of_found_addresses will output the number of addresses that was populated in found_addresses.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsReturnCode tcs_resolve_hostname(const char* hostname,
                                   TcsAddressFamily address_family,
                                   struct TcsAddress found_addresses[],
                                   size_t found_addresses_max_length,
                                   size_t* no_of_found_addresses);

/**
* @brief Get local addresses of your computer.
*
* Use NULL for @p found_interfaces and 0 for @p found_interfaces_length to get the total number of addresses found.
*
* @param found_interfaces is a pointer to your array which will be populated with found interfaces.
* @param found_interfaces_length is number of elements your @p found_interfaces array can store.
* @param no_of_found_interfaces will output the number of addresses that was populated in found_interfaces.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsReturnCode tcs_local_interfaces(struct TcsInterface found_interfaces[],
                                   size_t found_interfaces_length,
                                   size_t* no_of_found_interfaces);

/*
* @brief Enable the socket to be allowed to use broadcast.
*
* Only valid for protocols that support broadcast, for example UDP. Default is false.
*
* @param socket_ctx socket to enable/disable permission to send broadcast on.
* @param do_allow_broadcast set to true to allow, false to forbid.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsReturnCode tcs_set_broadcast(TcsSocket socket_ctx, bool do_allow_broadcast);
TcsReturnCode tcs_get_broadcast(TcsSocket socket_ctx, bool* is_broadcast_allowed);

TcsReturnCode tcs_set_keep_alive(TcsSocket socket_ctx, bool do_keep_alive);
TcsReturnCode tcs_get_keep_alive(TcsSocket socket_ctx, bool* is_keep_alive_enabled);

TcsReturnCode tcs_set_reuse_address(TcsSocket socket_ctx, bool do_allow_reuse_address);
TcsReturnCode tcs_get_reuse_address(TcsSocket socket_ctx, bool* is_reuse_address_allowed);

TcsReturnCode tcs_set_send_buffer_size(TcsSocket socket_ctx, size_t send_buffer_size);
TcsReturnCode tcs_get_send_buffer_size(TcsSocket socket_ctx, size_t* send_buffer_size);

TcsReturnCode tcs_set_receive_buffer_size(TcsSocket socket_ctx, size_t receive_buffer_size);
TcsReturnCode tcs_get_receive_buffer_size(TcsSocket socket_ctx, size_t* receive_buffer_size);

TcsReturnCode tcs_set_receive_timeout(TcsSocket socket_ctx, int timeout_ms);
TcsReturnCode tcs_get_receive_timeout(TcsSocket socket_ctx, int* timeout_ms);

TcsReturnCode tcs_set_linger(TcsSocket socket_ctx, bool do_linger, int timeout_seconds);
TcsReturnCode tcs_get_linger(TcsSocket socket_ctx, bool* do_linger, int* timeout_seconds);

TcsReturnCode tcs_set_ip_no_delay(TcsSocket socket_ctx, bool use_no_delay);
TcsReturnCode tcs_get_ip_no_delay(TcsSocket socket_ctx, bool* is_no_delay_used);

TcsReturnCode tcs_set_out_of_band_inline(TcsSocket socket_ctx, bool enable_oob);
TcsReturnCode tcs_get_out_of_band_inline(TcsSocket socket_ctx, bool* is_oob_enabled);

TcsReturnCode tcs_set_ip_multicast_add(TcsSocket socket_ctx,
                                       const struct TcsAddress* local_address,
                                       const struct TcsAddress* multicast_address);

// Use NULL for local_Address for default (make sense? or should we "#define DEFAULT NULL" ?
TcsReturnCode tcs_set_ip_multicast_drop(TcsSocket socket_ctx,
                                        const struct TcsAddress* local_address,
                                        const struct TcsAddress* multicast_address);

/**
* @brief Read up to and including a delimiter.
*
* This function ensures that the socket buffer will keep its data after the delimiter.
* For performance it is recommended to read everything and split it yourself.
* The call will block until the delimiter is received or the supplied buffer is filled.
* The timeout time will not be per call but between each packet received. Longer call time than timeout is possible.
*
* @param socket_ctx is your in-out socket context.
* @param buffer is a pointer to your buffer where you want to store the incoming data to.
* @param buffer_size is the byte size of your buffer, for preventing overflows.
* @param bytes_received is how many bytes that was successfully written to your buffer.
* @param delimiter is your byte value where you want to stop reading. (including delimiter)
* @return #TCS_AGAIN if no delimiter was found and the supplied buffer was filled.
* @return #TCS_SUCCESS if the delimiter was found. Otherwise the error code.
* @see tcs_receive_netstring()
*/
TcsReturnCode tcs_receive_line(TcsSocket socket_ctx,
                               uint8_t* buffer,
                               size_t buffer_size,
                               size_t* bytes_received,
                               uint8_t delimiter);

TcsReturnCode tcs_receive_netstring(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_size, size_t* bytes_received);

TcsReturnCode tcs_send_netstring(TcsSocket socket_ctx, const uint8_t* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
