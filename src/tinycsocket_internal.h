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

static const char* const TCS_VERSION_TXT = "v0.3.77";
extern const char* const TCS_LICENSE_TXT;

/*
* List of all functions in the library:
*
* The documentation for each function is located at the declaration in this document.
* Note: Most IDE:s and editors can jump to decalarations directly from here (ctrl+click on symbol or right click -> "Go to Declaration")
* 
* Library Management:
* - TcsResult tcs_lib_init(void);
* - TcsResult tcs_lib_cleanup(void);
*
* Socket Creation:
* - TcsResult tcs_socket(TcsSocket* out_socket, TcsFamily family, TcsSocketType type, TcsProtocol protocol);
* - TcsResult tcs_socket_tcp(TcsSocket* out_socket, const struct TcsAddress* local_address, const struct TcsAddress* remote_address, int timeout_ms);
* - TcsResult tcs_socket_tcp_str(TcsSocket* out_socket, const char* local_address, const char* remote_address, int timeout_ms);
* - TcsResult tcs_socket_udp(TcsSocket* out_socket, const struct TcsAddress* local_address, const struct TcsAddress* remote_address);
* - TcsResult tcs_socket_udp_str(TcsSocket* out_socket, const char* local_address, const char* remote_address);
* - TcsResult tcs_socket_packet(TcsSocket* out_socket, const struct TcsAddress* bind_address, TcsSocketType type);
* - TcsResult tcs_socket_packet_str(TcsSocket* out_socket, const char* interface_name, uint16_t protocol, TcsSocketType type);
* - TcsResult tcs_close(TcsSocket* socket);
*
* Socket Operations:
* - TcsResult tcs_bind(TcsSocket socket, const struct TcsAddress* local_address);
* - TcsResult tcs_connect(TcsSocket socket, const struct TcsAddress* address);
* - TcsResult tcs_connect_str(TcsSocket socket, const char* remote_address, uint16_t port);
* - TcsResult tcs_listen(TcsSocket socket, int backlog);
* - TcsResult tcs_accept(TcsSocket listener, TcsSocket* out_socket, struct TcsAddress* out_address);
* - TcsResult tcs_shutdown(TcsSocket socket, TcsShutdownDirection direction);
*
* Data Transfer:
* - TcsResult tcs_send(TcsSocket socket, const uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* out_sent_size);
* - TcsResult tcs_send_to(TcsSocket socket, const uint8_t* buffer, size_t buffer_size, uint32_t flags, const struct TcsAddress* destination_address, size_t* out_sent_size);
* - TcsResult tcs_sendv(TcsSocket socket, const struct TcsIoVec* iov, size_t iov_length, uint32_t flags, size_t* out_sent_size);
* - TcsResult tcs_send_netstring(TcsSocket socket, const uint8_t* buffer, size_t buffer_size);
* - TcsResult tcs_receive(TcsSocket socket, uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* out_received_size);
* - TcsResult tcs_receive_from(TcsSocket socket, uint8_t* buffer, size_t buffer_size, uint32_t flags, struct TcsAddress* out_source_address, size_t* out_received_size);
* - TcsResult tcs_receive_line(TcsSocket socket, uint8_t* buffer, size_t buffer_size, uint8_t delimiter, size_t* out_received_size);
* - TcsResult tcs_receive_netstring(TcsSocket socket, uint8_t* buffer, size_t buffer_size, size_t* out_received_size);
*
* Socket Polling:
* - TcsResult tcs_poll_create(struct TcsPoll** out_poll);
* - TcsResult tcs_poll_destroy(struct TcsPoll** poll);
* - TcsResult tcs_poll_add(struct TcsPoll* poll, TcsSocket socket, void* user_data, uint32_t flags);
* - TcsResult tcs_poll_modify(struct TcsPoll* poll, TcsSocket socket, uint32_t flags);
* - TcsResult tcs_poll_remove(struct TcsPoll* poll, TcsSocket socket);
* - TcsResult tcs_poll_wait(struct TcsPoll* poll, struct TcsPollEvent* out_events, size_t events_length, size_t* out_events_length, int timeout_ms);
*
* Socket Options:
* - TcsResult tcs_opt_set(TcsSocket socket, int32_t level, int32_t option_name, const void* option_value, size_t option_size);
* - TcsResult tcs_opt_get(TcsSocket socket, int32_t level, int32_t option_name, void* out_option_value, size_t* option_size);
* - TcsResult tcs_opt_type_get(TcsSocket socket, TcsSocketType* out_type);
* - TcsResult tcs_opt_broadcast_set(TcsSocket socket, bool do_allow_broadcast);
* - TcsResult tcs_opt_broadcast_get(TcsSocket socket, bool* out_is_broadcast_allowed);
* - TcsResult tcs_opt_keep_alive_set(TcsSocket socket, bool do_keep_alive);
* - TcsResult tcs_opt_keep_alive_get(TcsSocket socket, bool* out_is_keep_alive_enabled);
* - TcsResult tcs_opt_reuse_address_set(TcsSocket socket, bool do_allow_reuse_address);
* - TcsResult tcs_opt_reuse_address_get(TcsSocket socket, bool* out_is_reuse_address_allowed);
* - TcsResult tcs_opt_reuse_port_set(TcsSocket socket, bool do_allow_reuse_port);
* - TcsResult tcs_opt_reuse_port_get(TcsSocket socket, bool* out_is_reuse_port_allowed);
* - TcsResult tcs_opt_send_buffer_size_set(TcsSocket socket, size_t send_buffer_size);
* - TcsResult tcs_opt_send_buffer_size_get(TcsSocket socket, size_t* out_send_buffer_size);
* - TcsResult tcs_opt_receive_buffer_size_set(TcsSocket socket, size_t receive_buffer_size);
* - TcsResult tcs_opt_receive_buffer_size_get(TcsSocket socket, size_t* out_receive_buffer_size);
* - TcsResult tcs_opt_receive_timeout_set(TcsSocket socket, int timeout_ms);
* - TcsResult tcs_opt_receive_timeout_get(TcsSocket socket, int* out_timeout_ms);
* - TcsResult tcs_opt_linger_set(TcsSocket socket, bool do_linger, int timeout_seconds);
* - TcsResult tcs_opt_linger_get(TcsSocket socket, bool* out_do_linger, int* out_timeout_seconds);
* - TcsResult tcs_opt_ip_no_delay_set(TcsSocket socket, bool use_no_delay);
* - TcsResult tcs_opt_ip_no_delay_get(TcsSocket socket, bool* out_is_no_delay_used);
* - TcsResult tcs_opt_out_of_band_inline_set(TcsSocket socket, bool enable_oob);
* - TcsResult tcs_opt_out_of_band_inline_get(TcsSocket socket, bool* out_is_oob_enabled);
* - TcsResult tcs_opt_priority_set(TcsSocket socket, int priority);
* - TcsResult tcs_opt_priority_get(TcsSocket socket, int* out_priority);
* - TcsResult tcs_opt_nonblocking_set(TcsSocket socket, bool do_nonblocking);
* - TcsResult tcs_opt_nonblocking_get(TcsSocket socket, bool* out_is_nonblocking);
* - TcsResult tcs_opt_membership_add(TcsSocket socket, const struct TcsAddress* multicast_address);
* - TcsResult tcs_opt_membership_add_str(TcsSocket socket, const char* multicast_address);
* - TcsResult tcs_opt_membership_add_to(TcsSocket socket, const struct TcsAddress* local_address, const struct TcsAddress* multicast_address);
* - TcsResult tcs_opt_membership_drop(TcsSocket socket, const struct TcsAddress* multicast_address);
* - TcsResult tcs_opt_membership_drop_str(TcsSocket socket, const char* multicast_address);
* - TcsResult tcs_opt_membership_drop_from(TcsSocket socket, const struct TcsAddress* local_address, const struct TcsAddress* multicast_address);
* - TcsResult tcs_opt_multicast_interface_set(TcsSocket socket, const struct TcsAddress* local_address);
* - TcsResult tcs_opt_multicast_loop_set(TcsSocket socket, bool do_loopback);
* - TcsResult tcs_opt_multicast_loop_get(TcsSocket socket, bool* out_is_loopback);
*
* Address and Interface Utilities:
* - TcsResult tcs_interface_list(struct TcsInterface out_interfaces[], size_t interfaces_length, size_t* out_length);
* - TcsResult tcs_address_resolve(const char* hostname, TcsFamily address_family, struct TcsAddress out_addresses[], size_t addresses_length, size_t* out_length);
* - TcsResult tcs_address_list(TcsInterfaceId interface_id_filter, TcsFamily address_family_filter, struct TcsInterfaceAddress out_interface_addresses[], size_t interface_addresses_length, size_t* out_length);
* - TcsResult tcs_address_socket_local(TcsSocket socket, struct TcsAddress* out_local_address);
* - TcsResult tcs_address_socket_remote(TcsSocket socket, struct TcsAddress* out_remote_address);
* - TcsResult tcs_address_socket_family(TcsSocket socket, TcsFamily* out_family);
* - TcsResult tcs_address_parse(const char str[], struct TcsAddress* out_address);
* - TcsResult tcs_address_to_str(const struct TcsAddress* address, char out_str[70]);
* - bool tcs_address_is_equal(const struct TcsAddress* l, const struct TcsAddress* r);
* - bool tcs_address_is_any(const struct TcsAddress* addr);
* - bool tcs_address_is_link_local(const struct TcsAddress* addr);
* - bool tcs_address_is_loopback(const struct TcsAddress* addr);
* - bool tcs_address_is_multicast(const struct TcsAddress* addr);
* - bool tcs_address_is_broadcast(const struct TcsAddress* addr);
* - bool tcs_address_is_supported(const struct TcsAddress* addr);
*/

// Recognize which system we are compiling against

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

#if defined(TINYCSOCKET_IMPLEMENTATION)
#if (defined(__linux__) || defined(__CYGWIN__)) && defined(__STRICT_ANSI__)
#pragma message(                                                        \
    "tinycsocket: Strict ANSI C mode detected on glibc/Cygwin. "        \
    "POSIX symbols may be hidden. Use -std=gnu99 instead of -std=c99, " \
    "or define _POSIX_C_SOURCE=200112L and _DEFAULT_SOURCE before including this header.")
#endif
#if defined(__sun) && !defined(__EXTENSIONS__)
#pragma message(                                                     \
    "tinycsocket: illumos/Solaris detected without __EXTENSIONS__. " \
    "Define __EXTENSIONS__ and _XOPEN_SOURCE=500 before including this header.")
#endif
#endif

typedef int TcsSocket;
typedef unsigned int TcsInterfaceId;
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Configuration

#ifndef TCS_CFG_SENDV_STACK_MAX
#define TCS_CFG_SENDV_STACK_MAX 112
#endif

#ifndef TCS_CFG_INTERFACE_NAME_SIZE
#define TCS_CFG_INTERFACE_NAME_SIZE 64
#endif

// Declarations

/** @internal */
#define tcs_static_assert(name, expr) typedef char tcs_sa_##name[(expr) ? 1 : -1]

/**
 * @brief Address Family. Holds a native AF_* value in `native`.
 *
 * Use the `TCS_FAMILY_*` constants below as the only valid values.
 * Compare two values directly via the `.native` field.
 */
typedef struct TcsFamily
{
    int native;
} TcsFamily;

/**
 * @brief Socket type. Holds the native SOCK_* value in `native`.
 * Use the TCS_SOCK_* constants below as the only valid values.
 */
typedef struct TcsSocketType
{
    int native;
} TcsSocketType;

/**
 * @brief Protocol number for ::tcs_socket().
 *
 * Typedef'd as uint16_t since both IP protocol numbers (IANA, 8-bit) and EtherTypes (IEEE, 16-bit) fit.
 */
typedef uint16_t TcsProtocol;

/**
 * @brief Flags for poll events
 */
typedef enum
{
    TCS_POLL_READ = 1,
    TCS_POLL_WRITE = 2,
} TcsPollFlags;

// Socket Direction
typedef enum
{
    TCS_SHUTDOWN_RECEIVE, /**< To shutdown incoming packets for socket */
    TCS_SHUTDOWN_SEND,    /**< To shutdown outgoing packets for socket */
    TCS_SHUTDOWN_BOTH,    /**< To shutdown both incoming and outgoing packets for socket */
} TcsShutdownDirection;

// Return codes
typedef enum
{
    TCS_SUCCESS = 0,

    /* 1–15: Non-fatal return codes */
    TCS_AGAIN = 1,
    TCS_IN_PROGRESS = 2,
    TCS_SHUTDOWN = 3,

    /* -1...-31: General errors */
    TCS_ERROR_UNKNOWN = -1,
    TCS_ERROR_MEMORY = -2,
    TCS_ERROR_INVALID_ARGUMENT = -3,
    TCS_ERROR_SYSTEM = -4, /* OS error not mapped below */
    TCS_ERROR_PERMISSION_DENIED = -5,
    TCS_ERROR_NOT_IMPLEMENTED = -6,
    TCS_ERROR_NOT_SUPPORTED = -7, /* OS does not support this functionality */

    /* -32...-63: Network and socket errors */
    TCS_ERROR_ADDRESS_LOOKUP_FAILED = -32,
    TCS_ERROR_CONNECTION_REFUSED = -33,
    TCS_ERROR_NOT_CONNECTED = -34,
    TCS_ERROR_SOCKET_CLOSED = -35,
    TCS_ERROR_WOULD_BLOCK = -36,
    TCS_ERROR_TIMED_OUT = -37,
    TCS_ERROR_TEMPORARY_FAILURE = -38,
    TCS_ERROR_NETWORK_UNREACHABLE = -39,
    TCS_ERROR_CONNECTION_RESET = -40,
    TCS_ERROR_ADDRESS_IN_USE = -41,

    /* -64...-95: Configuration errors */
    TCS_ERROR_LIBRARY_NOT_INITIALIZED = -64,

    /* -96...-128: Protocol errors */
    TCS_ERROR_ILL_FORMED_MESSAGE = -96,
} TcsResult;

/**
 * @brief Convert a tinycsocket result code to a static human-readable string.
 *
 * The returned pointer is never NULL and must not be freed.
 */
const char* tcs_strerror(TcsResult result);

/**
 * @brief IPv6 address (16 bytes), analogous to POSIX struct in6_addr.
 */
struct TcsAddressIpv6
{
    uint8_t bytes[16];
};

typedef uint32_t TcsAddressIpv4;

/**
 * @brief Network Address
 *
 * Always host-byte-order. You will never need to use htons etc.

 */
struct TcsAddress
{
    TcsFamily family;
    union
    {
        unsigned char _storage[24]; /**< Ensures full zero-initialization when copied from TCS_ADDRESS_NONE */
        struct
        {
            TcsAddressIpv4 address; /**< Same byte order as the host */
            uint16_t port;          /**< Same byte order as the host */

        } ipv4;
        struct
        {
            struct TcsAddressIpv6 address;
            TcsInterfaceId
                scope_id;  /**< Native type. Only valid for local link addresses. See ::tcs_interface_list(). */
            uint16_t port; /**< Same byte order as the host */
        } ipv6;
        struct
        {
            TcsInterfaceId
                interface_id; /**< Local interface index, use tcs_interface_list() to find valid interfaces. Native type. */
            uint16_t
                protocol;   /**< Host byte order. E.g. TCS_PROTOCOL_ETH_ALL, 0 (block all until bind), ETH_P_TSN etc. */
            uint8_t mac[6]; /**< Typical destination mac address or local mac address when joining groups */
        } packet;
    } data;
};

/**
 * @brief Network Interface Information
 */
struct TcsInterface
{
    TcsInterfaceId id;
    char name[TCS_CFG_INTERFACE_NAME_SIZE];
};

struct TcsInterfaceAddress
{
    struct TcsInterface iface;
    struct TcsAddress address;
};

tcs_static_assert(address_storage_size, sizeof(((struct TcsAddress*)0)->data) <= 24);

/**
 * @brief Scatter/gather buffer descriptor (analogous to POSIX `struct iovec`).
 *
 * Useful if you want to send two or more data arrays at once, for example a header and a body.
 * Make an array of TcsIoVec and use tcs_sendv() to send them all at once.
*/
struct TcsIoVec
{
    const uint8_t* buffer;
    size_t buffer_size;
};

struct TcsPoll;
struct TcsPollEvent
{
    TcsSocket socket;
    void* user_data;
    bool can_read;
    bool can_write;
    TcsResult error;
};

extern const TcsFamily TCS_FAMILY_ANY;    /**< Layer 4 agnostic (AF_UNSPEC) */
extern const TcsFamily TCS_FAMILY_IPV4;   /**< INET IPv4 interface (AF_INET) */
extern const TcsFamily TCS_FAMILY_IPV6;   /**< INET IPv6 interface (AF_INET6) */
extern const TcsFamily TCS_FAMILY_PACKET; /**< Layer 2 interface (AF_PACKET on Linux; unsupported elsewhere) */

// gcc may trigger bug #53119
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif
static const struct TcsAddress TCS_ADDRESS_NONE = {{0}, {{0}}};
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

extern const TcsAddressIpv4 TCS_ADDRESS_IPV4_ANY;
extern const TcsAddressIpv4 TCS_ADDRESS_IPV4_LOOPBACK;
extern const TcsAddressIpv4 TCS_ADDRESS_IPV4_BROADCAST;
extern const TcsAddressIpv4 TCS_ADDRESS_IPV4_NONE;

extern const struct TcsAddressIpv6 TCS_ADDRESS_IPV6_ANY;
extern const struct TcsAddressIpv6 TCS_ADDRESS_IPV6_LOOPBACK;

extern const TcsSocket TCS_SOCKET_INVALID; /**< Define new sockets to this value, always. */
static const uint32_t TCS_FLAG_NONE = 0;

extern const TcsSocketType TCS_SOCKET_STREAM; /**< Use for streaming types like TCP */
extern const TcsSocketType TCS_SOCKET_DGRAM;  /**< Use for datagrams types like UDP */
extern const TcsSocketType TCS_SOCKET_RAW;    /**< Use for raw sockets, eg. layer 2 packet sockets */

static const TcsProtocol TCS_PROTOCOL_IP_TCP = 6;  /**< TCP, IANA-assigned (RFC 9293). Use with TCS_SOCKET_STREAM. */
static const TcsProtocol TCS_PROTOCOL_IP_UDP = 17; /**< UDP, IANA-assigned (RFC 768). Use with TCS_SOCKET_DGRAM. */
static const TcsProtocol TCS_PROTOCOL_ETH_ALL = 3; /**< Receive all protocols. Use with TCS_FAMILY_PACKET. */

// Recv flags
extern const uint32_t TCS_MSG_PEEK;
extern const uint32_t TCS_MSG_OOB;
extern const uint32_t TCS_MSG_WAITALL;

// Send flags
extern const uint32_t TCS_MSG_SENDALL;

// Backlog
extern const int TCS_BACKLOG_MAX; /**< Max number of queued sockets when listening */

// Option levels
extern const int32_t TCS_SOL_SOCKET; /**< Socket option level for socket options */
extern const int32_t TCS_SOL_IP;     /**< IP option level for socket options */
extern const int32_t TCS_SOL_TCP;    /**< TCP option level for socket options */
extern const int32_t TCS_SOL_PACKET; /**< Packet option level for socket options. Linux-only; -1 elsewhere. */

// Socket options
extern const int32_t TCS_SO_TYPE;
extern const int32_t TCS_SO_BROADCAST;
extern const int32_t TCS_SO_KEEPALIVE;
extern const int32_t TCS_SO_LINGER;
extern const int32_t TCS_SO_REUSEADDR;
extern const int32_t TCS_SO_REUSEPORT;
extern const int32_t TCS_SO_RCVBUF; /**< Byte size of receiving buffer */
extern const int32_t TCS_SO_RCVTIMEO;
extern const int32_t TCS_SO_SNDBUF; /**< Byte size of sending buffer */
extern const int32_t TCS_SO_OOBINLINE;
extern const int32_t TCS_SO_PRIORITY;

// IP options
extern const int32_t TCS_IP_MEMBERSHIP_ADD;
extern const int32_t TCS_IP_MEMBERSHIP_DROP;
extern const int32_t TCS_IP_MULTICAST_LOOP;

// TCP options
extern const int32_t TCS_TCP_NODELAY;

// Packet options
extern const int32_t TCS_PACKET_MEMBERSHIP_ADD;
extern const int32_t TCS_PACKET_MEMBERSHIP_DROP;

// Use for timeout to wait until infinity happens
extern const int32_t TCS_WAIT_INF;

static const struct TcsPollEvent TCS_POLL_EVENT_EMPTY = {0, 0, false, false, TCS_SUCCESS};

// ######## Library Management ########

/**
 * @brief Initialize tinycsocket library.
 *
 * This function needs to be called before using any other function in the library.
 *
 * On Windows, it will initialize Winsock, otherwise it does nothing and will always return #TCS_SUCCESS.
 *
 * You can call this multiple times, it will keep a counter of how many times you have called it (RAII friendly).
 * You should call tcs_lib_cleanup() after you are done with the library (before program exit), atleast the number of times you have called tcs_lib_init().
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
 *   tcs_lib_cleanup();
 * }
 * @endcode
 *
 * @see tcs_lib_cleanup()
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
TcsResult tcs_lib_cleanup(void);

// ######## Socket Creation ########

/**
 * @brief Create a new socket (BSD-style)
 *
 * This is a thin wrapper around the native socket function.
 * You may want to use one of the helper functions to create and setup a socket directly:
 *   - ::tcs_socket_tcp_str()
 *   - ::tcs_socket_udp_str()
 *   - ::tcs_socket_packet_str()
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
 *   TcsResult tcs_socket_res = tcs_socket(&my_socket, TCS_FAMILY_IPV4, TCS_SOCKET_STREAM, TCS_PROTOCOL_IP_TCP);
 *   if (tcs_socket_res != TCS_SUCCESS)
 *   {
 *     tcs_lib_cleanup();
 *     return -2; // Failed to create socket
 *   }
 *
 *   // Do stuff with my_socket here. See examples in the documentation.
 *
 *   tcs_close(&my_socket); // Safe to call even if my_socket is TCS_SOCKET_INVALID
 *   tcs_lib_cleanup();
 * }
 * @endcode
 *
 * @param[out] out_socket pointer to socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
 * @param[in] family See ::TcsFamily for supported values.
 * @param[in] type specifies the type of the socket, supported values are: ::TCS_SOCKET_STREAM, ::TCS_SOCKET_DGRAM and ::TCS_SOCKET_RAW.
 * @param[in] protocol specifies the protocol, for example #TCS_PROTOCOL_IP_TCP or #TCS_PROTOCOL_IP_UDP.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 *
 * @retval #TCS_SUCCESS if successful.
 * @retval #TCS_ERROR_INVALID_ARGUMENT if you have provided an invalid argument. Such as a socket that is not #TCS_SOCKET_INVALID.
 * @retval #TCS_ERROR_NOT_SUPPORTED if you have provided an address family, type, or protocol that is not supported on this platform.
 * @retval #TCS_ERROR_PERMISSION_DENIED if you do not have permission to create the socket. E.g. raw sockets often require elevated permissions.
 *
 * @see tcs_socket_tcp_str()
 * @see tcs_socket_udp_str()
 * @see tcs_socket_packet_str()
 * @see tcs_close()
 * @see tcs_lib_init()
 * @see tcs_lib_cleanup()
 */
TcsResult tcs_socket(TcsSocket* out_socket, TcsFamily family, TcsSocketType type, TcsProtocol protocol);

/**
* @brief Create a TCP socket, optionally bind to a local address and/or connect to a remote address.
*
* Creates an IPv4 or IPv6 TCP socket based on the address family of the provided address(es).
* If @p local_address is not NULL, SO_REUSEADDR is set and the socket is bound to it.
* If @p remote_address is not NULL, the socket connects to it.
* At least one of @p local_address or @p remote_address must be non-NULL.
* If both are provided, they must have the same address family.
* On failure, *out_socket is always set back to #TCS_SOCKET_INVALID.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   tcs_lib_init();
*
*   struct TcsAddress local = TCS_ADDRESS_NONE;
*   local.family = TCS_FAMILY_IPV4;
*   local.data.ipv4.address = TCS_ADDRESS_IPV4_ANY;
*   local.data.ipv4.port = 8080;
*
*   TcsSocket server = TCS_SOCKET_INVALID;
*   TcsResult res = tcs_socket_tcp(&server, &local, NULL, 0);
*   if (res != TCS_SUCCESS)
*   {
*     tcs_lib_cleanup();
*     return -1;
*   }
*
*   // Socket is now bound, call tcs_listen() and tcs_accept() to accept connections
*
*   tcs_close(&server);
*   tcs_lib_cleanup();
* }
* @endcode
*
* @param[out] out_socket pointer to socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] local_address address to bind to, or NULL to skip binding.
* @param[in] remote_address address to connect to, or NULL to skip connecting.
* @param[in] timeout_ms maximum time in milliseconds to wait for connection, or #TCS_WAIT_INF for OS default timeout. Ignored if @p remote_address is NULL.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if @p out_socket is NULL, if *out_socket is not #TCS_SOCKET_INVALID, if both addresses are NULL, or if both are provided with different address families.
* @retval #TCS_ERROR_CONNECTION_REFUSED if the remote server refused the connection.
* @retval #TCS_ERROR_TIMED_OUT if the connection attempt timed out.
*
* @see tcs_connect()
* @see tcs_listen()
* @see tcs_close()
*/
TcsResult tcs_socket_tcp(TcsSocket* out_socket,
                         const struct TcsAddress* local_address,
                         const struct TcsAddress* remote_address,
                         int timeout_ms);

/**
* @brief Create a TCP socket from string addresses, optionally bind and/or connect.
*
* Resolves the address strings with ::tcs_address_resolve() and delegates to ::tcs_socket_tcp().
* Addresses must include a port, e.g. "127.0.0.1:8080" or "[::1]:8080".
* At least one of @p local_address or @p remote_address must be non-NULL.
* On failure, *out_socket is always set back to #TCS_SOCKET_INVALID.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   tcs_lib_init();
*
*   TcsSocket socket = TCS_SOCKET_INVALID;
*   TcsResult res = tcs_socket_tcp_str(&socket, NULL, "127.0.0.1:8080", 5000);
*   if (res != TCS_SUCCESS)
*   {
*     tcs_lib_cleanup();
*     return -1;
*   }
*
*   // Socket is now connected and ready for communication
*
*   tcs_close(&socket);
*   tcs_lib_cleanup();
* }
* @endcode
*
* @param[out] out_socket pointer to socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] local_address address string to bind to, or NULL to skip binding.
* @param[in] remote_address address string to connect to, or NULL to skip connecting.
* @param[in] timeout_ms maximum time in milliseconds to wait for connection, or #TCS_WAIT_INF for OS default timeout. Ignored if @p remote_address is NULL.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if @p out_socket is NULL, if *out_socket is not #TCS_SOCKET_INVALID, or if both addresses are NULL.
* @retval #TCS_ERROR_CONNECTION_REFUSED if the remote server refused the connection.
* @retval #TCS_ERROR_TIMED_OUT if the connection attempt timed out.
*
* @see tcs_socket_tcp()
* @see tcs_close()
*/
TcsResult tcs_socket_tcp_str(TcsSocket* out_socket,
                             const char* local_address,
                             const char* remote_address,
                             int timeout_ms);

/**
* @brief Create a UDP socket, optionally bind to a local address and/or connect to a remote address.
*
* Creates an IPv4 or IPv6 UDP socket based on the address family of the provided address(es).
* If @p local_address is not NULL, SO_REUSEADDR is set and the socket is bound to it.
* If @p remote_address is not NULL and is a unicast address, the socket is connected to it,
* allowing use of ::tcs_send() instead of ::tcs_send_to().
* If @p remote_address is a multicast address, the socket joins the multicast group.
* When only @p remote_address is given (no @p local_address), the socket is also connected,
* allowing use of ::tcs_send(). When both are given, the socket is not connected to avoid
* filtering out multicast traffic — use ::tcs_send_to() instead.
* At least one of @p local_address or @p remote_address must be non-NULL.
* If both are provided, they must have the same address family.
* On failure, *out_socket is always set back to #TCS_SOCKET_INVALID.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   tcs_lib_init();
*
*   struct TcsAddress local = TCS_ADDRESS_NONE;
*   local.family = TCS_FAMILY_IPV4;
*   local.data.ipv4.address = TCS_ADDRESS_IPV4_ANY;
*   local.data.ipv4.port = 8080;
*
*   TcsSocket socket = TCS_SOCKET_INVALID;
*   TcsResult res = tcs_socket_udp(&socket, &local, NULL);
*   if (res != TCS_SUCCESS)
*   {
*     tcs_lib_cleanup();
*     return -1;
*   }
*
*   // Socket is now bound and ready to receive with tcs_receive_from()
*
*   tcs_close(&socket);
*   tcs_lib_cleanup();
* }
* @endcode
*
* @param[out] out_socket pointer to socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] local_address address to bind to, or NULL to skip binding.
* @param[in] remote_address address to connect to, or NULL to skip connecting.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if @p out_socket is NULL, if *out_socket is not #TCS_SOCKET_INVALID, if both addresses are NULL, or if both are provided with different address families.
*
* @see tcs_socket_udp_str()
* @see tcs_close()
*/
TcsResult tcs_socket_udp(TcsSocket* out_socket,
                         const struct TcsAddress* local_address,
                         const struct TcsAddress* remote_address);

/**
* @brief Create a UDP socket from string addresses, optionally bind and/or connect.
*
* Parses the address strings with ::tcs_address_resolve() and delegates to ::tcs_socket_udp().
* Addresses must include a port, e.g. "127.0.0.1:8080" or "[::1]:8080".
* At least one of @p local_address or @p remote_address must be non-NULL.
* On failure, *out_socket is always set back to #TCS_SOCKET_INVALID.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   tcs_lib_init();
*
*   TcsSocket socket = TCS_SOCKET_INVALID;
*   TcsResult res = tcs_socket_udp_str(&socket, "0.0.0.0:8080", NULL);
*   if (res != TCS_SUCCESS)
*   {
*     tcs_lib_cleanup();
*     return -1;
*   }
*
*   // Socket is now bound and ready to receive with tcs_receive_from()
*
*   tcs_close(&socket);
*   tcs_lib_cleanup();
* }
* @endcode
*
* @param[out] out_socket pointer to socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] local_address address string to bind to, or NULL to skip binding.
* @param[in] remote_address address string to connect to, or NULL to skip connecting.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if @p out_socket is NULL, if *out_socket is not #TCS_SOCKET_INVALID, or if both addresses are NULL.
*
* @see tcs_socket_udp()
* @see tcs_close()
*/
TcsResult tcs_socket_udp_str(TcsSocket* out_socket, const char* local_address, const char* remote_address);

/**
* @brief Create a packet socket bound to a network interface.
*
* Creates an AF_PACKET socket for sending and receiving raw L2 frames.
* The socket is bound to the interface and protocol specified in @p bind_address.
* On failure, *out_socket is always set back to #TCS_SOCKET_INVALID.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   tcs_lib_init();
*
*   struct TcsAddress bind = TCS_ADDRESS_NONE;
*   bind.family = TCS_FAMILY_PACKET;
*   bind.data.packet.interface_id = 1; // e.g. lo
*   bind.data.packet.protocol = 0x22F0; // e.g. AVTP
*
*   TcsSocket socket = TCS_SOCKET_INVALID;
*   TcsResult res = tcs_socket_packet(&socket, &bind, TCS_SOCKET_DGRAM);
*   if (res != TCS_SUCCESS)
*   {
*     tcs_lib_cleanup();
*     return -1;
*   }
*
*   // Socket is now bound and ready for tcs_send_to() / tcs_receive_from()
*
*   tcs_close(&socket);
*   tcs_lib_cleanup();
* }
* @endcode
*
* @param[out] out_socket pointer to socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] bind_address address with family TCS_FAMILY_PACKET specifying interface_id and protocol.
* @param[in] type socket type, either #TCS_SOCKET_RAW for full L2 frames or #TCS_SOCKET_DGRAM for frames without the L2 header.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if @p out_socket is NULL, if *out_socket is not #TCS_SOCKET_INVALID, or if @p bind_address is NULL or not TCS_FAMILY_PACKET.
*
* @see tcs_socket_packet_str()
* @see tcs_close()
*/
TcsResult tcs_socket_packet(TcsSocket* out_socket, const struct TcsAddress* bind_address, TcsSocketType type);

/**
* @brief Create a packet socket bound to a named network interface.
*
* Looks up the interface by name using ::tcs_interface_list() and delegates to ::tcs_socket_packet().
* On failure, *out_socket is always set back to #TCS_SOCKET_INVALID.
*
* @code
* #include "tinycsocket.h"
* int main()
* {
*   tcs_lib_init();
*
*   TcsSocket socket = TCS_SOCKET_INVALID;
*   TcsResult res = tcs_socket_packet_str(&socket, "eth0", 0x22F0, TCS_SOCKET_DGRAM);
*   if (res != TCS_SUCCESS)
*   {
*     tcs_lib_cleanup();
*     return -1;
*   }
*
*   // Socket is now bound and ready for tcs_send_to() / tcs_receive_from()
*
*   tcs_close(&socket);
*   tcs_lib_cleanup();
* }
* @endcode
*
* @param[out] out_socket pointer to socket context to be created, which must have been initialized to #TCS_SOCKET_INVALID before use.
* @param[in] interface_name name of the network interface to bind to.
* @param[in] protocol EtherType protocol in host byte order, e.g. 0x22F0 for AVTP.
* @param[in] type socket type, either #TCS_SOCKET_RAW for full L2 frames or #TCS_SOCKET_DGRAM for frames without the L2 header.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if @p out_socket is NULL, if *out_socket is not #TCS_SOCKET_INVALID, or if @p interface_name is NULL or not found.
*
* @see tcs_socket_packet()
* @see tcs_close()
*/
TcsResult tcs_socket_packet_str(TcsSocket* out_socket,
                                const char* interface_name,
                                uint16_t protocol,
                                TcsSocketType type);

/**
* @brief Closes the socket, stop communication and free all resources for the socket.
*
* This will free all resources associated with the socket and set the socket value to #TCS_SOCKET_INVALID.
*
* @param[in,out] socket is a pointer to your socket context you have previously created with ::tcs_socket() or one of the helper functions. Will be set to #TCS_SOCKET_INVALID on success.
*
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if @p socket is NULL or already #TCS_SOCKET_INVALID.
*
* @see tcs_socket()
* @see tcs_socket_tcp()
* @see tcs_socket_udp()
* @see tcs_socket_packet()
* @see tcs_lib_init()
* @see tcs_lib_cleanup()
*/
TcsResult tcs_close(TcsSocket* socket);

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
 *   TcsResult socket_res = tcs_socket(&server_socket, TCS_FAMILY_IPV4, TCS_SOCKET_STREAM, TCS_PROTOCOL_IP_TCP);
 *   if (socket_res != TCS_SUCCESS)
 *   {
 *     tcs_lib_cleanup();
 *     return -2;
 *   }
 *
 *   struct TcsAddress local_address = TCS_ADDRESS_NONE;
 *   local_address.family = TCS_FAMILY_IPV4;
 *   local_address.data.ipv4.address = TCS_ADDRESS_IPV4_ANY; // Bind to all interfaces
 *   local_address.data.ipv4.port = 8080;
 *
 *   TcsResult bind_res = tcs_bind(server_socket, &local_address);
 *   if (bind_res != TCS_SUCCESS)
 *   {
 *     tcs_close(&server_socket);
 *     tcs_lib_cleanup();
 *     return -3; // Failed to bind to address
 *   }
 *
 *   // For TCP: now call tcs_listen()
 *   // For UDP: socket is ready to receive datagrams
 *
 *   tcs_close(&server_socket);
 *   tcs_lib_cleanup();
 *   return 0;
 * }
 * @endcode
 *
 * @param[in] socket The socket to bind. Must be a valid socket created with tcs_socket().
 * @param[in] local_address The local address structure to bind to. Use TCS_ADDRESS_IPV4_ANY for the address field to bind to all interfaces.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @retval #TCS_ERROR_INVALID_ARGUMENT if socket is invalid or local_address is NULL.
 * @retval #TCS_ERROR_NOT_SUPPORTED if local_address has an address family not supported on this platform.
 * @retval #TCS_ERROR_PERMISSION_DENIED if binding to the specified address/port requires elevated privileges.
 * @retval #TCS_ERROR_SYSTEM if the address is already in use or another system error occurred.
 *
 * @see tcs_listen()
 * @see tcs_socket_tcp()
 * @see tcs_socket_udp()
 * @see tcs_address_socket_local()
 */
TcsResult tcs_bind(TcsSocket socket, const struct TcsAddress* local_address);

/**
 * @brief Connect a socket to a remote address structure.
 *
 * This function establishes a connection to the specified remote address structure.
 * For TCP sockets, this initiates a three-way handshake. For UDP sockets, this
 * associates the socket with the remote address for subsequent send operations.
 * The function blocks until the connection is established, fails, or the OS-default
 * connect timeout expires (typically tens of seconds to a few minutes, platform-dependent).
 * Use TcsPoll or ::tcs_opt_nonblocking_set() for non-blocking behavior.
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
 *   TcsResult socket_res = tcs_socket(&client_socket, TCS_FAMILY_IPV4, TCS_SOCKET_STREAM, TCS_PROTOCOL_IP_TCP);
 *   if (socket_res != TCS_SUCCESS)
 *   {
 *     tcs_lib_cleanup();
 *     return -2;
 *   }
 *
 *   struct TcsAddress remote_address = TCS_ADDRESS_NONE;
 *   remote_address.family = TCS_FAMILY_IPV4;
 *   remote_address.data.ipv4.address = 0x7F000001; // 127.0.0.1 loopback
 *   remote_address.data.ipv4.port = 8080;
 *
 *   TcsResult connect_res = tcs_connect(client_socket, &remote_address);
 *   if (connect_res != TCS_SUCCESS)
 *   {
 *     tcs_close(&client_socket);
 *     tcs_lib_cleanup();
 *     return -3; // Failed to connect
 *   }
 *
 *   // Socket is now connected and ready for communication
 *   uint8_t buffer[] = "Hello, server!";
 *   size_t sent_size = 0;
 *   tcs_send(client_socket, buffer, sizeof(buffer)-1, TCS_MSG_SENDALL, &sent_size);
 *
 *   tcs_close(&client_socket);
 *   tcs_lib_cleanup();
 *   return 0;
 * }
 * @endcode
 *
 * @param[in] socket The socket to connect. Must be a valid socket created with tcs_socket().
 * @param[in] address The remote address structure to connect to.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @retval #TCS_ERROR_INVALID_ARGUMENT if socket is invalid or address is NULL.
 * @retval #TCS_ERROR_NOT_SUPPORTED if address has an address family not supported on this platform.
 * @retval #TCS_ERROR_CONNECTION_REFUSED if the remote server refused the connection.
 * @retval #TCS_ERROR_TIMED_OUT if the connection attempt timed out (can take 3+ minutes for unreachable hosts).
 * @retval #TCS_ERROR_SYSTEM if another system error occurred.
 *
 * @see tcs_connect_str()
 * @see tcs_socket_tcp()
 * @see tcs_bind()
 * @see tcs_listen()
 */
TcsResult tcs_connect(TcsSocket socket, const struct TcsAddress* address);

/**
 * @brief Connect a socket to a remote hostname and port.
 *
 * This function establishes a connection to the specified remote address and port.
 * For TCP sockets, this initiates a three-way handshake. For UDP sockets, this
 * associates the socket with the remote address for subsequent send operations.
 * Timeout for this function is set by OS defaults. Use TcsPoll or
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
 *   TcsResult socket_res = tcs_socket(&client_socket, TCS_FAMILY_IPV4, TCS_SOCKET_STREAM, TCS_PROTOCOL_IP_TCP);
 *   if (socket_res != TCS_SUCCESS)
 *   {
 *     tcs_lib_cleanup();
 *     return -2;
 *   }
 *
 *   TcsResult connect_res = tcs_connect_str(client_socket, "192.168.1.100", 8080);
 *   if (connect_res != TCS_SUCCESS)
 *   {
 *     tcs_close(&client_socket);
 *     tcs_lib_cleanup();
 *     return -3; // Failed to connect
 *   }
 *
 *   // Socket is now connected and ready for communication
 *   uint8_t buffer[] = "Hello, server!";
 *   size_t sent_size = 0;
 *   tcs_send(client_socket, buffer, sizeof(buffer)-1, TCS_MSG_SENDALL, &sent_size);
 *
 *   tcs_close(&client_socket);
 *   tcs_lib_cleanup();
 *   return 0;
 * }
 * @endcode
 *
 * @param[in] socket The socket to connect. Must be a valid socket created with tcs_socket().
 * @param[in] remote_address The remote hostname or IP address to connect to.
 * @param[in] port The remote port number to connect to.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @retval #TCS_ERROR_INVALID_ARGUMENT if socket is invalid or remote_address is NULL.
 * @retval #TCS_ERROR_CONNECTION_REFUSED if the remote server refused the connection.
 * @retval #TCS_ERROR_ADDRESS_LOOKUP_FAILED if the hostname could not be resolved.
 * @retval #TCS_ERROR_TIMED_OUT if the connection attempt timed out.
 * @retval #TCS_ERROR_SYSTEM if another system error occurred.
 *
 * @see tcs_connect()
 * @see tcs_socket_tcp_str()
 * @see tcs_bind()
 * @see tcs_listen()
 */
TcsResult tcs_connect_str(TcsSocket socket, const char* remote_address, uint16_t port);

/**
 * @brief Let a socket start listening for incoming connections.
 *
 * Call #tcs_bind() first to bind to a local address to listening at.
 *
 * @param[in] socket is your in-out socket context.
 * @param[in] backlog is the maximum number of queued incoming sockets. Use #TCS_BACKLOG_MAX to set it to max.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_accept()
 */
TcsResult tcs_listen(TcsSocket socket, int backlog);

/**
 * @brief Accept a socket from a listening socket.
 *
 * The accepted socket inherits the listening socket's local address and port; only the remote
 * (peer) endpoint differs. The listening socket itself is not affected by this call.
 *
 * Example usage:
 * @code
 * TcsSocket listen_socket = TCS_SOCKET_INVALID;
 * tcs_socket(&listen_socket, TCS_FAMILY_IPV4, TCS_SOCKET_STREAM, TCS_PROTOCOL_IP_TCP);
 * struct TcsAddress local_address = TCS_ADDRESS_NONE;
 * local_address.family = TCS_FAMILY_IPV4;
 * local_address.data.ipv4.port = 1212;
 * tcs_bind(listen_socket, &local_address);
 * tcs_listen(listen_socket, TCS_BACKLOG_MAX);
 * while (true)
 * {
 *   TcsSocket client_socket = TCS_SOCKET_INVALID;
 *   tcs_accept(listen_socket, &client_socket, NULL);
 *   // Do stuff with client_socket here
 *   tcs_close(&client_socket);
 * }
 * @endcode
 * 
 * @param[in] listener is your listening socket you used when you called ::tcs_listen().
 * @param[out] out_socket is your accepted socket. Must have the in value of #TCS_SOCKET_INVALID.
 * @param[out] out_address is an optional pointer to a buffer where the remote address of the accepted socket can be stored.
 *
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsResult tcs_accept(TcsSocket listener, TcsSocket* out_socket, struct TcsAddress* out_address);

/**
* @brief Turn off communication with a 3-way handshaking for the socket.
* 
* Use this function to cancel blocking calls (recv, accept etc) from another thread, or use sigaction.
* The socket will finish all queued sends first.
*
* @param[in] socket is your in-out socket context.
* @param[in] direction defines in which direction you want to turn off the communication.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_shutdown(TcsSocket socket, TcsShutdownDirection direction);

/**
 * @brief Sends data on a socket, blocking
 *
 * @param[in] socket is your in-out socket context.
 * @param[in] buffer is a pointer to your data you want to send.
 * @param[in] buffer_size is number of bytes of the data you want to send.
 * @param[in] flags is a bitmask of send flags. Use #TCS_FLAG_NONE for no flags, or #TCS_MSG_SENDALL to keep sending until all bytes are transmitted (or the call fails).
 * @param[out] out_sent_size is how many bytes that was successfully sent.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @see tcs_receive()
 */
TcsResult tcs_send(TcsSocket socket, const uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* out_sent_size);

/**
 * @brief Sends data to an address, useful with UDP sockets.
 *
 * @param[in] socket is your in-out socket context.
 * @param[in] buffer is a pointer to your data you want to send.
 * @param[in] buffer_size is number of bytes of the data you want to send.
 * @param[in] flags is a bitmask of send flags. Use #TCS_FLAG_NONE for no flags, or #TCS_MSG_SENDALL to keep sending until all bytes are transmitted (or the call fails).
 * @param[in] destination_address is the address to send to.
 * @param[out] out_sent_size is how many bytes that was successfully sent.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 * @retval #TCS_ERROR_NOT_SUPPORTED if destination_address has an address family not supported on this platform.
 * @see tcs_receive_from()
 */
TcsResult tcs_send_to(TcsSocket socket,
                      const uint8_t* buffer,
                      size_t buffer_size,
                      uint32_t flags,
                      const struct TcsAddress* destination_address,
                      size_t* out_sent_size);

/**
* @brief Sends several data buffers on a socket as one message.
*
* @param[in] socket is your in-out socket context.
* @param[in] iov is a pointer to your array of scatter/gather buffers you want to send.
* @param[in] iov_length is the number of buffers in your array.
* @param[in] flags is a bitmask of send flags. Use #TCS_FLAG_NONE for no flags, or #TCS_MSG_SENDALL to keep sending until all bytes are transmitted (or the call fails).
* @param[out] out_sent_size is how many bytes in total that was successfully sent.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_sendv(TcsSocket socket,
                    const struct TcsIoVec* iov,
                    size_t iov_length,
                    uint32_t flags,
                    size_t* out_sent_size);

/**
* @brief Send data encoded as a netstring.
*
* Netstrings provide a simple framing format for sending discrete messages over a
* stream-oriented transport such as TCP (::TCS_SOCKET_STREAM). The format is:
* @code
* <length>:<data>,
* @endcode
* For example, the string "hello" is encoded as @c 5:hello,
*
* This is useful when you need packet-like semantics over TCP, where message
* boundaries are otherwise not preserved.
*
* @param[in] socket socket to send on.
* @param[in] buffer data to send.
* @param[in] buffer_size number of bytes to send.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_receive_netstring()
*/
TcsResult tcs_send_netstring(TcsSocket socket, const uint8_t* buffer, size_t buffer_size);

/**
* @brief Receive data from a socket to your buffer
*
* @param[in] socket is your in-out socket context.
* @param[out] buffer is a pointer to your buffer where you want to store the incoming data to.
* @param[in] buffer_size is the byte size of your buffer, for preventing overflows.
* @param[in] flags is a bitmask of receive flags. Use #TCS_FLAG_NONE for no flags, or any combination of #TCS_MSG_PEEK, #TCS_MSG_OOB, and #TCS_MSG_WAITALL.
* @param[out] out_received_size is how many bytes that was successfully written to your buffer.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_send()
*/
TcsResult tcs_receive(TcsSocket socket, uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* out_received_size);

/**
* @brief Receive data from an address, useful with UDP sockets.
*
* @param[in] socket is your in-out socket context.
* @param[out] buffer is a pointer to your buffer where you want to store the incoming data to.
* @param[in] buffer_size is the byte size of your buffer, for preventing overflows.
* @param[in] flags is a bitmask of receive flags. Use #TCS_FLAG_NONE for no flags, or any combination of #TCS_MSG_PEEK, #TCS_MSG_OOB, and #TCS_MSG_WAITALL.
* @param[out] out_source_address is the address the data was received from.
* @param[out] out_received_size is how many bytes that was successfully written to your buffer.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_send_to()
*/
TcsResult tcs_receive_from(TcsSocket socket,
                           uint8_t* buffer,
                           size_t buffer_size,
                           uint32_t flags,
                           struct TcsAddress* out_source_address,
                           size_t* out_received_size);

/**
* @brief Read up to and including a delimiter.
*
* This function ensures that the socket buffer will keep its data after the delimiter.
* For performance it is recommended to read everything and split it yourself.
* The call will block until the delimiter is received or the supplied buffer is filled.
* The timeout time will not be per call but between each packet received. Longer call time than timeout is possible.
*
* @param[in] socket is your in-out socket context.
* @param[out] buffer is a pointer to your buffer where you want to store the incoming data to.
* @param[in] buffer_size is the byte size of your buffer, for preventing overflows.
* @param[in] delimiter is your byte value where you want to stop reading. (including delimiter)
* @param[out] out_received_size is how many bytes that was successfully written to your buffer.
* @return #TCS_AGAIN if no delimiter was found and the supplied buffer was filled.
* @return #TCS_SUCCESS if the delimiter was found. Otherwise the error code.
* @see tcs_receive_netstring()
*/
TcsResult tcs_receive_line(TcsSocket socket,
                           uint8_t* buffer,
                           size_t buffer_size,
                           uint8_t delimiter,
                           size_t* out_received_size);

/**
* @brief Receive a netstring-encoded message.
*
* Reads and decodes a netstring (see tcs_send_netstring() for format details) from
* a stream socket. This allows receiving discrete messages over TCP where message
* boundaries are otherwise not preserved.
*
* @param[in] socket socket to receive from.
* @param[out] buffer buffer to store the decoded data (without the netstring framing).
* @param[in] buffer_size size of the buffer in bytes.
* @param[out] out_received_size optional pointer to receive the number of payload bytes received.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_ILL_FORMED_MESSAGE if the netstring is malformed or the length overflows.
* @retval #TCS_ERROR_MEMORY if the buffer is too small for the payload.
* @see tcs_send_netstring()
*/
TcsResult tcs_receive_netstring(TcsSocket socket, uint8_t* buffer, size_t buffer_size, size_t* out_received_size);

/**
* @brief Create a context used for waiting on several sockets.
*
* TcsPoll can be used to monitor several sockets for events (reading, writing or error).
* Use tcs_poll_wait() to get a list of sockets ready to interact with.
* Errors are always reported regardless of flags.
*
* @code
* tcs_lib_init();
* TcsSocket socket1 = TCS_SOCKET_INVALID;
* TcsSocket socket2 = TCS_SOCKET_INVALID;
* tcs_socket(&socket1, TCS_FAMILY_IPV4, TCS_SOCKET_DGRAM, TCS_PROTOCOL_IP_UDP);
* tcs_socket(&socket2, TCS_FAMILY_IPV4, TCS_SOCKET_DGRAM, TCS_PROTOCOL_IP_UDP);
*
* struct TcsAddress addr1 = TCS_ADDRESS_NONE;
* addr1.family = TCS_FAMILY_IPV4;
* addr1.data.ipv4.port = 1000;
* tcs_bind(socket1, &addr1);
*
* struct TcsAddress addr2 = TCS_ADDRESS_NONE;
* addr2.family = TCS_FAMILY_IPV4;
* addr2.data.ipv4.port = 1001;
* tcs_bind(socket2, &addr2);
*
* struct TcsPoll* poll = NULL;
* tcs_poll_create(&poll);
* tcs_poll_add(poll, socket1, NULL, TCS_POLL_READ); // Only wait for incoming data
* tcs_poll_add(poll, socket2, NULL, TCS_POLL_READ);
*
* size_t populated = 0;
* struct TcsPollEvent ev[2] = {TCS_POLL_EVENT_EMPTY, TCS_POLL_EVENT_EMPTY};
* tcs_poll_wait(poll, ev, 2, &populated, 1000); // Will wait 1000 ms for data on port 1000 or 1001
* for (size_t i = 0; i < populated; ++i)
* {
*     if (ev[i].can_read)
*     {
*         uint8_t recv_buffer[8192] = {0};
*         size_t received_size = 0;
*         tcs_receive(ev[i].socket, recv_buffer, 8191, TCS_FLAG_NONE, &received_size);
*     }
* }
* tcs_poll_destroy(&poll);
* tcs_close(&socket1);
* tcs_close(&socket2);
* tcs_lib_cleanup();
* @endcode
*
* @param[out] out_poll is your out poll context pointer. Initiate a TcsPoll pointer to NULL and use the address of this pointer.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_poll_destroy()
*/
TcsResult tcs_poll_create(struct TcsPoll** out_poll);

/**
* @brief Frees all resources bound to the poll context.
*
* Will set @p poll to NULL when successful.
*
* @param[in,out] poll is your poll context pointer created with tcs_poll_create(). Will be set to NULL.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_poll_create()
*/
TcsResult tcs_poll_destroy(struct TcsPoll** poll);

/**
* @brief Add a socket to the poll context.
*
* @param[in] poll is your poll context pointer created with tcs_poll_create().
* @param[in] socket will be added to the poll context. Note that you can still use it outside of the poll context.
* @param[in] user_data is a pointer of your choice that is associated with the socket. Use NULL if not used.
* @param[in] flags is a bitmask of ::TcsPollFlags (e.g. TCS_POLL_READ | TCS_POLL_WRITE). Errors are always reported.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_poll_remove()
*/
TcsResult tcs_poll_add(struct TcsPoll* poll, TcsSocket socket, void* user_data, uint32_t flags);

/**
* @brief Modify the poll flags for a socket already in the poll context.
*
* @param[in] poll is your poll context pointer created with tcs_poll_create().
* @param[in] socket is the socket to modify.
* @param[in] flags is the new bitmask of ::TcsPollFlags. Errors are always reported.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if the socket is not in the poll context.
* @see tcs_poll_add()
*/
TcsResult tcs_poll_modify(struct TcsPoll* poll, TcsSocket socket, uint32_t flags);

/**
* @brief Remove a socket from the poll context.
*
* @param[in] poll is a context pointer created with tcs_poll_create()
* @param[in] socket will be removed from the poll context.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_poll_add()
*/
TcsResult tcs_poll_remove(struct TcsPoll* poll, TcsSocket socket);

/**
* @brief Wait for events on sockets in the poll context.
*
* @param[in] poll is your poll context pointer created with @p tcs_poll_create().
* @param[in,out] out_events is an array to fill with events. Assign each element to #TCS_POLL_EVENT_EMPTY before the call.
* @param[in] events_length number of in elements in your @p out_events array. Does not make sense to have more events than number of sockets in the poll context. If too short, all events may not be returned.
* @param[out] out_events_length will contain the number of events the @p out_events array has been populated with by the call.
* @param[in] timeout_ms is the maximum wait time for any event. If any event happens before this time, the call will return immediately.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @see tcs_poll_remove()
*/
TcsResult tcs_poll_wait(struct TcsPoll* poll,
                        struct TcsPollEvent* out_events,
                        size_t events_length,
                        size_t* out_events_length,
                        int timeout_ms);

/**
* @brief Set parameters on a socket. It is recommended to use tcs_opt_*_set() instead.
*
* @param[in] socket is your in-out socket context.
* @param[in] level is the definition level.
* @param[in] option_name is the option name.
* @param[in] option_value is a pointer to the option value.
* @param[in] option_size is the byte size of the data pointed by @p option_value.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_set(TcsSocket socket,
                      int32_t level,
                      int32_t option_name,
                      const void* option_value,
                      size_t option_size);

/**
* @brief Get parameters on a socket. It is recommended to use tcs_opt_*_get() instead.
*
* @code
* uint8_t c;
* size_t a = sizeof(c);
* tcs_opt_get(socket, TCS_SOL_IP, TCS_IP_MULTICAST_LOOP, &c, &a);
* @endcode
*
* @param[in] socket is your in-out socket context.
* @param[in] level is the definition level.
* @param[in] option_name is the option name.
* @param[out] out_option_value is a pointer to receive the option value.
* @param[in,out] option_size is a pointer the byte size of the data pointed by @p out_option_value.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_get(TcsSocket socket, int32_t level, int32_t option_name, void* out_option_value, size_t* option_size);

/**
* @brief Query the socket type (e.g. ::TCS_SOCKET_STREAM or ::TCS_SOCKET_DGRAM).
*
* @param[in] socket socket to query.
* @param[out] out_type pointer to receive the socket type.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_type_get(TcsSocket socket, TcsSocketType* out_type);

/**
* @brief Enable the socket to be allowed to send to broadcast addresses.
*
* By default, sockets are not permitted to send to broadcast addresses (e.g.
* 255.255.255.255 or subnet broadcast addresses) as a safety measure to prevent
* accidental broadcast storms. Enable this option on UDP sockets that need to
* send broadcast traffic.
*
* Only valid for protocols that support broadcast, for example UDP. Default is false.
*
* @param[in] socket socket to enable/disable permission to send broadcast on.
* @param[in] do_allow_broadcast set to true to allow, false to forbid.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_broadcast_set(TcsSocket socket, bool do_allow_broadcast);

/**
* @brief Query whether broadcast is enabled on a socket.
*
* See tcs_opt_broadcast_set() for details on the broadcast option.
*
* @param[in] socket socket to query.
* @param[out] out_is_broadcast_allowed pointer to receive the current broadcast setting.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_broadcast_get(TcsSocket socket, bool* out_is_broadcast_allowed);

/**
* @brief Enable or disable TCP keep-alive on a socket.
*
* Keep-alive operates at the transport layer (TCP). When enabled, the OS periodically
* sends probe packets on an idle connection to detect if the remote peer is still
* reachable. Without keep-alive, a connection where neither side sends data can remain
* open indefinitely even if the remote host has crashed or the network path is broken.
*
* Keep-alive probes also prevent NAT routers and stateful firewalls from dropping
* idle connection mappings due to inactivity timeouts.
*
* This is particularly useful for long-lived connections that may be idle for extended
* periods, such as database connections or control channels.
*
* @param[in] socket socket to configure.
* @param[in] do_keep_alive set to true to enable, false to disable.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_keep_alive_set(TcsSocket socket, bool do_keep_alive);

/**
* @brief Query whether keep-alive is enabled on a socket.
*
* See tcs_opt_keep_alive_set() for details on what keep-alive does.
*
* @param[in] socket socket to query.
* @param[out] out_is_keep_alive_enabled pointer to receive the current keep-alive setting.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_keep_alive_get(TcsSocket socket, bool* out_is_keep_alive_enabled);

/**
* @brief Allow or disallow address reuse on a socket.
*
* **TCP:**
* Allows binding to a port that is in the TIME_WAIT state. When a TCP connection
* is closed, the port remains reserved for up to 2 minutes (2x Maximum Segment
* Lifetime) to ensure delayed packets from the old connection are not misinterpreted
* by a new one. This option bypasses that restriction, which is useful for server
* sockets that need to restart quickly.
* On POSIX, it also allows overlapping wildcard and specific address binds on the same
* port (e.g. both 0.0.0.0:8080 and 192.168.1.1:8080). The most specific address wins.
*
* **UDP:**
* Allows multiple sockets to bind to the same address and port. The behavior for
* unicast datagrams is OS-specific, but on most implementations only the most
* recently bound socket receives them.
* This is primarily useful for multicast, where several sockets need to receive the
* same group traffic.
*
* For load-balancing multiple sockets on the same port, see ::tcs_opt_reuse_port_set().
*
* @note On Windows, this library sets SO_EXCLUSIVEADDRUSE alongside SO_REUSEADDR
* for TCP sockets to prevent port hijacking, matching the security guarantees of
* POSIX. For UDP sockets, only SO_REUSEADDR is set, preserving multicast and
* same-port binding behavior. I.e. it mimics POSIX behaviour.
*
* @param[in] socket socket to configure.
* @param[in] do_allow_reuse_address set to true to allow, false to disallow.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_reuse_address_set(TcsSocket socket, bool do_allow_reuse_address);

/**
* @brief Query whether address reuse is enabled on a socket.
*
* See tcs_opt_reuse_address_set() for details on the address reuse option.
*
* @param[in] socket socket to query.
* @param[out] out_is_reuse_address_allowed pointer to receive the current setting.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_reuse_address_get(TcsSocket socket, bool* out_is_reuse_address_allowed);

/**
* @brief Allow or disallow multiple sockets to bind to the same address and port.
*
* When enabled, the kernel distributes incoming traffic across all sockets bound to
* the same address and port. All participating sockets must set this option, and on
* POSIX systems they must belong to the same effective UID.
*
* This is useful for multi-threaded or multi-process servers that want to load-balance
* incoming connections or datagrams across workers without application-level dispatching.
*
* @note Only supported on POSIX (Linux 3.9+). Returns #TCS_ERROR_NOT_SUPPORTED on Windows,
* which has no equivalent with the same semantics.
*
* @param[in] socket socket to configure.
* @param[in] do_allow_reuse_port set to true to allow, false to disallow.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_reuse_port_set(TcsSocket socket, bool do_allow_reuse_port);

/**
* @brief Query whether port reuse is enabled on a socket.
*
* See tcs_opt_reuse_port_set() for details.
*
* @note Returns #TCS_ERROR_NOT_SUPPORTED on Windows.
*
* @param[in] socket socket to query.
* @param[out] out_is_reuse_port_allowed pointer to receive the current setting.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_reuse_port_get(TcsSocket socket, bool* out_is_reuse_port_allowed);

/**
* @brief Set the send buffer size of a socket.
*
* @param[in] socket socket to configure.
* @param[in] send_buffer_size desired send buffer size in bytes.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_send_buffer_size_set(TcsSocket socket, size_t send_buffer_size);

/**
* @brief Query the send buffer size of a socket.
*
* @param[in] socket socket to query.
* @param[out] out_send_buffer_size pointer to receive the send buffer size in bytes.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_send_buffer_size_get(TcsSocket socket, size_t* out_send_buffer_size);

/**
* @brief Set the receive buffer size of a socket.
*
* @param[in] socket socket to configure.
* @param[in] receive_buffer_size desired receive buffer size in bytes.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_receive_buffer_size_set(TcsSocket socket, size_t receive_buffer_size);

/**
* @brief Query the receive buffer size of a socket.
*
* @param[in] socket socket to query.
* @param[out] out_receive_buffer_size pointer to receive the receive buffer size in bytes.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_receive_buffer_size_get(TcsSocket socket, size_t* out_receive_buffer_size);

/**
* @brief Set the receive timeout of a socket.
*
* @param[in] socket socket to configure.
* @param[in] timeout_ms timeout in milliseconds. Must be non-negative.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_receive_timeout_set(TcsSocket socket, int timeout_ms);

/**
* @brief Query the receive timeout of a socket.
*
* @param[in] socket socket to query.
* @param[out] out_timeout_ms pointer to receive the timeout in milliseconds.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_receive_timeout_get(TcsSocket socket, int* out_timeout_ms);

/**
* @brief Configure the linger behavior of a socket on close.
*
* @param[in] socket socket to configure.
* @param[in] do_linger set to true to enable lingering, false to disable.
* @param[in] timeout_seconds linger timeout in seconds (only used when do_linger is true).
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_linger_set(TcsSocket socket, bool do_linger, int timeout_seconds);

/**
* @brief Query the linger behavior of a socket.
*
* @param[in] socket socket to query.
* @param[out] out_do_linger pointer to receive whether lingering is enabled.
* @param[out] out_timeout_seconds pointer to receive the linger timeout in seconds.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_linger_get(TcsSocket socket, bool* out_do_linger, int* out_timeout_seconds);

/**
* @brief Enable or disable Nagle's algorithm (TCP_NODELAY).
*
* @param[in] socket socket to configure.
* @param[in] use_no_delay set to true to disable Nagle's algorithm (lower latency), false to enable it.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_ip_no_delay_set(TcsSocket socket, bool use_no_delay);

/**
* @brief Query whether Nagle's algorithm is disabled on a socket.
*
* @param[in] socket socket to query.
* @param[out] out_is_no_delay_used pointer to receive the current setting.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_ip_no_delay_get(TcsSocket socket, bool* out_is_no_delay_used);

/**
* @brief Enable or disable inline reception of out-of-band data.
*
* @param[in] socket socket to configure.
* @param[in] enable_oob set to true to receive OOB data inline, false to disable.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_out_of_band_inline_set(TcsSocket socket, bool enable_oob);

/**
* @brief Query whether out-of-band data is received inline.
*
* @param[in] socket socket to query.
* @param[out] out_is_oob_enabled pointer to receive the current setting.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_out_of_band_inline_get(TcsSocket socket, bool* out_is_oob_enabled);

/**
* @brief Set the socket priority.
*
* @note Not supported on Windows. Will return #TCS_ERROR_NOT_SUPPORTED on that platform.
*
* @param[in] socket socket to configure.
* @param[in] priority priority value.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_priority_set(TcsSocket socket, int priority);

/**
* @brief Query the socket priority.
*
* @note Not supported on Windows. Will return #TCS_ERROR_NOT_SUPPORTED on that platform.
*
* @param[in] socket socket to query.
* @param[out] out_priority pointer to receive the priority value.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_priority_get(TcsSocket socket, int* out_priority);

/**
* @brief Join a multicast group on a specific local interface.
*
* @param[in] socket socket to configure.
* @param[in] local_address local interface address to use.
* @param[in] multicast_address multicast group address to join.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_NOT_SUPPORTED if either address has an address family not supported on this platform.
*/
TcsResult tcs_opt_membership_add_to(TcsSocket socket,
                                    const struct TcsAddress* local_address,
                                    const struct TcsAddress* multicast_address);

/**
* @brief Leave a multicast group on a specific local interface.
*
* @param[in] socket socket to configure.
* @param[in] local_address local interface address used when joining.
* @param[in] multicast_address multicast group address to leave.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_NOT_SUPPORTED if either address has an address family not supported on this platform.
*/
TcsResult tcs_opt_membership_drop_from(TcsSocket socket,
                                       const struct TcsAddress* local_address,
                                       const struct TcsAddress* multicast_address);

/**
* @brief Join a multicast group using the default local interface.
*
* @param[in] socket socket to configure.
* @param[in] multicast_address multicast group address to join.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_NOT_SUPPORTED if multicast_address has an address family not supported on this platform.
*/
TcsResult tcs_opt_membership_add(TcsSocket socket, const struct TcsAddress* multicast_address);

/**
* @brief Join a multicast group by address string.
*
* Resolves the multicast address string and joins the group using the default local interface.
*
* @param[in] socket socket to configure.
* @param[in] multicast_address multicast group address string (e.g. "239.1.2.3" or "ff02::1").
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if multicast_address is NULL.
* @retval #TCS_ERROR_ADDRESS_LOOKUP_FAILED if the address string could not be resolved.
* @see tcs_opt_membership_add()
* @see tcs_opt_membership_drop_str()
*/
TcsResult tcs_opt_membership_add_str(TcsSocket socket, const char* multicast_address);

/**
* @brief Leave a multicast group using the default local interface.
*
* @param[in] socket socket to configure.
* @param[in] multicast_address multicast group address to leave.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_NOT_SUPPORTED if multicast_address has an address family not supported on this platform.
*/
TcsResult tcs_opt_membership_drop(TcsSocket socket, const struct TcsAddress* multicast_address);

/**
* @brief Leave a multicast group by address string.
*
* Parses the multicast address string and leaves the group using the default local interface.
*
* @param[in] socket socket to configure.
* @param[in] multicast_address multicast group address string (e.g. "239.1.2.3" or "ff02::1").
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_INVALID_ARGUMENT if multicast_address is NULL.
* @retval #TCS_ERROR_ADDRESS_LOOKUP_FAILED if the address string could not be resolved.
* @see tcs_opt_membership_drop()
* @see tcs_opt_membership_add_str()
*/
TcsResult tcs_opt_membership_drop_str(TcsSocket socket, const char* multicast_address);

/**
* @brief Set the outgoing interface for multicast packets.
*
* @param[in] socket socket to configure.
* @param[in] local_address local interface address to use for outgoing multicast.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_NOT_SUPPORTED if local_address has an address family not supported on this platform.
*/
TcsResult tcs_opt_multicast_interface_set(TcsSocket socket, const struct TcsAddress* local_address);

/**
* @brief Enable or disable multicast loopback.
*
* When enabled, multicast packets sent on this socket are looped back and
* delivered to local receivers on the same host.
*
* @param[in] socket socket to configure.
* @param[in] do_loopback set to true to enable loopback, false to disable.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_multicast_loop_set(TcsSocket socket, bool do_loopback);

/**
* @brief Get the current multicast loopback setting.
*
* @param[in] socket socket to query.
* @param[out] out_is_loopback pointer to receive the current setting.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_multicast_loop_get(TcsSocket socket, bool* out_is_loopback);

/**
* @brief Set a socket to non-blocking or blocking mode.
*
* @param[in] socket socket to configure.
* @param[in] do_nonblocking set to true for non-blocking, false for blocking.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_opt_nonblocking_set(TcsSocket socket, bool do_nonblocking);

/**
* @brief Query the non-blocking state of a socket.
*
* @note Not supported on Windows. Will return #TCS_ERROR_NOT_SUPPORTED on that platform.
*/
TcsResult tcs_opt_nonblocking_get(TcsSocket socket, bool* out_is_nonblocking);

/**
* @brief List available network interfaces.
*
* @param[out] out_interfaces array to receive interface information, or NULL to only count.
* @param[in] interfaces_length number of elements in the @p out_interfaces array.
* @param[out] out_length pointer to receive the total number of interfaces available, which may exceed @p interfaces_length.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_interface_list(struct TcsInterface out_interfaces[], size_t interfaces_length, size_t* out_length);

/**
* @brief Resolve a hostname to one or more addresses.
*
* @param[in] hostname hostname or IP string to resolve.
* @param[in] address_family address family filter, or ::TCS_FAMILY_ANY for all.
* @param[out] out_addresses array to receive resolved addresses, or NULL to only count.
* @param[in] addresses_length number of elements in the @p out_addresses array.
* @param[out] out_length pointer to receive the number of addresses found.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @retval #TCS_ERROR_NOT_SUPPORTED if @p address_family is not supported on this platform (e.g. ::TCS_FAMILY_PACKET on non-Linux).
*/
TcsResult tcs_address_resolve(const char* hostname,
                              TcsFamily address_family,
                              struct TcsAddress out_addresses[],
                              size_t addresses_length,
                              size_t* out_length);

/**
* @brief List addresses associated with network interfaces.
*
* @param[in] interface_id_filter interface ID to filter by, or 0 for all interfaces.
* @param[in] address_family_filter address family filter, or ::TCS_FAMILY_ANY for all.
* @param[out] out_interface_addresses array to receive results, or NULL to only count.
* @param[in] interface_addresses_length number of elements in the @p out_interface_addresses array.
* @param[out] out_length pointer to receive the total number of results available, which may exceed @p interface_addresses_length.
* @return #TCS_SUCCESS if successful, otherwise the error code.
* @note If @p address_family_filter is not supported on this platform (e.g. ::TCS_FAMILY_PACKET on non-Linux), no entries match and *out_length is 0.
*/
TcsResult tcs_address_list(TcsInterfaceId interface_id_filter,
                           TcsFamily address_family_filter,
                           struct TcsInterfaceAddress out_interface_addresses[],
                           size_t interface_addresses_length,
                           size_t* out_length);

/**
* @brief Get the local address of a bound or connected socket.
*
* @param[in] socket socket to query.
* @param[out] out_local_address pointer to receive the local address.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_address_socket_local(TcsSocket socket, struct TcsAddress* out_local_address);

/**
* @brief Get the remote address of a connected socket.
*
* @param[in] socket socket to query.
* @param[out] out_remote_address pointer to receive the remote address.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_address_socket_remote(TcsSocket socket, struct TcsAddress* out_remote_address);

/**
* @brief Get the address family of a socket.
*
* @param[in] socket socket to query.
* @param[out] out_family pointer to receive the address family.
* @return #TCS_SUCCESS if successful, otherwise the error code.
*/
TcsResult tcs_address_socket_family(TcsSocket socket, TcsFamily* out_family);

/**
 * @brief Parse a network address from a string.
 *
 * Supports IPv4, IPv6 (RFC 4291 all three forms), MAC, and bracket/port notation (RFC 3986).
 * IPv6 zone IDs are limited to numeric values per the minimum requirement of RFC 4007.
 * String-based zone IDs (e.g. "%%eth0") are not supported.
 *
 * Examples:
 * @verbatim
 * 192.168.0.1:1212
 * ::1
 * [::1]:443
 * fe80::1%3
 * ::ffff:192.168.1.1
 * 91:E0:F0:00:FE:00
 * @endverbatim
 *
 * Note that this function will not perform DNS resolution. Use ::tcs_address_resolve() for that.
 *
 * @param[in] str The string to parse.
 * @param[out] out_address The parsed address.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsResult tcs_address_parse(const char str[], struct TcsAddress* out_address);

/**
 * @brief Convert an address to a string.
 * 
 * This will make a verbose string representation of the address.
 * 
 * @param[in] address the address to convert.
 * @param[out] out_str buffer of at least 70 bytes to receive the string representation.
 * @return #TCS_SUCCESS if successful, otherwise the error code.
 */
TcsResult tcs_address_to_str(const struct TcsAddress* address, char out_str[70]);

/** @brief Check if two addresses are equal. Returns false for NULL, mismatched, unknown, or unsupported address families. */
bool tcs_address_is_equal(const struct TcsAddress* l, const struct TcsAddress* r);

/** @brief Check if the address is a wildcard (any) address. Returns false for NULL, unknown, or unsupported address families. */
bool tcs_address_is_any(const struct TcsAddress* addr);

/** @brief Check if the address is a link-local address. Returns false for NULL, unknown, or unsupported address families. */
bool tcs_address_is_link_local(const struct TcsAddress* addr);

/** @brief Check if the address is a loopback address. Returns false for NULL, unknown, or unsupported address families. */
bool tcs_address_is_loopback(const struct TcsAddress* addr);

/** @brief Check if the address is a multicast address. Returns false for NULL, unknown, or unsupported address families. */
bool tcs_address_is_multicast(const struct TcsAddress* addr);

/** @brief Check if the address is a broadcast address. Returns false for NULL, unknown, or unsupported address families. */
bool tcs_address_is_broadcast(const struct TcsAddress* addr);

/** @brief Check if the address family is known and supported by this platform. Returns false for NULL or unknown families. */
bool tcs_address_is_supported(const struct TcsAddress* addr);

#ifdef __cplusplus
}
#endif

#endif
