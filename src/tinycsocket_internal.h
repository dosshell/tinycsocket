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

static const char* const TCS_VERSION_TXT = "v0.4-dev";
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

/*
* List of all functions in the library:
*
* The documentation for each function is located at the declaration in this document.
* Note: Most IDE:s and editors can jump to decalarations directly from here (ctrl+click on symbol or right click -> "Go to Declaration")
* 
* Library Management:
* - TcsResult tcs_lib_init(void);
* - TcsResult tcs_lib_free(void);
*
* Socket Creation:
* - TcsResult tcs_socket(TcsSocket* socket_ctx, TcsAddressFamily family, int type, int protocol);
* - TcsResult tcs_socket_preset(TcsSocket* socket_ctx, TcsPreset socket_type);
* - TcsResult tcs_close(TcsSocket* socket_ctx);
*
* High-level Socket Creation:
* - TcsResult tcs_tcp_server(TcsSocket* socket_ctx, const struct TcsAddress* local_address);
* - TcsResult tcs_tcp_server_str(TcsSocket* socket_ctx, const char* local_address, uint16_t port);
* - TcsResult tcs_tcp_client(TcsSocket* socket_ctx, const struct TcsAddress* remote_address, int timeout_ms);
* - TcsResult tcs_tcp_client_str(TcsSocket* socket_ctx, const char* remote_address, uint16_t port, int timeout_ms);
* - TcsResult tcs_udp_receiver(TcsSocket* socket_ctx, const struct TcsAddress* local_address);
* - TcsResult tcs_udp_receiver_str(TcsSocket* socket_ctx, const char* local_address, uint16_t port);
* - TcsResult tcs_udp_sender(TcsSocket* socket_ctx, const struct TcsAddress* remote_address);
* - TcsResult tcs_udp_sender_str(TcsSocket* socket_ctx, const char* remote_address, uint16_t port);
* - TcsResult tcs_udp_peer(TcsSocket* socket_ctx, const struct TcsAddress* local_address, const struct TcsAddress* remote_address);
* - TcsResult tcs_udp_peer_str(TcsSocket* socket_ctx, const char* local_address, uint16_t local_port, const char* remote_address, uint16_t remote_port);
*
* High-level Raw L2-Packet Sockets (Experimental):
* - TcsResult tcs_packet_sender(TcsSocket* socket_ctx, const struct TcsAddress* remote_address);
* - TcsResult tcs_packet_sender_str(TcsSocket* socket_ctx, const char* interface_name, const uint8_t destination_mac[6], uint16_t protocol);
* - TcsResult tcs_packet_peer(TcsSocket* socket_ctx, const struct TcsAddress* local_address, const struct TcsAddress* remote_address);
* - TcsResult tcs_packet_peer_str(TcsSocket* socket_ctx, const char* interface_name, const uint8_t destination_mac[6], uint16_t protocol);
* - TcsResult tcs_packet_capture_iface(TcsSocket* socket_ctx, const struct TcsInterface* iface);
* - TcsResult tcs_packet_capture_ifname(TcsSocket* socket_ctx, const char* interface_name);
*
* Socket Operations:
* - TcsResult tcs_bind(TcsSocket socket_ctx, const struct TcsAddress* local_address);
* - TcsResult tcs_connect(TcsSocket socket_ctx, const struct TcsAddress* address);
* - TcsResult tcs_connect_str(TcsSocket socket_ctx, const char* remote_address, uint16_t port);
* - TcsResult tcs_listen(TcsSocket socket_ctx, int backlog);
* - TcsResult tcs_accept(TcsSocket socket_ctx, TcsSocket* child_socket_ctx, struct TcsAddress* address);
* - TcsResult tcs_shutdown(TcsSocket socket_ctx, TcsSocketDirection direction);
*
* Data Transfer:
* - TcsResult tcs_send(TcsSocket socket_ctx, const uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_sent);
* - TcsResult tcs_send_to(TcsSocket socket_ctx, const uint8_t* buffer, size_t buffer_size, uint32_t flags, const struct TcsAddress* destination_address, size_t* bytes_sent);
* - TcsResult tcs_sendv(TcsSocket socket_ctx, const struct TcsBuffer* buffers, size_t buffer_count, uint32_t flags, size_t* bytes_sent);
* - TcsResult tcs_send_netstring(TcsSocket socket_ctx, const uint8_t* buffer, size_t buffer_size);
* - TcsResult tcs_receive(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_received);
* - TcsResult tcs_receive_from(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_size, uint32_t flags, struct TcsAddress* source_address, size_t* bytes_received);
* - TcsResult tcs_receive_line(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_size, size_t* bytes_received, uint8_t delimiter);
* - TcsResult tcs_receive_netstring(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_size, size_t* bytes_received);

*
* Socket Pooling:
* - TcsResult tcs_pool_create(struct TcsPool** pool);
* - TcsResult tcs_pool_destroy(struct TcsPool** pool);
* - TcsResult tcs_pool_add(struct TcsPool* pool, TcsSocket socket_ctx, void* user_data, bool poll_can_read, bool poll_can_write, bool poll_error);
* - TcsResult tcs_pool_remove(struct TcsPool* pool, TcsSocket socket_ctx);
* - TcsResult tcs_pool_poll(struct TcsPool* pool, struct TcsPollEvent* events, size_t events_count, size_t* events_populated, int64_t timeout_in_ms);
*
* Socket Options:
* - TcsResult tcs_opt_set(TcsSocket socket_ctx, int32_t level, int32_t option_name, const void* option_value, size_t option_size);
* - TcsResult tcs_opt_get(TcsSocket socket_ctx, int32_t level, int32_t option_name, void* option_value, size_t* option_size);
* - TcsResult tcs_opt_broadcast_set(TcsSocket socket_ctx, bool do_allow_broadcast);
* - TcsResult tcs_opt_broadcast_get(TcsSocket socket_ctx, bool* is_broadcast_allowed);
* - TcsResult tcs_opt_keep_alive_set(TcsSocket socket_ctx, bool do_keep_alive);
* - TcsResult tcs_opt_keep_alive_get(TcsSocket socket_ctx, bool* is_keep_alive_enabled);
* - TcsResult tcs_opt_reuse_address_set(TcsSocket socket_ctx, bool do_allow_reuse_address);
* - TcsResult tcs_opt_reuse_address_get(TcsSocket socket_ctx, bool* is_reuse_address_allowed);
* - TcsResult tcs_opt_send_buffer_size_set(TcsSocket socket_ctx, size_t send_buffer_size);
* - TcsResult tcs_opt_send_buffer_size_get(TcsSocket socket_ctx, size_t* send_buffer_size);
* - TcsResult tcs_opt_receive_buffer_size_set(TcsSocket socket_ctx, size_t receive_buffer_size);
* - TcsResult tcs_opt_receive_buffer_size_get(TcsSocket socket_ctx, size_t* receive_buffer_size);
* - TcsResult tcs_opt_receive_timeout_set(TcsSocket socket_ctx, int timeout_ms);
* - TcsResult tcs_opt_receive_timeout_get(TcsSocket socket_ctx, int* timeout_ms);
* - TcsResult tcs_opt_linger_set(TcsSocket socket_ctx, bool do_linger, int timeout_seconds);
* - TcsResult tcs_opt_linger_get(TcsSocket socket_ctx, bool* do_linger, int* timeout_seconds);
* - TcsResult tcs_opt_ip_no_delay_set(TcsSocket socket_ctx, bool use_no_delay);
* - TcsResult tcs_opt_ip_no_delay_get(TcsSocket socket_ctx, bool* is_no_delay_used);
* - TcsResult tcs_opt_out_of_band_inline_set(TcsSocket socket_ctx, bool enable_oob);
* - TcsResult tcs_opt_out_of_band_inline_get(TcsSocket socket_ctx, bool* is_oob_enabled);
* - TcsResult tcs_opt_priority_set(TcsSocket socket_ctx, int priority);
* - TcsResult tcs_opt_priority_get(TcsSocket socket_ctx, int* priority);
* - TcsResult tcs_opt_nonblocking_set(TcsSocket socket_ctx, bool do_nonblocking);
* - TcsResult tcs_opt_nonblocking_get(TcsSocket socket_ctx, bool* is_nonblocking);
* - TcsResult tcs_opt_membership_add(TcsSocket socket_ctx, const struct TcsAddress* multicast_address);
* - TcsResult tcs_opt_membership_add_to(TcsSocket socket_ctx, const struct TcsAddress* local_address, const struct TcsAddress* multicast_address);
* - TcsResult tcs_opt_membership_drop(TcsSocket socket_ctx, const struct TcsAddress* multicast_address);
* - TcsResult tcs_opt_membership_drop_from(TcsSocket socket_ctx, const struct TcsAddress* local_address, const struct TcsAddress* multicast_address);

*
* Address and Interface Utilities:
* - TcsResult tcs_interface_list(struct TcsInterface interfaces[], size_t capacity, size_t* out_count);
* - TcsResult tcs_address_resolve(const char* hostname, TcsAddressFamily address_family, struct TcsAddress addresses[], size_t capacity, size_t* out_count);
* - TcsResult tcs_address_resolve_timeout(const char* hostname, TcsAddressFamily address_family, struct TcsAddress addresses[], size_t capacity, size_t* out_count, int timeout_ms);
* - TcsResult tcs_address_list(unsigned int interface_id_filter, TcsAddressFamily address_family_filter, struct TcsInterfaceAddress interface_addresses[], size_t capacity, size_t* out_count);
* - TcsResult tcs_address_socket_local(TcsSocket socket_ctx, struct TcsAddress* local_address);
* - TcsResult tcs_address_socket_remote(TcsSocket socket_ctx, struct TcsAddress* remote_address);
* - TcsResult tcs_address_socket_family(TcsSocket socket_ctx, TcsAddressFamily* out_family);
* - TcsResult tcs_address_parse(const char str[], struct TcsAddress* out_address);
* - TcsResult tcs_address_to_str(const struct TcsAddress* address, char out_str[70]);
* - bool tcs_address_is_equal(const struct TcsAddress* l, const struct TcsAddress* r);
* - bool tcs_address_is_any(const struct TcsAddress* addr);
* - bool tcs_address_is_local(const struct TcsAddress* addr);
* - bool tcs_address_is_loopback(const struct TcsAddress* addr);
* - bool tcs_address_is_multicast(const struct TcsAddress* addr);
* - bool tcs_address_is_broadcast(const struct TcsAddress* addr);
*/

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
typedef unsigned int TcsInterfaceId; // TODO: GUID is used for in vista at newer. Change this type.
#elif defined(TINYCSOCKET_USE_POSIX_IMPL)
typedef int TcsSocket;
typedef unsigned int TcsInterfaceId;
#endif

#ifndef TCS_SENDV_MAX
#ifdef TCS_SMALL_STACK
#define TCS_SENDV_MAX 128
#else
#define TCS_SENDV_MAX 1024
#endif
#endif

/**
 * @brief Address Family
 */
typedef enum
{
    TCS_AF_ANY,    /**< Layer 4 agnostic */
    TCS_AF_IP4,    /**< INET IPv4 interface */
    TCS_AF_IP6,    /**< INET IPv6 interface */
    TCS_AF_PACKET, /**< Layer 2 interface */
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
            uint32_t address; /**< Same byte order as the host */
            uint16_t port;    /**< Same byte order as the host */

        } ip4;
        struct
        {
            uint8_t address[16]; /**< Same byte order as the host */
            TcsInterfaceId
                scope_id;  /**< Native type. Only valid for local link addresses. See ::tcs_interface_list(). */
            uint16_t port; /**< Same byte order as the host */
        } ip6;
        struct
        {
            TcsInterfaceId
                interface_id; /**< Local interface index, use tcs_interface_list() to find valid interfaces. Native type. */
            uint16_t protocol; /**< Host byte order. E.i. TCS_ETH_P_ALL, 0 (block all until bind), ETH_P_TSN etc. */
            uint8_t mac[6];    /**< Typical destination mac address or local mac address when joining groups */
        } packet;
    } data;
};

/**
 * @brief Network Interface Information
 */
struct TcsInterface
{
    TcsInterfaceId id;
    char name[32];
};

struct TcsInterfaceAddress
{
    struct TcsInterface iface;
    struct TcsAddress address;
};

// gcc may trigger bug #53119
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif
static const struct TcsAddress TCS_ADDRESS_NONE = {TCS_AF_ANY, {0, 0}};
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

extern const uint32_t TCS_ADDRESS_ANY_IP4;
extern const uint32_t TCS_ADDRESS_LOOPBACK_IP4;
extern const uint32_t TCS_ADDRESS_BROADCAST_IP4;
extern const uint32_t TCS_ADDRESS_NONE_IP4;

/**
 * @brief Used when sending/receiving an array of buffers.
 *
 * Useful if you want to send two or more data arrays at once, for example a header and a body.
 * Make an array of TcsBuffer and use tcs_sendv() to send them all at once.
*/
struct TcsBuffer
{
    const uint8_t* data;
    size_t size;
};

extern const TcsSocket TCS_SOCKET_INVALID; /**< Define new sockets to this value, always. */
static const uint32_t TCS_FLAG_NONE = 0;

// Type
extern const int TCS_SOCK_STREAM; /**< Use for streaming types like TCP */
extern const int TCS_SOCK_DGRAM;  /**< Use for datagrams types like UDP */
extern const int TCS_SOCK_RAW;    /**< Use for raw sockets, eg. layer 2 packet sockets */

// Protocol
extern const uint16_t TCS_PROTOCOL_IP_TCP; /**< Use TCP protocol (use with TCS_SOCK_STREAM for normal cases) */
extern const uint16_t TCS_PROTOCOL_IP_UDP; /**< Use UDP protocol (use with TCS_SOCK_DGRAM for normal cases) */

// Simple socket creation
typedef enum
{
    TCS_PRESET_TCP_IP4,
    TCS_PRESET_UDP_IP4,
    TCS_PRESET_TCP_IP6,
    TCS_PRESET_UDP_IP6,
    TCS_PRESET_PACKET, // Layer 2, CAP_NET_RAW permission may be needed
} TcsPreset;

// Flags
extern const uint32_t TCS_AI_PASSIVE; /**< Use this flag for pure listening sockets */

// Recv flags
extern const uint32_t TCS_MSG_PEEK;
extern const uint32_t TCS_MSG_OOB;
extern const uint32_t TCS_MSG_WAITALL;

// Send flags
extern const uint32_t TCS_MSG_SENDALL;

// Backlog
extern const int TCS_BACKLOG_MAX; /**< Max number of queued sockets when listening */

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
extern const int TCS_SO_PRIORITY;

// IP options
extern const int TCS_SO_IP_NODELAY;
extern const int TCS_SO_IP_MEMBERSHIP_ADD;
extern const int TCS_SO_IP_MEMBERSHIP_DROP;
extern const int TCS_SO_IP_MULTICAST_LOOP;

// Packet options
extern const int TCS_SO_PACKET_MEMBERSHIP_ADD;
extern const int TCS_SO_PACKET_MEMBERSHIP_DROP;

// Use for timeout to wait until infinity happens
extern const int TCS_WAIT_INF;

// Return codes
typedef enum
{
    TCS_SUCCESS = 0,

    /* 1–15: Non-fatal return codes */
    TCS_AGAIN = 1,

    /* -1...-31: General errors */
    TCS_ERROR_UNKNOWN = -1,
    TCS_ERROR_MEMORY = -2,
    TCS_ERROR_INVALID_ARGUMENT = -3,
    TCS_ERROR_SYSTEM = -4, /* OS error not mapped below */
    TCS_ERROR_PERMISSION_DENIED = -5,
    TCS_ERROR_NOT_IMPLEMENTED = -6,

    /* -32...-63: Network and socket errors */
    TCS_ERROR_ADDRESS_LOOKUP_FAILED = -32,
    TCS_ERROR_CONNECTION_REFUSED = -33,
    TCS_ERROR_NOT_CONNECTED = -34,
    TCS_ERROR_SOCKET_CLOSED = -35,
    TCS_ERROR_WOULD_BLOCK = -36,
    TCS_ERROR_TIMED_OUT = -37,

    /* -64...-95: Configuration errors */
    TCS_ERROR_LIBRARY_NOT_INITIALIZED = -64,

    /* -96...-128: Protocol errors */
    TCS_ERROR_ILL_FORMED_MESSAGE = -96,
} TcsResult;

struct TcsPool;
struct TcsPollEvent
{
    TcsSocket socket;
    void* user_data;
    bool can_read;
    bool can_write;
    TcsResult error;
};
static const struct TcsPollEvent TCS_POOL_EVENT_EMPTY = {0, 0, false, false, TCS_SUCCESS};

// ######## Library Management ########

/**
 * @brief Initialize tinycsocket library.
 *
 * This function needs to be called before using any other function in the library.
 *
 * On Windows, it will initialize Winsock, otherwise it does nothing and will always return #TCS_SUCCESS.
 *
 * You can call this multiple times, it will keep a counter of how many times you have called it (RAII friendly).
 * You should call tcs_lib_free() after you are done with the library (before program exit), atleast the number of times you have called tcs_lib_init().
 * 
 * @code
 * #include "tinycsocket.h"
 *
 * int main()
 * {
 *   TcsResult tcs_init_res = tcs_lib_init();
 *   if (tcs_init_res != TCS_SUCCESS)
 *       return -1; // Failed to initialize tinycsocket
 *   // Do stuff with the library here
 *   tcs_lib_free();
 * }
 * @endcode
 *
 * @see tcs_lib_free()
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 *
 * @retval #TCS_ERROR_SYSTEM if a system error occurred. Maybe the Windows version is not supported, or broken.
 * @retval #TCS_ERROR_MEMORY if the library failed to allocate memory.
 */
TcsResult tcs_lib_init(void);

/**
 * @brief De-initialize tinycsocket library.
 *
 * You need to call this the same amount of times as you have called ::tcs_lib_init().
 * 
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 *
 * @retval #TCS_ERROR_LIBRARY_NOT_INITIALIZED if you have not called tcs_lib_init() before calling this function.
 */
TcsResult tcs_lib_free(void);

// ######## Socket Creation ########

/**
 * @brief Create a new socket (BSD-style)
 *
 * This is a thin wrapper around the native socket function.
 * You may want to use ::tcs_socket_preset() instead, or one of the helper functions to create and setup a socket directly:
 *   - ::tcs_tcp_server_str()
 *   - ::tcs_tcp_client_str()
 *   - ::tcs_udp_receiver_str()
 *   - ::tcs_udp_sender_str()
 * 
 * Call ::tcs_close() to stop communication and free all resources for the socket.
 * 
 * @code
 * #include "tinycsocket.h"
 *
 * int main()
 * {
 *   TcsResult tcs_init_res = tcs_lib_init();
 *   if (tcs_init_res != TCS_SUCCESS)
 *       return -1; // Failed to initialize tinycsocket
 *
 *   TcsSocket my_socket = TCS_SOCKET_INVALID; // Always initialize TcsSocket to TCS_SOCKET_INVALID.
 *   TcsResult tcs_socket_res = tcs_socket(&my_socket, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP);
 *   if (tcs_socket_res != TCS_SUCCESS)
 *   {
 *     tcs_lib_free();
 *     return -2; // Failed to create socket
 *   }
 *
 *   // Do stuff with my_socket here. See examples in the documentation.
 *
 *   tcs_close(&my_socket); // Safe to call even if my_socket is TCS_SOCKET_INVALID
 *   tcs_lib_free();
 * }
 * @endcode
 *
 * @param[out] socket_ctx pointer to socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
 * @param[in] family See ::TcsAddressFamily enum for supported values.
 * @param[in] type specifies the type of the socket, supported values are:
 *   - ::TCS_SOCK_STREAM
 *   - ::TCS_SOCK_DGRAM
 *   - ::TCS_SOCK_RAW
 * @param[in] protocol specifies the protocol, for example #TCS_PROTOCOL_IP_TCP or #TCS_PROTOCOL_IP_UDP.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 *
 * @retval #TCS_SUCCESS if successful.
 * @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument. Suck as a socket that is not #TCS_SOCKET_INVALID.
 * @retval #TCS_ERROR_NOT_IMPLEMENTED if you have provided an address family that is not supported on this platform.
 * @retval #TCS_ERROR_PERMISSION_DENIED if you do not have permission to create the socket. E.g. raw sockets often require elevated permissions.
 *
 * @see tcs_socket_preset()
 * @see tcs_tcp_server_str()
 * @see tcs_tcp_client_str()
 * @see tcs_udp_receiver_str()
 * @see tcs_udp_sender_str()
 * @see tcs_close()
 * @see tcs_lib_init()
 * @see tcs_lib_free()
 */
TcsResult tcs_socket(TcsSocket* socket_ctx, TcsAddressFamily family, int type, int protocol);

/**
 * @brief Creates a new socket with simplified options.
 *
 * This is a simple wrapper around tcs_socket() to make it easier to create common socket types.
 * Consider using one of the helper functions instead to create and setup a socket directly:
 *   - ::tcs_tcp_server_str()
 *   - ::tcs_tcp_client_str()
 *   - ::tcs_udp_receiver_str()
 *   - ::tcs_udp_sender_str()
 *
 * @code
 * #include "tinycsocket.h"
 *
 * int main()
 * {
 *   TcsResult tcs_init_res = tcs_lib_init();
 *   if (tcs_init_res != TCS_SUCCESS)
 *       return -1; // Failed to initialize tinycsocket
 *
 *   TcsSocket my_socket = TCS_SOCKET_INVALID; // Always initialize TcsSocket to TCS_SOCKET_INVALID.
 *   TcsResult tcs_socket_res = tcs_socket_preset(&my_socket, TCS_PRESET_TCP_IP4);
 *   if (tcs_socket_res != TCS_SUCCESS)
 *   {
 *     tcs_lib_free();
 *     return -2; // Failed to create socket
 *   }
 *
 *   // Do stuff with my_socket here. See examples in the documentation.
 *
 *   tcs_close(&my_socket); // Safe to call even if my_socket is TCS_SOCKET_INVALID
 *   tcs_lib_free();
 * }
 * @endcode
 *
 * @param[out] socket_ctx pointer to socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
 * @param[in]  socket_type Socket family / transport combination:  
 *   - ::TCS_PRESET_TCP_IP4   — TCP over IPv4  
 *   - ::TCS_PRESET_UDP_IP4   — UDP over IPv4  
 *   - ::TCS_PRESET_TCP_IP6   — TCP over IPv6  
 *   - ::TCS_PRESET_UDP_IP6   — UDP over IPv6  
 *   - ::TCS_PRESET_PACKET    — Raw Layer-2 (may require *CAP_NET_RAW*)
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 *
 * @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument. Suck as a socket value that is not #TCS_SOCKET_INVALID.
 *
 * @see tcs_socket()
 * @see tcs_tcp_server_str()
 * @see tcs_tcp_client_str()
 * @see tcs_udp_receiver_str()
 * @see tcs_udp_sender_str()
 * @see tcs_close()
 * @see tcs_lib_init()
 * @see tcs_lib_free()
 */
TcsResult tcs_socket_preset(TcsSocket* socket_ctx, TcsPreset socket_type);

/**
* @brief Closes the socket, stop communication and free all recourses for the socket.
*
* This will free all resources associated with the socket and set the socket value to #TCS_SOCKET_INVALID.
*
* @param[in,out] socket_ctx is a pointer to your socket context you have previously created with ::tcs_socket() or one of the helper functions. Will be set to #TCS_SOCKET_INVALID on success.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument. Such as a NULL pointer or a socket that is already #TCS_SOCKET_INVALID.
* @retval #TCS_ERROR_SOCKET_CLOSED if the socket is already closed.
*
* @see tcs_socket()
* @see tcs_socket_preset()
* @see tcs_tcp_server_str()
* @see tcs_tcp_client_str()
* @see tcs_udp_receiver_str()
* @see tcs_udp_sender_str()
* @see tcs_lib_init()
* @see tcs_lib_free()
*/
TcsResult tcs_close(TcsSocket* socket_ctx);

// ######## High-level Socket Creation ########

/**
* @brief Setup a socket to listen for incoming tcp connections given an address struct.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   TcsResult tcs_init_res = tcs_lib_init();
*   if (tcs_init_res != TCS_SUCCESS)
*     return -1; // Failed to initialize tinycsocket
*
*   struct TcsAddress local_address = TCS_ADDRESS_NONE;
*   local_address.family = TCS_AF_IP4;
*   local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4; // Bind to all IPv4 interfaces
*   local_address.data.ip4.port = 1212;
*   
*   TcsSocket server_socket = TCS_SOCKET_INVALID; // Always initialize TcsSocket to TCS_SOCKET_INVALID.
*   TcsResult tcs_server_res = tcs_tcp_server_str(&server_socket, &local_address);
*   if (tcs_server_res != TCS_SUCCESS)
*   {
*     tcs_lib_free();
*     return -2; // Failed to create server socket
*   }
*
*   TcsSocket client_socket = TCS_SOCKET_INVALID;
*   TcsResult accept_result = tcs_accept(&server_socket, &client_socket, NULL); // Accept incoming connections
*   if (accept_result != TCS_SUCCESS)
*   {
*     tcs_close(&server_socket);
*     tcs_lib_free();
*     return -3; // Failed to accept incoming connection
*   }
*
*   // Do stuff with client_socket here. See examples in the examples folder.
*
*   tcs_shutdown(&client_socket, TCS_SD_BOTH); // Shutdown the accepted socket
*   tcs_close(&client_socket); // Close the accepted socket
*   tcs_close(&server_socket); // Safe to call even if server_socket is TCS_SOCKET_INVALID
*   tcs_lib_free();
* }
* @endcode
* @param[out] socket_ctx is a pointer to your socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] local_address
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument. Such as a socket value that is not #TCS_SOCKET_INVALID.
*
* @see tcs_socket_preset()
 */
TcsResult tcs_tcp_server(TcsSocket* socket_ctx, const struct TcsAddress* local_address);

/**
* @brief Setup a socket to listen for incoming tcp connections.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   TcsResult tcs_init_res = tcs_lib_init();
*   if (tcs_init_res != TCS_SUCCESS)
*     return -1; // Failed to initialize tinycsocket
*
*   TcsSocket server_socket = TCS_SOCKET_INVALID; // Always initialize TcsSocket to TCS_SOCKET_INVALID.
*   TcsResult tcs_server_res = tcs_tcp_server_str(&server_socket, "0.0.0.0", 1212);
*   if (tcs_server_res != TCS_SUCCESS)
*   {
*     tcs_lib_free();
*     return -2; // Failed to create server socket
*   }
*
*   TcsSocket client_socket = TCS_SOCKET_INVALID;
*   TcsResult accept_result = tcs_accept(&server_socket, &client_socket, NULL); // Accept incoming connections
*   if (accept_result != TCS_SUCCESS)
*   {
*     tcs_close(&server_socket);
*     tcs_lib_free();
*     return -3; // Failed to accept incoming connection
*   }
*
*   // Do stuff with server_socket here. See examples in the documentation.
*
*   tcs_shutdown(&client_socket, TCS_SD_BOTH); // Shutdown the accepted socket
*   tcs_close(&client_socket); // Close the accepted socket
*   tcs_close(&server_socket); // Safe to call even if server_socket is TCS_SOCKET_INVALID
*   tcs_lib_free();
* }
* @endcode
*
* @param[out] socket_ctx is a pointer to your socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] local_address is the local address to bind to. Use NULL or "0.0.0.0" to bind to all interfaces. Port number in string will result in invalid argument error.
* @param[in] port is the local port to bind to. 0 will result in a random free port being assigned.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument. Such as a socket value that is not #TCS_SOCKET_INVALID.
*
* @see tcs_socket()
* @see tcs_socket_preset()
* @see tcs_tcp_client_str()
* @see tcs_close()
* @see tcs_lib_init()
* @see tcs_lib_free()
*/
TcsResult tcs_tcp_server_str(TcsSocket* socket_ctx, const char* local_address, uint16_t port);

/**
* @brief Setup a socket to connect to a remote TCP server using a specific address structure.
*
* This will create a TCP client socket and attempt to connect to the specified remote address.
* The function blocks until connected or until the specified timeout elapses.
* Use tcs_tcp_client_str() if you want to specify the address as a hostname and port instead.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   TcsResult tcs_init_res = tcs_lib_init();
*   if (tcs_init_res != TCS_SUCCESS)
*     return -1;
*
*   struct TcsAddress remote_addr = {0};
*   remote_addr.family = TCS_AF_IP4;
*   remote_addr.data.ip4.address = 0x7F000001; // 127.0.0.1 loopback
*   remote_addr.data.ip4.port = 8080;
*
*   TcsSocket client_socket = TCS_SOCKET_INVALID;
*   TcsResult connect_res = tcs_tcp_client(&client_socket, &remote_addr, 1000); // 1000 milliseconds timeout
*   if (connect_res != TCS_SUCCESS)
*   {
*     tcs_lib_free();
*     return -2;
*   }
*
*   // Use the connected socket
*   uint8_t buffer[] = "Hello, server!";
*   size_t bytes_sent = 0;
*   tcs_send(client_socket, buffer, sizeof(buffer)-1, TCS_MSG_SENDALL, &bytes_sent);
*
*   tcs_close(&client_socket);
*   tcs_lib_free();
* }
* @endcode
*
* @param[out] socket_ctx is a pointer to your socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use
* @param[in] remote_address is the remote address structure to connect to
* @param[in] timeout_ms is the maximum time in milliseconds to wait until connected, use #TCS_WAIT_INF to wait indefinitely
*
* @return #TCS_SUCCESS if successful, otherwise the error code
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument, such as a socket value that is not #TCS_SOCKET_INVALID
* @retval #TCS_ERROR_CONNECTION_REFUSED if the remote server refused the connection
* @retval #TCS_ERROR_TIMED_OUT if the connection attempt timed out
* @retval #TCS_ERROR_ADDRESS_LOOKUP_FAILED if the address could not be resolved
*
* @see tcs_tcp_client_str()
* @see tcs_close()
*/
TcsResult tcs_tcp_client(TcsSocket* socket_ctx, const struct TcsAddress* remote_address, int timeout_ms);

/**
* @brief Setup a socket and connect to a remote TCP server.
*
* This will create a TCP client socket and attempt to connect to the specified remote address.
* The function blocks until connected or until the specified timeout elapses.
* Use tcs_tcp_client() if you want to specify the address as a struct TcsAddress instead.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   TcsResult tcs_init_res = tcs_lib_init();
*   if (tcs_init_res != TCS_SUCCESS)
*     return -1;
*
*   TcsSocket client_socket = TCS_SOCKET_INVALID;
*   TcsResult connect_res = tcs_tcp_client_str(&client_socket, "127.0.0.1", 8080, 1000); // 1000 milliseconds timeout
*   if (connect_res != TCS_SUCCESS)
*   {
*     tcs_lib_free();
*     return -2;
*   }
*
*   // Use the connected socket
*   uint8_t buffer[] = "Hello, server!";
*   size_t bytes_sent = 0;
*   tcs_send(client_socket, buffer, sizeof(buffer)-1, TCS_MSG_SENDALL, &bytes_sent);
*
*   tcs_close(&client_socket);
*   tcs_lib_free();
*   return 0;
* }
* @endcode
*
* @param[out] socket_ctx is a pointer to your socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use
* @param[in] remote_address is the remote address to connect to. Hostname or IP address string.
* @param[in] port is the remote port to connect to
* @param[in] timeout_ms is the maximum time in milliseconds to wait until connected, use #TCS_WAIT_INF to wait indefinitely
*
* @return #TCS_SUCCESS if successful, otherwise the error code
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument, such as a socket value that is not #TCS_SOCKET_INVALID
* @retval #TCS_ERROR_CONNECTION_REFUSED if the remote server refused the connection
* @retval #TCS_ERROR_TIMED_OUT if the connection attempt timed out
* @retval #TCS_ERROR_ADDRESS_LOOKUP_FAILED if the address could not be resolved
*
* @see tcs_tcp_client()
* @see tcs_close()
*/
TcsResult tcs_tcp_client_str(TcsSocket* socket_ctx, const char* remote_address, uint16_t port, int timeout_ms);

/**
* @brief Creates a UDP socket bound to a local address structure for receiving datagrams.
*
* This function creates a UDP socket and binds it to the specified local address structure,
* allowing it to receive incoming UDP datagrams sent to that address/port combination.
* Use tcs_udp_receiver_str() if you want to specify the address as a hostname and port instead.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   TcsResult tcs_init_res = tcs_lib_init();
*   if (tcs_init_res != TCS_SUCCESS)
*     return -1; // Failed to initialize tinycsocket
*
*   struct TcsAddress local_address = TCS_ADDRESS_NONE;
*   local_address.family = TCS_AF_IP4;
*   local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4; // Bind to all interfaces
*   local_address.data.ip4.port = 8888;
*   
*   TcsSocket receiver = TCS_SOCKET_INVALID; // Always initialize TcsSocket to TCS_SOCKET_INVALID
*   TcsResult udp_res = tcs_udp_receiver(&receiver, &local_address);
*   if (udp_res != TCS_SUCCESS)
*   {
*     tcs_lib_free();
*     return -2; // Failed to create UDP receiver
*   }
*
*   // Receive data
*   uint8_t buffer[2048];
*   size_t bytes_received = 0;
*   struct TcsAddress sender_address;
*   
*   TcsResult recv_res = tcs_receive_from(receiver, buffer, sizeof(buffer), 
*                                       TCS_FLAG_NONE, &sender_address, &bytes_received);
*   if (recv_res == TCS_SUCCESS && bytes_received > 0)
*   {
*     // Process received data...
*   }
*
*   tcs_close(&receiver);
*   tcs_lib_free();
*   return 0;
* }
* @endcode
*
* @param[out] socket_ctx is a pointer to your socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] local_address is the local address structure to bind to.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument, such as a socket value that is not #TCS_SOCKET_INVALID.
* @retval #TCS_ERROR_PERMISSION_DENIED if binding to the specified address/port requires elevated privileges.
*
* @see tcs_udp_receiver_str()
* @see tcs_udp_sender()
* @see tcs_udp_peer()
* @see tcs_receive_from()
* @see tcs_close()
*/
TcsResult tcs_udp_receiver(TcsSocket* socket_ctx, const struct TcsAddress* local_address);

/**
* @brief Creates a UDP socket bound to a local address for receiving datagrams.
*
* This function creates a UDP socket and binds it to the specified local address and port,
* allowing it to receive incoming UDP datagrams sent to that address/port combination.
* Use tcs_udp_receiver() if you want to specify the address as a TcsAddress structure.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   TcsResult tcs_init_res = tcs_lib_init();
*   if (tcs_init_res != TCS_SUCCESS)
*     return -1; // Failed to initialize tinycsocket
*
*   TcsSocket receiver = TCS_SOCKET_INVALID; // Always initialize TcsSocket to TCS_SOCKET_INVALID
*   TcsResult udp_res = tcs_udp_receiver_str(&receiver, "0.0.0.0", 8888);
*   if (udp_res != TCS_SUCCESS)
*   {
*     tcs_lib_free();
*     return -2; // Failed to create UDP receiver
*   }
*
*   // Receive data
*   uint8_t buffer[2048];
*   size_t bytes_received = 0;
*   struct TcsAddress remote_address;
*   
*   TcsResult recv_res = tcs_receive_from(receiver, buffer, sizeof(buffer), TCS_FLAG_NONE, &remote_address, &bytes_received);
*   if (recv_res == TCS_SUCCESS)
*   {
*     // Process received data...
*   }
*
*   tcs_close(&receiver);
*   tcs_lib_free();
*   return 0;
* }
* @endcode
*
* @param[out] socket_ctx is a pointer to your socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] local_address is the local address to bind to. Use NULL or "0.0.0.0" to bind to all interfaces.
* @param[in] port is the local port to bind to. 0 will result in a random free port being assigned.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument, such as a socket value that is not #TCS_SOCKET_INVALID.
* @retval #TCS_ERROR_PERMISSION_DENIED if binding to the specified address/port requires elevated privileges.
* @retval #TCS_ERROR_ADDRESS_LOOKUP_FAILED if the local address could not be resolved.
*
* @see tcs_udp_receiver()
* @see tcs_udp_sender_str()
* @see tcs_udp_peer_str()
* @see tcs_receive_from()
* @see tcs_close()
*/
TcsResult tcs_udp_receiver_str(TcsSocket* socket_ctx, const char* local_address, uint16_t port);

/**
* @brief Creates a UDP socket configured to send datagrams to a specific remote address structure.
*
* This function creates a UDP socket that's pre-configured to send datagrams to the specified
* remote address structure. The socket is not bound to a specific local address, so the OS
* will automatically assign a local address and port when sending data.
* Use tcs_udp_sender_str() if you want to specify the address as a hostname and port instead.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   TcsResult tcs_init_res = tcs_lib_init();
*   if (tcs_init_res != TCS_SUCCESS)
*     return -1; // Failed to initialize tinycsocket
*
*   struct TcsAddress remote_address = TCS_ADDRESS_NONE;
*   remote_address.family = TCS_AF_IP4;
*   remote_address.data.ip4.address = 0x7F000001; // 127.0.0.1 loopback
*   remote_address.data.ip4.port = 8888;
*   
*   TcsSocket sender = TCS_SOCKET_INVALID; // Always initialize TcsSocket to TCS_SOCKET_INVALID
*   TcsResult udp_res = tcs_udp_sender(&sender, &remote_address);
*   if (udp_res != TCS_SUCCESS)
*   {
*     tcs_lib_free();
*     return -2; // Failed to create UDP sender
*   }
*
*   // Send data
*   uint8_t buffer[] = "Hello, UDP receiver!";
*   size_t bytes_sent = 0;
*   
*   TcsResult send_res = tcs_send(sender, buffer, sizeof(buffer)-1, TCS_FLAG_NONE, &bytes_sent);
*   if (send_res == TCS_SUCCESS)
*   {
*     // Data sent successfully
*   }
*
*   tcs_close(&sender);
*   tcs_lib_free();
*   return 0;
* }
* @endcode
*
* @param[out] socket_ctx is a pointer to your socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] remote_address is the remote address structure to send to.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument, such as a socket value that is not #TCS_SOCKET_INVALID.
*
* @see tcs_udp_sender_str()
* @see tcs_udp_receiver()
* @see tcs_udp_peer()
* @see tcs_send()
* @see tcs_close()
*/
TcsResult tcs_udp_sender(TcsSocket* socket_ctx, const struct TcsAddress* remote_address);

/**
* @brief Creates a UDP socket configured to send datagrams to a specific remote address.
*
* This function creates a UDP socket that's pre-configured to send datagrams to the specified
* remote address and port. The socket is not bound to a specific local address, so the OS
* will automatically assign a local address and port when sending data.
* Use tcs_udp_sender() if you want to specify the address as a TcsAddress structure.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   TcsResult tcs_init_res = tcs_lib_init();
*   if (tcs_init_res != TCS_SUCCESS)
*     return -1; // Failed to initialize tinycsocket
*
*   TcsSocket sender = TCS_SOCKET_INVALID; // Always initialize TcsSocket to TCS_SOCKET_INVALID
*   TcsResult udp_res = tcs_udp_sender_str(&sender, "127.0.0.1", 8888);
*   if (udp_res != TCS_SUCCESS)
*   {
*     tcs_lib_free();
*     return -2; // Failed to create UDP sender
*   }
*
*   // Send data
*   uint8_t buffer[] = "Hello, UDP receiver!";
*   size_t bytes_sent = 0;
*   
*   TcsResult send_res = tcs_send(sender, buffer, sizeof(buffer)-1, TCS_FLAG_NONE, &bytes_sent);
*   if (send_res == TCS_SUCCESS)
*   {
*     // Data sent successfully
*   }
*
*   tcs_close(&sender);
*   tcs_lib_free();
*   return 0;
* }
* @endcode
*
* @param[out] socket_ctx is a pointer to your socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] remote_address is the remote address to send to. Hostname or IP address string.
* @param[in] port is the remote port to send to.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument, such as a socket value that is not #TCS_SOCKET_INVALID.
* @retval #TCS_ERROR_ADDRESS_LOOKUP_FAILED if the remote address could not be resolved.
*
* @see tcs_udp_sender()
* @see tcs_udp_receiver_str()
* @see tcs_udp_peer_str()
* @see tcs_send()
* @see tcs_close()
*/
TcsResult tcs_udp_sender_str(TcsSocket* socket_ctx, const char* remote_address, uint16_t port);

/**
* @brief Creates a UDP socket bound to a local address structure that can send to a specific remote address structure.
*
* This function creates a UDP socket that's both bound to a local address structure for receiving
* datagrams and pre-configured to send datagrams to a specified remote address structure. This
* creates a complete bidirectional UDP communication channel in a single call.
* Use tcs_udp_peer_str() if you want to specify the addresses as hostname and port pairs instead.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   TcsResult tcs_init_res = tcs_lib_init();
*   if (tcs_init_res != TCS_SUCCESS)
*     return -1; // Failed to initialize tinycsocket
*
*   struct TcsAddress local_address = TCS_ADDRESS_NONE;
*   local_address.family = TCS_AF_IP4;
*   local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4; // Bind to all interfaces
*   local_address.data.ip4.port = 8888;
*   
*   struct TcsAddress remote_address = TCS_ADDRESS_NONE;
*   remote_address.family = TCS_AF_IP4;
*   remote_address.data.ip4.address = 0x7F000001; // 127.0.0.1 loopback
*   remote_address.data.ip4.port = 9999;
*   
*   TcsSocket peer = TCS_SOCKET_INVALID; // Always initialize TcsSocket to TCS_SOCKET_INVALID
*   TcsResult udp_res = tcs_udp_peer(&peer, &local_address, &remote_address);
*   if (udp_res != TCS_SUCCESS)
*   {
*     tcs_lib_free();
*     return -2; // Failed to create UDP peer
*   }
*
*   // Send data - goes to the pre-configured remote address
*   uint8_t send_buffer[] = "Hello, remote peer!";
*   size_t bytes_sent = 0;
*   tcs_send(peer, send_buffer, sizeof(send_buffer)-1, TCS_FLAG_NONE, &bytes_sent);
*
*   // Receive data from any sender
*   uint8_t recv_buffer[2048];
*   size_t bytes_received = 0;
*   struct TcsAddress sender_address;
*   tcs_receive_from(peer, recv_buffer, sizeof(recv_buffer), TCS_FLAG_NONE, 
*                   &sender_address, &bytes_received);
*
*   tcs_close(&peer);
*   tcs_lib_free();
*   return 0;
* }
* @endcode
*
* @param[out] socket_ctx is a pointer to your socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] local_address is the local address structure to bind to.
* @param[in] remote_address is the remote address structure to send to.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument, such as a socket value that is not #TCS_SOCKET_INVALID.
* @retval #TCS_ERROR_PERMISSION_DENIED if binding to the specified address/port requires elevated privileges.
*
* @see tcs_udp_peer_str()
* @see tcs_udp_receiver()
* @see tcs_udp_sender()
* @see tcs_send()
* @see tcs_receive_from()
* @see tcs_close()
*/
TcsResult tcs_udp_peer(TcsSocket* socket_ctx,
                       const struct TcsAddress* local_address,
                       const struct TcsAddress* remote_address);

/**
* @brief Creates a UDP socket bound to a local address that can send to a specific remote address.
*
* This function creates a UDP socket that's both bound to a local address/port for receiving
* datagrams and pre-configured to send datagrams to a specified remote address/port. This
* creates a complete bidirectional UDP communication channel in a single call.
* Use tcs_udp_peer() if you want to specify the addresses as TcsAddress structures.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   TcsResult tcs_init_res = tcs_lib_init();
*   if (tcs_init_res != TCS_SUCCESS)
*     return -1; // Failed to initialize tinycsocket
*
*   TcsSocket peer = TCS_SOCKET_INVALID; // Always initialize TcsSocket to TCS_SOCKET_INVALID
*   TcsResult udp_res = tcs_udp_peer_str(&peer, "0.0.0.0", 8888, "192.168.1.100", 9999);
*   if (udp_res != TCS_SUCCESS)
*   {
*     tcs_lib_free();
*     return -2; // Failed to create UDP peer
*   }
*
*   // Send data - goes to the pre-configured remote address
*   uint8_t send_buffer[] = "Hello, remote peer!";
*   size_t bytes_sent = 0;
*   tcs_send(peer, send_buffer, sizeof(send_buffer)-1, TCS_FLAG_NONE, &bytes_sent);
*
*   // Receive data from any sender
*   uint8_t recv_buffer[2048];
*   size_t bytes_received = 0;
*   struct TcsAddress sender_address;
*   tcs_receive_from(peer, recv_buffer, sizeof(recv_buffer), TCS_FLAG_NONE, 
*                   &sender_address, &bytes_received);
*
*   tcs_close(&peer);
*   tcs_lib_free();
*   return 0;
* }
* @endcode
*
* @param[out] socket_ctx is a pointer to your socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] local_address is the local address to bind to. Use NULL or "0.0.0.0" to bind to all interfaces.
* @param[in] local_port is the local port to bind to. 0 will result in a random free port being assigned.
* @param[in] remote_address is the remote address to send to. Hostname or IP address string.
* @param[in] remote_port is the remote port to send to.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument, such as a socket value that is not #TCS_SOCKET_INVALID.
* @retval #TCS_ERROR_PERMISSION_DENIED if binding to the specified address/port requires elevated privileges.
* @retval #TCS_ERROR_ADDRESS_LOOKUP_FAILED if the local or remote address could not be resolved.
*
* @see tcs_udp_peer()
* @see tcs_udp_receiver_str()
* @see tcs_udp_sender_str()
* @see tcs_send()
* @see tcs_receive_from()
* @see tcs_close()
*/
TcsResult tcs_udp_peer_str(TcsSocket* socket_ctx,
                           const char* local_address,
                           uint16_t local_port,
                           const char* remote_address,
                           uint16_t remote_port);

// ######## High-level Raw L2-Packet Sockets (Experimental) ########

/**
 * @brief Create a raw packet socket targeting a specific remote address.
 *
 * @warning This API is **experimental** and not recommended for production use.
 *
 * @param[out] socket_ctx     Pointer to a #TcsSocket handle that will be initialized on success. Must be set to
 *                            #TCS_SOCKET_INVALID before the call.
 * @param[in]  remote_address Pointer to the remote ::TcsAddress to which the socket will send packets.
 *
 * @retval TCS_SUCCESS        Socket created successfully.
 * @retval TCS_ERR_PERMISSION Operation not permitted (may require CAP_NET_RAW).
 * @retval TCS_ERR_INVALID    The supplied address was invalid or unsupported.
 * @retval TCS_ERR_SYS        Underlying OS error.
 *
 * @see tcs_packet_sender_str()
 */
TcsResult tcs_packet_sender(TcsSocket* socket_ctx, const struct TcsAddress* remote_address);

/**
 * @brief Create a raw packet socket for Ethernet frame transmission given interface name, destination MAC, and EtherType.
 *
 * @warning This API is **experimental** and must not be used in production.
 *
 * @param[out] socket_ctx       Pointer to a #TcsSocket handle that will be initialized on success. Must be set to
 *                              #TCS_SOCKET_INVALID before the call.
 * @param[in]  interface_name   Null-terminated name of the network interface to bind (e.g. `"eth0"`).
 * @param[in]  destination_mac  Destination hardware address (6 bytes).
 * @param[in]  protocol         EtherType value in host byte order.
 *
 * @retval TCS_SUCCESS          Socket created successfully.
 * @retval TCS_ERR_PERMISSION   Operation not permitted (may require CAP_NET_RAW).
 * @retval TCS_ERR_NOT_FOUND    Interface not found.
 * @retval TCS_ERR_SYS          Underlying OS error.
 */
TcsResult tcs_packet_sender_str(TcsSocket* socket_ctx,
                                const char* interface_name,
                                const uint8_t destination_mac[6],
                                uint16_t protocol);

/**
 * @brief Create a raw packet socket bound to a local/remote address pair.
 *
 * @warning This API is **experimental** and not recommended for production use.
 *
 * @param[out] socket_ctx     Pointer to a #TcsSocket handle that will be initialized on success. Must be set to
 *                            #TCS_SOCKET_INVALID before the call.
 * @param[in]  local_address  Pointer to a ::TcsAddress that specifies the local endpoint to bind.
 * @param[in]  remote_address Pointer to a ::TcsAddress that specifies the
 *                            remote peer address.
 *
 * @retval TCS_SUCCESS        Socket created successfully.
 * @retval TCS_ERR_PERMISSION Operation not permitted (may require CAP_NET_RAW).
 * @retval TCS_ERR_NOT_FOUND  Interface or address not found.
 * @retval TCS_ERR_INVALID    One or more addresses were invalid.
 * @retval TCS_ERR_SYS        Underlying OS error.
 *
 * @see tcs_packet_peer_str()
 * @see tcs_packet_sender_str()
 * @see tcs_packet_sender()
 */
TcsResult tcs_packet_peer(TcsSocket* socket_ctx,
                          const struct TcsAddress* local_address,
                          const struct TcsAddress* remote_address);

/**
 * @brief Create a raw packet socket bound to a specific peer.
 *
 * @warning This API is **experimental** and not recommended for production use.
 *
 * @param[out] socket_ctx       Pointer to a #TcsSocket handle that will be initialized on success. Must be set to
 *                              #TCS_SOCKET_INVALID before the call.
 * @param[in]  interface_name   Null-terminated name of the network interface
 *                              to bind (e.g. `"eth0"`).
 * @param[in]  destination_mac  Destination hardware address (6 bytes).
 * @param[in]  protocol         EtherType value in host byte order.
 *
 * @retval TCS_SUCCESS          Socket created successfully.
 * @retval TCS_ERR_PERMISSION   Operation not permitted (may require CAP_NET_RAW).
 * @retval TCS_ERR_NOT_FOUND    Interface not found.
 * @retval TCS_ERR_INVALID      Invalid arguments (e.g. malformed MAC).
 * @retval TCS_ERR_SYS          Underlying OS error.
 *
 * @see tcs_packet_sender_str()
 * @see tcs_packet_sender()
 */
TcsResult tcs_packet_peer_str(TcsSocket* socket_ctx,
                              const char* interface_name,
                              const uint8_t destination_mac[6],
                              uint16_t protocol);

/**
 * @brief Create a raw packet socket in capture mode on a given interface.
 *
 * @warning This API is **experimental** and not recommended for production use.
 *
 * @param[out] socket_ctx  Pointer to a #TcsSocket handle that will be initialized on success. Must be set to
 *                         #TCS_SOCKET_INVALID before the call.
 * @param[in]  interface   Pointer to a ::TcsInterface structure that identifies the interface to capture from.
 *
 * @retval TCS_SUCCESS          Socket created successfully.
 * @retval TCS_ERR_PERMISSION   Operation not permitted (may require CAP_NET_RAW).
 * @retval TCS_ERR_NOT_FOUND    Interface not found or not available.
 * @retval TCS_ERR_INVALID      Invalid argument(s).
 * @retval TCS_ERR_SYS          Underlying OS error.
 *
 * @see tcs_packet_capture()
 * @see tcs_packet_peer_str()
 */
TcsResult tcs_packet_capture_iface(TcsSocket* socket_ctx, const struct TcsInterface* iface);

/**
 * @brief Create a raw packet socket in capture mode on a given interface using its name.
 *
 * @warning This API is **experimental** and not recommended for production use.
 *
 * @param[out] socket_ctx      Pointer to a #TcsSocket handle that will be initialized on success. Must be set to
 *                             #TCS_SOCKET_INVALID before the call.
 * @param[in]  interface_name  Null-terminated name of the network interface to capture from (e.g. "eth0", "wlan0").
 *
 * @retval TCS_SUCCESS                  Socket created successfully.
 * @retval TCS_ERROR_PERMISSION_DENIED  Operation not permitted (may require CAP_NET_RAW).
 * @retval TCS_ERROR_INVALID_ARGUMENT   Invalid argument(s) provided.
 * @retval TCS_ERROR_SYSTEM             Interface not found or other underlying OS error.
 *
 * @see tcs_packet_capture_iface()
 * @see tcs_interface_list()
 * @see tcs_receive_from()
 */
TcsResult tcs_packet_capture_ifname(TcsSocket* socket_ctx, const char* interface_name);

/**
 * @brief Binds a socket to a local address.
 *
 * This function associates a socket with a specific local address and port, allowing it to
 * receive incoming connections or datagrams on that address. For TCP server sockets, you
 * must call this before tcs_listen(). For UDP sockets, binding determines which address
 * and port the socket will receive datagrams on.
 *
 * @code
 * #include "tinycsocket.h"
 * int main()
 * {
 *   TcsResult tcs_init_res = tcs_lib_init();
 *   if (tcs_init_res != TCS_SUCCESS)
 *     return -1;
 *
 *   TcsSocket server_socket = TCS_SOCKET_INVALID;
 *   TcsResult socket_res = tcs_socket_preset(&server_socket, TCS_PRESET_TCP_IP4);
 *   if (socket_res != TCS_SUCCESS)
 *   {
 *     tcs_lib_free();
 *     return -2;
 *   }
 *
 *   struct TcsAddress local_address = TCS_ADDRESS_NONE;
 *   local_address.family = TCS_AF_IP4;
 *   local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4; // Bind to all interfaces
 *   local_address.data.ip4.port = 8080;
 *
 *   TcsResult bind_res = tcs_bind(server_socket, &local_address);
 *   if (bind_res != TCS_SUCCESS)
 *   {
 *     tcs_close(&server_socket);
 *     tcs_lib_free();
 *     return -3; // Failed to bind to address
 *   }
 *
 *   // For TCP: now call tcs_listen()
 *   // For UDP: socket is ready to receive datagrams
 *
 *   tcs_close(&server_socket);
 *   tcs_lib_free();
 *   return 0;
 * }
 * @endcode
 *
 * @param socket_ctx The socket to bind. Must be a valid socket created with tcs_socket() or tcs_socket_preset().
 * @param local_address The local address structure to bind to. Use TCS_ADDRESS_ANY_IP4 for the address field to bind to all interfaces.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @retval #TCS_ERROR_INVALID_ARGUMENT if socket_ctx is invalid or local_address is NULL.
 * @retval #TCS_ERROR_PERMISSION_DENIED if binding to the specified address/port requires elevated privileges.
 * @retval #TCS_ERROR_SYSTEM if the address is already in use or another system error occurred.
 *
 * @see tcs_listen()
 * @see tcs_tcp_server_str()
 * @see tcs_udp_receiver_str()
 * @see tcs_address_socket_local()
 */
TcsResult tcs_bind(TcsSocket socket_ctx, const struct TcsAddress* local_address);

/**
 * @brief Connect a socket to a remote address structure.
 *
 * This function establishes a connection to the specified remote address structure.
 * For TCP sockets, this initiates a three-way handshake. For UDP sockets, this
 * associates the socket with the remote address for subsequent send operations.
 * The function blocks indefinitely until the connection is established or fails.
 * Timeout for this function is set by OS defaults. Use TcsPool or
 * ::tcs_opt_nonblocking_set() for non-blocking behavior.
 *
 * @code
 * #include "tinycsocket.h"
 * int main()
 * {
 *   TcsResult tcs_init_res = tcs_lib_init();
 *   if (tcs_init_res != TCS_SUCCESS)
 *     return -1;
 *
 *   TcsSocket client_socket = TCS_SOCKET_INVALID;
 *   TcsResult socket_res = tcs_socket_preset(&client_socket, TCS_PRESET_TCP_IP4);
 *   if (socket_res != TCS_SUCCESS)
 *   {
 *     tcs_lib_free();
 *     return -2;
 *   }
 *
 *   struct TcsAddress remote_address = TCS_ADDRESS_NONE;
 *   remote_address.family = TCS_AF_IP4;
 *   remote_address.data.ip4.address = 0x7F000001; // 127.0.0.1 loopback
 *   remote_address.data.ip4.port = 8080;
 *
 *   TcsResult connect_res = tcs_connect(client_socket, &remote_address);
 *   if (connect_res != TCS_SUCCESS)
 *   {
 *     tcs_close(&client_socket);
 *     tcs_lib_free();
 *     return -3; // Failed to connect
 *   }
 *
 *   // Socket is now connected and ready for communication
 *   uint8_t buffer[] = "Hello, server!";
 *   size_t bytes_sent = 0;
 *   tcs_send(client_socket, buffer, sizeof(buffer)-1, TCS_MSG_SENDALL, &bytes_sent);
 *
 *   tcs_close(&client_socket);
 *   tcs_lib_free();
 *   return 0;
 * }
 * @endcode
 *
 * @param socket_ctx The socket to connect. Must be a valid socket created with tcs_socket() or tcs_socket_preset().
 * @param address The remote address structure to connect to.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @retval #TCS_ERROR_INVALID_ARGUMENT if socket_ctx is invalid or address is NULL.
 * @retval #TCS_ERROR_CONNECTION_REFUSED if the remote server refused the connection.
 * @retval #TCS_ERROR_TIMED_OUT if the connection attempt timed out (can take 3+ minutes for unreachable hosts).
 * @retval #TCS_ERROR_SYSTEM if another system error occurred.
 *
 * @see tcs_connect_str()
 * @see tcs_tcp_client()
 * @see tcs_bind()
 * @see tcs_listen()
 */
TcsResult tcs_connect(TcsSocket socket_ctx, const struct TcsAddress* address);

/**
 * @brief Connect a socket to a remote hostname and port.
 *
 * This function establishes a connection to the specified remote address and port.
 * For TCP sockets, this initiates a three-way handshake. For UDP sockets, this
 * associates the socket with the remote address for subsequent send operations.
 * Timeout for this function is set by OS defaults. Use TcsPool or
 * ::tcs_opt_nonblocking_set() for non-blocking behavior.
 *
 * @code
 * #include "tinycsocket.h"
 * int main()
 * {
 *   TcsResult tcs_init_res = tcs_lib_init();
 *   if (tcs_init_res != TCS_SUCCESS)
 *     return -1;
 *
 *   TcsSocket client_socket = TCS_SOCKET_INVALID;
 *   TcsResult socket_res = tcs_socket_preset(&client_socket, TCS_PRESET_TCP_IP4);
 *   if (socket_res != TCS_SUCCESS)
 *   {
 *     tcs_lib_free();
 *     return -2;
 *   }
 *
 *   TcsResult connect_res = tcs_connect_str(client_socket, "192.168.1.100", 8080);
 *   if (connect_res != TCS_SUCCESS)
 *   {
 *     tcs_close(&client_socket);
 *     tcs_lib_free();
 *     return -3; // Failed to connect
 *   }
 *
 *   // Socket is now connected and ready for communication
 *   uint8_t buffer[] = "Hello, server!";
 *   size_t bytes_sent = 0;
 *   tcs_send(client_socket, buffer, sizeof(buffer)-1, TCS_MSG_SENDALL, &bytes_sent);
 *
 *   tcs_close(&client_socket);
 *   tcs_lib_free();
 *   return 0;
 * }
 * @endcode
 *
 * @param socket_ctx The socket to connect. Must be a valid socket created with tcs_socket() or tcs_socket_preset().
 * @param remote_address The remote hostname or IP address to connect to.
 * @param port The remote port number to connect to.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @retval #TCS_ERROR_INVALID_ARGUMENT if socket_ctx is invalid or remote_address is NULL.
 * @retval #TCS_ERROR_CONNECTION_REFUSED if the remote server refused the connection.
 * @retval #TCS_ERROR_ADDRESS_LOOKUP_FAILED if the hostname could not be resolved.
 * @retval #TCS_ERROR_TIMED_OUT if the connection attempt timed out.
 * @retval #TCS_ERROR_SYSTEM if another system error occurred.
 *
 * @see tcs_connect()
 * @see tcs_tcp_client_str()
 * @see tcs_bind()
 * @see tcs_listen()
 */
TcsResult tcs_connect_str(TcsSocket socket_ctx, const char* remote_address, uint16_t port);

/**
 * @brief Let a socket start listening for incoming connections.
 *
 * Call #tcs_bind() first to bind to a local address to listening at.
 *
 * @param socket_ctx is your in-out socket context.
 * @param backlog is the maximum number of queued incoming sockets. Use #TCS_BACKLOG_MAX to set it to max.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_accept()
 * @see tcs_listen_to()
 */
TcsResult tcs_listen(TcsSocket socket_ctx, int backlog);

/**
 * @brief Accept a socket from a listening socket.
 *
 * The accepted socket will get assigned a random local free port.
 * The listening socket will not be affected by this call.
 * 
 * Example usage:
 * @code
 * TcsSocket listen_socket = TCS_NULLSOCKET;
 * tcs_create(&listen_socket, TCS_PRESET_TCP_IP4);
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
 * @param socket_ctx is your listening socket you used when you called ::tcs_listen().
 * @param child_socket_ctx is your accepted socket. Must have the in value of #TCS_SOCKET_INVALID.
 * @param address is an optional pointer to a buffer where the remote address of the accepted socket can be stored.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsResult tcs_accept(TcsSocket socket_ctx, TcsSocket* child_socket_ctx, struct TcsAddress* address);

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
TcsResult tcs_shutdown(TcsSocket socket_ctx, TcsSocketDirection direction);

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
TcsResult tcs_send(TcsSocket socket_ctx, const uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_sent);

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
TcsResult tcs_send_to(TcsSocket socket_ctx,
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
TcsResult tcs_sendv(TcsSocket socket_ctx,
                    const struct TcsBuffer* buffers,
                    size_t buffer_count,
                    uint32_t flags,
                    size_t* bytes_sent);

TcsResult tcs_send_netstring(TcsSocket socket_ctx, const uint8_t* buffer, size_t buffer_size);

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
TcsResult tcs_receive(TcsSocket socket_ctx,
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
TcsResult tcs_receive_from(TcsSocket socket_ctx,
                           uint8_t* buffer,
                           size_t buffer_size,
                           uint32_t flags,
                           struct TcsAddress* source_address,
                           size_t* bytes_received);

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
TcsResult tcs_receive_line(TcsSocket socket_ctx,
                           uint8_t* buffer,
                           size_t buffer_size,
                           size_t* bytes_received,
                           uint8_t delimiter);

TcsResult tcs_receive_netstring(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_size, size_t* bytes_received);

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
* tcs_create(&socket1, TCS_PRESET_UDP_IP4);
* tcs_create(&socket2, TCS_PRESET_UDP_IP4);
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
* tcs_pool_destroy(&pool);
* tcs_close(&socket1);
* tcs_close(&socket2);
* tcs_lib_free();
* @endcode
*
* @param[out] pool is your out pool context pointer. Initiate a TcsPool pointer to NULL and use the address of this pointer.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_pool_destroy()
*/
TcsResult tcs_pool_create(struct TcsPool** pool);

/**
* @brief Frees all resources bound to the pool.
*
* Will set @p pool to NULL when successfully.
*
* @param[in, out] pool is your in-out pool context pointer created with tcs_pool_create()
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_pool_create()
*/
TcsResult tcs_pool_destroy(struct TcsPool** pool);

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
TcsResult tcs_pool_add(struct TcsPool* pool,
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
TcsResult tcs_pool_remove(struct TcsPool* pool, TcsSocket socket_ctx);

/**
* @brief Remove a socket from the pool.
*
* @param[in] pool is your in-out pool context pointer created with @p tcs_pool_create().
* @param[in, out] events is an array with in-out events. Assign each element to #TCS_EVENT_NONE.
* @param events_count number of in elements in your events array. Does not make sense to have more events than number of sockets int the pool. If to short, all events may not be returned.
* @param[out] events_populated will contain the number of events the parameter ev has been populated with by the call.
* @param timeout_in_ms is the maximum wait time for any event. If any event happens before this time, the call will return immediately.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_pool_remove()
*/
TcsResult tcs_pool_poll(struct TcsPool* pool,
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
TcsResult tcs_opt_set(TcsSocket socket_ctx,
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
* tcs_opt_get(socket, TCS_SOL_IP, TCS_SO_IP_MULTICAST_LOOP, &c, &a);
* @endcode
*
* @param socket_ctx is your in-out socket context.
* @param level is the definition level.
* @param option_name is the option name.
* @param option_value is a pointer to the option value.
* @param option_size is a pointer the byte size of the data pointed by @p option_value.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_get(TcsSocket socket_ctx,
                      int32_t level,
                      int32_t option_name,
                      void* option_value,
                      size_t* option_size);

/*
* @brief Enable the socket to be allowed to use broadcast.
*
* Only valid for protocols that support broadcast, for example UDP. Default is false.
*
* @param socket_ctx socket to enable/disable permission to send broadcast on.
* @param do_allow_broadcast set to true to allow, false to forbid.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_broadcast_set(TcsSocket socket_ctx, bool do_allow_broadcast);
TcsResult tcs_opt_broadcast_get(TcsSocket socket_ctx, bool* is_broadcast_allowed);

TcsResult tcs_opt_keep_alive_set(TcsSocket socket_ctx, bool do_keep_alive);
TcsResult tcs_opt_keep_alive_get(TcsSocket socket_ctx, bool* is_keep_alive_enabled);

TcsResult tcs_opt_reuse_address_set(TcsSocket socket_ctx, bool do_allow_reuse_address);
TcsResult tcs_opt_reuse_address_get(TcsSocket socket_ctx, bool* is_reuse_address_allowed);

TcsResult tcs_opt_send_buffer_size_set(TcsSocket socket_ctx, size_t send_buffer_size);
TcsResult tcs_opt_send_buffer_size_get(TcsSocket socket_ctx, size_t* send_buffer_size);

TcsResult tcs_opt_receive_buffer_size_set(TcsSocket socket_ctx, size_t receive_buffer_size);
TcsResult tcs_opt_receive_buffer_size_get(TcsSocket socket_ctx, size_t* receive_buffer_size);

TcsResult tcs_opt_receive_timeout_set(TcsSocket socket_ctx, int timeout_ms);
TcsResult tcs_opt_receive_timeout_get(TcsSocket socket_ctx, int* timeout_ms);

TcsResult tcs_opt_linger_set(TcsSocket socket_ctx, bool do_linger, int timeout_seconds);
TcsResult tcs_opt_linger_get(TcsSocket socket_ctx, bool* do_linger, int* timeout_seconds);

TcsResult tcs_opt_ip_no_delay_set(TcsSocket socket_ctx, bool use_no_delay);
TcsResult tcs_opt_ip_no_delay_get(TcsSocket socket_ctx, bool* is_no_delay_used);

TcsResult tcs_opt_out_of_band_inline_set(TcsSocket socket_ctx, bool enable_oob);
TcsResult tcs_opt_out_of_band_inline_get(TcsSocket socket_ctx, bool* is_oob_enabled);

TcsResult tcs_opt_priority_set(TcsSocket socket_ctx, int priority);
TcsResult tcs_opt_priority_get(TcsSocket socket_ctx, int* priority);

TcsResult tcs_opt_membership_add_to(TcsSocket socket_ctx,
                                    const struct TcsAddress* local_address,
                                    const struct TcsAddress* multicast_address);

TcsResult tcs_opt_membership_drop_from(TcsSocket socket_ctx,
                                       const struct TcsAddress* local_address,
                                       const struct TcsAddress* multicast_address);

TcsResult tcs_opt_membership_add(TcsSocket socket_ctx, const struct TcsAddress* multicast_address);
TcsResult tcs_opt_membership_drop(TcsSocket socket_ctx, const struct TcsAddress* multicast_address);

TcsResult tcs_opt_nonblocking_set(TcsSocket socket_ctx, bool do_nonblocking);
TcsResult tcs_opt_nonblocking_get(TcsSocket socket_ctx, bool* is_nonblocking);

TcsResult tcs_interface_list(struct TcsInterface interfaces[], size_t capacity, size_t* out_count);
TcsResult tcs_address_resolve(const char* hostname,
                              TcsAddressFamily address_family,
                              struct TcsAddress addresses[],
                              size_t capacity,
                              size_t* out_count);
TcsResult tcs_address_resolve_timeout(const char* hostname,
                                      TcsAddressFamily address_family,
                                      struct TcsAddress addresses[],
                                      size_t capacity,
                                      size_t* out_count,
                                      int timeout_ms);
TcsResult tcs_address_list(unsigned int interface_id_filter,
                           TcsAddressFamily address_family_filter,
                           struct TcsInterfaceAddress interface_addresses[],
                           size_t capacity,
                           size_t* out_count);
TcsResult tcs_address_socket_local(TcsSocket socket_ctx, struct TcsAddress* local_address);
TcsResult tcs_address_socket_remote(TcsSocket socket_ctx, struct TcsAddress* remote_address);
TcsResult tcs_address_socket_family(TcsSocket socket_ctx, TcsAddressFamily* out_family);

/**
 * @brief Get an address from a string.
 *
 * For example:
 * - "192.168.0.1:1212"
 * - "localhost:80"
 * - "[::1]:443".
 *
 * Note that this function will not perform DNS resolution. Use ::tcs_address_resolve() for that.
 *
 * @param address
 * @param out_str
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsResult tcs_address_parse(const char str[], struct TcsAddress* out_address);

/**
 * @brief Convert an address to a string.
 * 
 * This will make a verbose string representation of the address.
 * 
 * @param address
 * @param out_str
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsResult tcs_address_to_str(const struct TcsAddress* address, char out_str[70]);

bool tcs_address_is_equal(const struct TcsAddress* l, const struct TcsAddress* r);
bool tcs_address_is_any(const struct TcsAddress* addr);
bool tcs_address_is_local(const struct TcsAddress* addr);
bool tcs_address_is_loopback(const struct TcsAddress* addr);
bool tcs_address_is_multicast(const struct TcsAddress* addr);
bool tcs_address_is_broadcast(const struct TcsAddress* addr);

#ifdef __cplusplus
}
#endif

#endif
