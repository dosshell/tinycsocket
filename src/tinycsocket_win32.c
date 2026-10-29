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
#include "tinycsocket_internal.h"
#endif
#ifdef TINYCSOCKET_USE_WIN32_IMPL

#ifdef DO_WRAP
#include "dbg_wrap.h"
#endif

// Header only should not need other files
#ifndef TINYDATASTRUCTURES_H_
#include "tinydatastructures.h"
#endif
// before windows.h
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h> // sockets

#include <windows.h>

// after windows.h
#include <iphlpapi.h> // GetAdaptersAddresses
#include <ws2tcpip.h> // getaddrinfo

#include <limits.h> // INT_MAX
#include <stdio.h>  // fprintf (debug diagnostics)
#include <stdlib.h> // Malloc for GetAdaptersAddresses
#include <string.h> // memset

// SOCKADDR_STORAGE was introduced in Windows XP (0x0501).
// For Win2K targets with MSVC, define it ourselves.
// mingw-w64 always defines it, so check _MSC_VER to avoid redefinition.
#if defined(_MSC_VER) && defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0501
typedef struct
{
    short ss_family;
    char __ss_pad1[6];
    __int64 __ss_align;
    char __ss_pad2[112];
} SOCKADDR_STORAGE, *PSOCKADDR_STORAGE;
#endif

#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#endif

#ifndef ULIST_SOC
#define ULIST_SOC
TDS_ULIST_IMPL(SOCKET, soc)
#endif

#ifndef ULIST_PVOID
#define ULIST_PVOID
TDS_ULIST_IMPL(void*, pvoid)
#endif

#ifndef TDS_MAP_socket_pvoid
#define TDS_MAP_socket_pvoid
TDS_MAP_IMPL(SOCKET, void*, socket_user)
#endif

// Needs to be compatible with fd_set, hopefully this works. Only used when FD_SETSIZE is to small.
struct tcs_fd_set
{
    u_int fd_count;
    SOCKET fd_array[1]; // dynamic memory hack that is compatible with Win32 API fd_set
};

struct TcsPoll
{
    struct TdsUList_soc read_sockets;
    struct TdsUList_soc write_sockets;
    struct TdsUList_soc error_sockets;
    struct TdsMap_socket_user user_data;
};

const TcsSocket TCS_SOCKET_INVALID = INVALID_SOCKET;
const int32_t TCS_WAIT_INF = -1;

// Addresses
const TcsAddressIpv4 TCS_ADDRESS_IPV4_ANY = INADDR_ANY;
const TcsAddressIpv4 TCS_ADDRESS_IPV4_LOOPBACK = INADDR_LOOPBACK;
const TcsAddressIpv4 TCS_ADDRESS_IPV4_BROADCAST = INADDR_BROADCAST;
const TcsAddressIpv4 TCS_ADDRESS_IPV4_NONE = INADDR_NONE;

const struct TcsAddressIpv6 TCS_ADDRESS_IPV6_ANY = {{0}};
const struct TcsAddressIpv6 TCS_ADDRESS_IPV6_LOOPBACK = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};

// Family
const TcsFamily TCS_FAMILY_ANY = {AF_UNSPEC};
const TcsFamily TCS_FAMILY_IPV4 = {AF_INET};
const TcsFamily TCS_FAMILY_IPV6 = {AF_INET6};
// Windows has no AF_PACKET. Packet sockets are emulated with Npcap, see tcs_lib_init(). The value never reaches
// Winsock, it only needs to differ from every AF_* value.
const TcsFamily TCS_FAMILY_PACKET = {1000};

// Type
const TcsSocketType TCS_SOCKET_STREAM = {SOCK_STREAM};
const TcsSocketType TCS_SOCKET_DGRAM = {SOCK_DGRAM};
const TcsSocketType TCS_SOCKET_RAW = {SOCK_RAW};

// Recv flags
const uint32_t TCS_MSG_PEEK = MSG_PEEK;
const uint32_t TCS_MSG_OOB = MSG_OOB;
const uint32_t TCS_MSG_WAITALL = 0x8; // Binary compatible when it does not exist

// Send flags
const uint32_t TCS_MSG_SENDALL = 0x80000000;

// Backlog
const int TCS_BACKLOG_MAX = SOMAXCONN;

// Option levels
const int32_t TCS_SOL_SOCKET = SOL_SOCKET;
const int32_t TCS_SOL_IP = IPPROTO_IP;
const int32_t TCS_SOL_TCP = IPPROTO_TCP;
const int32_t TCS_SOL_PACKET = -1; // No equivalent on Windows

// Socket options
const int32_t TCS_SO_TYPE = SO_TYPE;
const int32_t TCS_SO_BROADCAST = SO_BROADCAST;
const int32_t TCS_SO_KEEPALIVE = SO_KEEPALIVE;
const int32_t TCS_SO_LINGER = SO_LINGER;
const int32_t TCS_SO_REUSEADDR = SO_REUSEADDR;
const int32_t TCS_SO_REUSEPORT = -1;
const int32_t TCS_SO_RCVBUF = SO_RCVBUF;
const int32_t TCS_SO_RCVTIMEO = SO_RCVTIMEO;
const int32_t TCS_SO_SNDBUF = SO_SNDBUF;
const int32_t TCS_SO_OOBINLINE = SO_OOBINLINE;
const int32_t TCS_SO_PRIORITY = -1;

// IP options
const int32_t TCS_IP_MEMBERSHIP_ADD = IP_ADD_MEMBERSHIP;
const int32_t TCS_IP_MEMBERSHIP_DROP = IP_DROP_MEMBERSHIP;
const int32_t TCS_IP_MULTICAST_LOOP = IP_MULTICAST_LOOP;

// TCP options
const int32_t TCS_TCP_NODELAY = TCP_NODELAY;

// ######## Internal Helpers ########

static TcsResult wsaerror2retcode(int wsa_error)
{
    switch (wsa_error)
    {
        case WSANOTINITIALISED:
            return TCS_ERROR_LIBRARY_NOT_INITIALIZED;
        case WSAEWOULDBLOCK:
            return TCS_ERROR_WOULD_BLOCK;
        case WSAETIMEDOUT:
            return TCS_ERROR_TIMED_OUT;
        case WSAECONNREFUSED:
            return TCS_ERROR_CONNECTION_REFUSED;
        case WSAECONNRESET:
            return TCS_ERROR_CONNECTION_RESET;
        case WSAENOTCONN:
            return TCS_ERROR_NOT_CONNECTED;
        case WSAESHUTDOWN:
            return TCS_ERROR_SOCKET_CLOSED;
        case WSAENETUNREACH:
        case WSAEHOSTUNREACH:
        case WSAENETDOWN:
            return TCS_ERROR_NETWORK_UNREACHABLE;
        case WSAEACCES:
            return TCS_ERROR_PERMISSION_DENIED;
        case WSAEINVAL:
            return TCS_ERROR_INVALID_ARGUMENT;
        case WSAEADDRINUSE:
            return TCS_ERROR_ADDRESS_IN_USE;
        case WSAENOPROTOOPT:
            return TCS_ERROR_NOT_SUPPORTED;
        case WSAEAFNOSUPPORT:
        case WSAEPROTONOSUPPORT:
        case WSAESOCKTNOSUPPORT:
            return TCS_ERROR_NOT_SUPPORTED;
        case WSAENOBUFS:
        case WSA_NOT_ENOUGH_MEMORY:
            return TCS_ERROR_MEMORY;
        default:
            return TCS_ERROR_UNKNOWN;
    }
}

static TcsResult socketstatus2retcode(int status)
{
    if (status == 0)
    {
        return TCS_SUCCESS;
    }
    else if (status == SOCKET_ERROR)
    {
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }
    else
    {
        return TCS_ERROR_UNKNOWN;
    }
}

// Lists the adapters with GetAdaptersAddresses(). *out_adapters is NULL when there are none. Free it with free().
static TcsResult adapters_get(PIP_ADAPTER_ADDRESSES* out_adapters)
{
    const int MAX_TRIES = 5;
    ULONG buffer_size = 15000; // From msdn recommendation
    PIP_ADAPTER_ADDRESSES adapters = NULL;
    ULONG adapter_sts = ERROR_NO_DATA;
    for (int i = 0; i < MAX_TRIES; ++i)
    {
        adapters = (PIP_ADAPTER_ADDRESSES)malloc(buffer_size);
        if (adapters == NULL)
            return TCS_ERROR_MEMORY;
        adapter_sts = GetAdaptersAddresses(AF_UNSPEC,
                                           GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                           NULL,
                                           adapters,
                                           &buffer_size);
        if (adapter_sts != ERROR_BUFFER_OVERFLOW)
            break;
        free(adapters);
        adapters = NULL;
    }
    if (adapter_sts == ERROR_NO_DATA)
    {
        free(adapters);
        *out_adapters = NULL;
        return TCS_SUCCESS;
    }
    if (adapter_sts != NO_ERROR)
    {
        // Debug: log the actual Windows error code for diagnostics
        fprintf(stderr, "GetAdaptersAddresses failed with error: %lu (0x%lX)\n", adapter_sts, adapter_sts);
        free(adapters);
        return TCS_ERROR_UNKNOWN;
    }
    *out_adapters = adapters;
    return TCS_SUCCESS;
}

static TcsResult sockaddr2native(const struct TcsAddress* in_addr, PSOCKADDR out_addr, int* out_addrlen)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->family.native == TCS_FAMILY_ANY.native)
    {
        return TCS_ERROR_NOT_SUPPORTED;
    }
    if (in_addr->family.native == TCS_FAMILY_IPV4.native)
    {
        PSOCKADDR_IN addr = (PSOCKADDR_IN)out_addr;
        addr->sin_family = (ADDRESS_FAMILY)AF_INET;
        addr->sin_port = htons((USHORT)in_addr->data.ipv4.port);
        addr->sin_addr.S_un.S_addr = htonl((ULONG)in_addr->data.ipv4.address);

        if (out_addrlen != NULL)
            *out_addrlen = sizeof(SOCKADDR_IN);

        return TCS_SUCCESS;
    }
    else if (in_addr->family.native == TCS_FAMILY_IPV6.native)
    {
        PSOCKADDR_IN6 addr = (PSOCKADDR_IN6)out_addr;
        addr->sin6_family = (ADDRESS_FAMILY)AF_INET6;
        addr->sin6_port = htons((USHORT)in_addr->data.ipv6.port);
        memcpy(&addr->sin6_addr, in_addr->data.ipv6.address.bytes, 16);
        addr->sin6_scope_id = (ULONG)in_addr->data.ipv6.scope_id;
        if (out_addrlen != NULL)
            *out_addrlen = sizeof(SOCKADDR_IN6);
        return TCS_SUCCESS;
    }
    else if (in_addr->family.native == TCS_FAMILY_PACKET.native)
    {
        return TCS_ERROR_NOT_SUPPORTED; // Packet addresses have no sockaddr on Windows, only packet sockets take them
    }
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult native2sockaddr(const PSOCKADDR in_addr, struct TcsAddress* out_addr)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->sa_family == AF_INET)
    {
        PSOCKADDR_IN addr = (PSOCKADDR_IN)in_addr;
        out_addr->family = TCS_FAMILY_IPV4;
        out_addr->data.ipv4.port = ntohs((uint16_t)addr->sin_port);
        out_addr->data.ipv4.address = ntohl((uint32_t)addr->sin_addr.S_un.S_addr);
    }
    else if (in_addr->sa_family == AF_INET6)
    {
        PSOCKADDR_IN6 addr = (PSOCKADDR_IN6)in_addr;
        out_addr->family = TCS_FAMILY_IPV6;
        out_addr->data.ipv6.port = ntohs((uint16_t)addr->sin6_port);
        memcpy(out_addr->data.ipv6.address.bytes, &addr->sin6_addr, 16);
        out_addr->data.ipv6.scope_id = (TcsInterfaceId)addr->sin6_scope_id;
    }
    else if (in_addr->sa_family == AF_UNSPEC)
    {
        return TCS_ERROR_INVALID_ARGUMENT;
    }
    else
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }

    return TCS_SUCCESS;
}

// ######## Packet Sockets ########
//
// Windows has no AF_PACKET. A packet socket is an index into a table with the top bit set, which can never collide
// with a kernel SOCKET handle. The public functions test that bit with tcs_internal_is_socket_packet() and hand
// packet sockets to the tcs_internal_packet_*_impl function pointers below, everything else goes to Winsock.
// Nothing outside the Npcap section touches Npcap.

#define TCS_PACKET_SOCKET_TAG ((TcsSocket)1 << (sizeof(TcsSocket) * 8 - 1))

static bool tcs_internal_is_socket_packet(TcsSocket socket)
{
    return socket != TCS_SOCKET_INVALID && (socket & TCS_PACKET_SOCKET_TAG) != 0;
}

// Packet socket operations go through these function pointers, so that the implementation can be chosen at runtime.
// They start at the tcs_internal_packet_*_impl_not_supported() functions below. The Npcap section points them at
// its implementation when tcs_lib_init() has loaded Npcap, and back when tcs_lib_cleanup() has unloaded it.

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100) // Unreferenced formal parameter, the stubs ignore their arguments
#endif

static TcsResult tcs_internal_packet_socket_impl_not_supported(TcsSocket* out_socket,
                                                               TcsFamily family,
                                                               TcsSocketType type,
                                                               TcsProtocol protocol)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_close_impl_not_supported(TcsSocket* socket)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_bind_impl_not_supported(TcsSocket socket, const struct TcsAddress* address)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_send_impl_not_supported(TcsSocket socket,
                                                             const uint8_t* buffer,
                                                             size_t buffer_size,
                                                             uint32_t flags,
                                                             size_t* sent_size)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_send_to_impl_not_supported(TcsSocket socket,
                                                                const uint8_t* buffer,
                                                                size_t buffer_size,
                                                                uint32_t flags,
                                                                const struct TcsAddress* destination_address,
                                                                size_t* sent_size)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_sendv_impl_not_supported(TcsSocket socket,
                                                              const struct TcsIoVec* iov,
                                                              size_t iov_length,
                                                              uint32_t flags,
                                                              size_t* sent_size)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_receive_from_impl_not_supported(TcsSocket socket,
                                                                     uint8_t* buffer,
                                                                     size_t buffer_size,
                                                                     uint32_t flags,
                                                                     struct TcsAddress* source_address,
                                                                     size_t* received_size)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_opt_get_impl_not_supported(TcsSocket socket,
                                                                int32_t level,
                                                                int32_t option_name,
                                                                void* out_option_value,
                                                                size_t* option_size)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_opt_receive_timeout_set_impl_not_supported(TcsSocket socket, int timeout_ms)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_opt_receive_timeout_get_impl_not_supported(TcsSocket socket, int* timeout_ms)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_opt_nonblocking_set_impl_not_supported(TcsSocket socket, bool do_non_blocking)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_opt_nonblocking_get_impl_not_supported(TcsSocket socket, bool* is_non_blocking)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_opt_membership_add_to_impl_not_supported(
    TcsSocket socket,
    const struct TcsAddress* local_address,
    const struct TcsAddress* multicast_address)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_opt_membership_drop_from_impl_not_supported(
    TcsSocket socket,
    const struct TcsAddress* local_address,
    const struct TcsAddress* multicast_address)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static TcsResult tcs_internal_packet_address_socket_local_impl_not_supported(TcsSocket socket,
                                                                             struct TcsAddress* local_address)
{
    return TCS_ERROR_NOT_SUPPORTED;
}

static bool tcs_internal_packet_address_is_supported_impl_not_supported(const struct TcsAddress* addr)
{
    return false;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

static TcsResult (*tcs_internal_packet_socket_impl)(TcsSocket* out_socket,
                                                    TcsFamily family,
                                                    TcsSocketType type,
                                                    TcsProtocol protocol) =
    tcs_internal_packet_socket_impl_not_supported;
static TcsResult (*tcs_internal_packet_close_impl)(TcsSocket* socket) = tcs_internal_packet_close_impl_not_supported;
static TcsResult (*tcs_internal_packet_bind_impl)(TcsSocket socket, const struct TcsAddress* address) =
    tcs_internal_packet_bind_impl_not_supported;
static TcsResult (*tcs_internal_packet_send_impl)(TcsSocket socket,
                                                  const uint8_t* buffer,
                                                  size_t buffer_size,
                                                  uint32_t flags,
                                                  size_t* sent_size) = tcs_internal_packet_send_impl_not_supported;
static TcsResult (*tcs_internal_packet_send_to_impl)(TcsSocket socket,
                                                     const uint8_t* buffer,
                                                     size_t buffer_size,
                                                     uint32_t flags,
                                                     const struct TcsAddress* destination_address,
                                                     size_t* sent_size) =
    tcs_internal_packet_send_to_impl_not_supported;
static TcsResult (*tcs_internal_packet_sendv_impl)(TcsSocket socket,
                                                   const struct TcsIoVec* iov,
                                                   size_t iov_length,
                                                   uint32_t flags,
                                                   size_t* sent_size) = tcs_internal_packet_sendv_impl_not_supported;
static TcsResult (*tcs_internal_packet_receive_from_impl)(TcsSocket socket,
                                                          uint8_t* buffer,
                                                          size_t buffer_size,
                                                          uint32_t flags,
                                                          struct TcsAddress* source_address,
                                                          size_t* received_size) =
    tcs_internal_packet_receive_from_impl_not_supported;
static TcsResult (*tcs_internal_packet_opt_get_impl)(TcsSocket socket,
                                                     int32_t level,
                                                     int32_t option_name,
                                                     void* out_option_value,
                                                     size_t* option_size) =
    tcs_internal_packet_opt_get_impl_not_supported;
static TcsResult (*tcs_internal_packet_opt_receive_timeout_set_impl)(TcsSocket socket, int timeout_ms) =
    tcs_internal_packet_opt_receive_timeout_set_impl_not_supported;
static TcsResult (*tcs_internal_packet_opt_receive_timeout_get_impl)(TcsSocket socket, int* timeout_ms) =
    tcs_internal_packet_opt_receive_timeout_get_impl_not_supported;
static TcsResult (*tcs_internal_packet_opt_nonblocking_set_impl)(TcsSocket socket, bool do_non_blocking) =
    tcs_internal_packet_opt_nonblocking_set_impl_not_supported;
static TcsResult (*tcs_internal_packet_opt_nonblocking_get_impl)(TcsSocket socket, bool* is_non_blocking) =
    tcs_internal_packet_opt_nonblocking_get_impl_not_supported;
static TcsResult (*tcs_internal_packet_opt_membership_add_to_impl)(TcsSocket socket,
                                                                   const struct TcsAddress* local_address,
                                                                   const struct TcsAddress* multicast_address) =
    tcs_internal_packet_opt_membership_add_to_impl_not_supported;
static TcsResult (*tcs_internal_packet_opt_membership_drop_from_impl)(TcsSocket socket,
                                                                      const struct TcsAddress* local_address,
                                                                      const struct TcsAddress* multicast_address) =
    tcs_internal_packet_opt_membership_drop_from_impl_not_supported;
static TcsResult (*tcs_internal_packet_address_socket_local_impl)(TcsSocket socket, struct TcsAddress* local_address) =
    tcs_internal_packet_address_socket_local_impl_not_supported;
static bool (*tcs_internal_packet_address_is_supported_impl)(const struct TcsAddress* addr) =
    tcs_internal_packet_address_is_supported_impl_not_supported;

// ######## Packet Sockets: Npcap ########
//
// Packet sockets are emulated on top of Npcap, whose wpcap.dll is loaded at runtime by tcs_lib_init(). Nothing from
// the Npcap SDK is needed at build time: the few pcap types and functions used are declared here, and their ABI has
// been frozen since WinPcap.
//
// Please be aware of npcap license issues.
//
// Supported: Npcap 1.00 (2020) or later.
//
// Wine is shipped with a custom npcap implementation that is currently not supported, a WaitForSingleObject workaround is needed.

typedef struct pcap pcap_t;

struct pcap_pkthdr
{
    struct timeval ts;
    unsigned int caplen;
    unsigned int len;
};

struct bpf_program
{
    unsigned int bf_len;
    struct bpf_insn* bf_insns;
};

#define TCS_PCAP_ERRBUF_SIZE 256
#define TCS_PCAP_NETMASK_UNKNOWN 0xffffffffu
#define TCS_PCAP_DLT_EN10MB 1
#define TCS_PCAP_ERROR_NO_SUCH_DEVICE (-5)
#define TCS_PCAP_ERROR_PERM_DENIED (-8)
#define TCS_PCAP_ERROR_IFACE_NOT_UP (-9)
#define TCS_PCAP_ERROR_PROMISC_PERM_DENIED (-11)
#define TCS_PCAP_OPENFLAG_PROMISCUOUS 1
#define TCS_PCAP_OPENFLAG_NOCAPTURE_LOCAL 8
#define TCS_PCAP_OPENFLAG_MAX_RESPONSIVENESS 16

struct pcap_rmtauth; // Only ever passed as NULL
typedef pcap_t*(__cdecl* TcsInternalPcapOpen)(const char*, int, int, int, struct pcap_rmtauth*, char*);
typedef const char*(__cdecl* TcsInternalPcapStatusToStr)(int);
typedef int(__cdecl* TcsInternalPcapGetInt)(pcap_t*);
typedef int(__cdecl* TcsInternalPcapSetNonblock)(pcap_t*, int, char*);
typedef HANDLE(__cdecl* TcsInternalPcapGetEvent)(pcap_t*);
typedef int(__cdecl* TcsInternalPcapCompile)(pcap_t*, struct bpf_program*, const char*, int, unsigned int);
typedef int(__cdecl* TcsInternalPcapSetFilter)(pcap_t*, struct bpf_program*);
typedef void(__cdecl* TcsInternalPcapFreeCode)(struct bpf_program*);
typedef int(__cdecl* TcsInternalPcapNextEx)(pcap_t*, struct pcap_pkthdr**, const unsigned char**);
typedef int(__cdecl* TcsInternalPcapSendPacket)(pcap_t*, const unsigned char*, int);
typedef void(__cdecl* TcsInternalPcapClose)(pcap_t*);

struct TcsInternalNpcapSocket
{
    pcap_t* pcap; // NULL until bound
    int type;     // SOCK_DGRAM or SOCK_RAW
    uint16_t protocol;
    TcsInterfaceId interface_id;
    uint8_t mac[6];         // The adapter's own address, source of DGRAM frames
    int receive_timeout_ms; // 0 waits forever
    bool nonblocking;
    uint8_t* groups; // Joined multicast addresses, 6 bytes each
    size_t group_count;
};

#ifndef TCS_CFG_PACKET_SOCKETS_MAX
#define TCS_CFG_PACKET_SOCKETS_MAX 64
#endif

static struct
{
    HMODULE module;
    int init_count;
    TcsInternalPcapOpen open;
    TcsInternalPcapStatusToStr statustostr;
    TcsInternalPcapGetInt datalink;
    TcsInternalPcapSetNonblock setnonblock;
    TcsInternalPcapGetEvent getevent;
    TcsInternalPcapCompile compile;
    TcsInternalPcapSetFilter setfilter;
    TcsInternalPcapFreeCode freecode;
    TcsInternalPcapNextEx next_ex;
    TcsInternalPcapSendPacket sendpacket;
    TcsInternalPcapClose close;
    struct TcsInternalNpcapSocket* sockets[TCS_CFG_PACKET_SOCKETS_MAX]; // Indexed by the handle without its tag bit
} g_npcap;

// The state behind a socket for which tcs_internal_is_socket_packet() is true, NULL once it has been closed
static struct TcsInternalNpcapSocket* tcs_internal_npcap_socket_get(TcsSocket socket)
{
    TcsSocket index = socket & ~TCS_PACKET_SOCKET_TAG;
    if (index >= TCS_CFG_PACKET_SOCKETS_MAX)
        return NULL;
    return g_npcap.sockets[index];
}

// Installs a BPF filter giving the socket the frames a Linux packet socket with the same protocol and memberships
// would see. Npcap cannot program the adapter's multicast filter, so memberships need promiscuous mode and the filter
// then also limits the destinations to the adapter's own address, broadcast and the joined groups.
static TcsResult tcs_internal_npcap_socket_filter(const struct TcsInternalNpcapSocket* ps, pcap_t* pcap)
{
    bool filter_protocol = ps->protocol != TCS_PROTOCOL_ETH_ALL;
    if (!filter_protocol && ps->group_count == 0)
        return TCS_SUCCESS;

    size_t expression_size = 128 + 36 * (ps->group_count + 1);
    char* expression = (char*)malloc(expression_size);
    if (expression == NULL)
        return TCS_ERROR_MEMORY;
    size_t pos = 0;
    if (ps->group_count > 0)
    {
        struct TcsAddress mac = TCS_ADDRESS_NONE;
        mac.family = TCS_FAMILY_PACKET;
        char mac_str[18];
        memcpy(mac.data.packet.mac, ps->mac, 6);
        tcs_address_to_str(&mac, mac_str, sizeof mac_str, NULL);
        pos += (size_t)snprintf(expression + pos, expression_size - pos, "(ether dst %s or ether broadcast", mac_str);
        for (size_t i = 0; i < ps->group_count; ++i)
        {
            memcpy(mac.data.packet.mac, ps->groups + i * 6, 6);
            tcs_address_to_str(&mac, mac_str, sizeof mac_str, NULL);
            pos += (size_t)snprintf(expression + pos, expression_size - pos, " or ether dst %s", mac_str);
        }
        pos += (size_t)snprintf(expression + pos, expression_size - pos, "%s", filter_protocol ? ") and " : ")");
    }
    if (filter_protocol)
    {
        // Tagged frames are matched too, Linux delivers them untagged with the inner protocol
        snprintf(expression + pos,
                 expression_size - pos,
                 "(ether proto 0x%04x or (vlan and ether proto 0x%04x))",
                 (unsigned int)ps->protocol,
                 (unsigned int)ps->protocol);
    }

    struct bpf_program program;
    memset(&program, 0, sizeof program);
    int compile_sts = g_npcap.compile(pcap, &program, expression, 1, TCS_PCAP_NETMASK_UNKNOWN);
    free(expression);
    if (compile_sts != 0)
        return TCS_ERROR_SYSTEM;
    int setfilter_sts = g_npcap.setfilter(pcap, &program);
    g_npcap.freecode(&program);
    return setfilter_sts == 0 ? TCS_SUCCESS : TCS_ERROR_SYSTEM;
}

// Opens the Npcap handle of a packet socket from its current state, replacing the previous handle on success
static TcsResult tcs_internal_npcap_socket_open(struct TcsInternalNpcapSocket* ps)
{
    PIP_ADAPTER_ADDRESSES adapters = NULL;
    TcsResult adapters_sts = adapters_get(&adapters);
    if (adapters_sts != TCS_SUCCESS)
        return adapters_sts;

    // Npcap names its devices after the adapter GUID
    const char prefix[] = "\\Device\\NPF_";
    char device[sizeof prefix + MAX_ADAPTER_NAME_LENGTH + 4];
    bool found = false;
    for (PIP_ADAPTER_ADDRESSES iter = adapters; iter != NULL && !found; iter = iter->Next)
    {
        size_t name_size = strlen(iter->AdapterName) + 1;
        if (iter->IfIndex != ps->interface_id || iter->PhysicalAddressLength != 6 ||
            sizeof prefix - 1 + name_size > sizeof device)
            continue;
        memcpy(device, prefix, sizeof prefix - 1);
        memcpy(device + sizeof prefix - 1, iter->AdapterName, name_size);
        memcpy(ps->mac, iter->PhysicalAddress, 6);
        found = true;
    }
    free(adapters);
    if (!found)
        return TCS_ERROR_INVALID_ARGUMENT;

    int flags = TCS_PCAP_OPENFLAG_NOCAPTURE_LOCAL | TCS_PCAP_OPENFLAG_MAX_RESPONSIVENESS;
    if (ps->group_count > 0)
        flags |= TCS_PCAP_OPENFLAG_PROMISCUOUS;
    char errbuf[TCS_PCAP_ERRBUF_SIZE] = "";
    pcap_t* pcap = g_npcap.open(device, 65535, flags, 0, NULL, errbuf);
    if (pcap == NULL)
    {
        if (strstr(errbuf, g_npcap.statustostr(TCS_PCAP_ERROR_PERM_DENIED)) != NULL ||
            strstr(errbuf, g_npcap.statustostr(TCS_PCAP_ERROR_PROMISC_PERM_DENIED)) != NULL)
            return TCS_ERROR_PERMISSION_DENIED;
        if (strstr(errbuf, g_npcap.statustostr(TCS_PCAP_ERROR_NO_SUCH_DEVICE)) != NULL)
            return TCS_ERROR_INVALID_ARGUMENT;
        if (strstr(errbuf, g_npcap.statustostr(TCS_PCAP_ERROR_IFACE_NOT_UP)) != NULL)
            return TCS_ERROR_NETWORK_UNREACHABLE;
        return TCS_ERROR_SYSTEM;
    }
    // Receiving waits on the handle's event instead, so the socket's timeout and blocking mode can change later
    if (g_npcap.datalink(pcap) != TCS_PCAP_DLT_EN10MB || g_npcap.setnonblock(pcap, 1, errbuf) != 0)
    {
        g_npcap.close(pcap);
        return TCS_ERROR_NOT_SUPPORTED;
    }
    TcsResult filter_sts = tcs_internal_npcap_socket_filter(ps, pcap);
    if (filter_sts != TCS_SUCCESS)
    {
        g_npcap.close(pcap);
        return filter_sts;
    }
    if (ps->pcap != NULL)
        g_npcap.close(ps->pcap);
    ps->pcap = pcap;
    return TCS_SUCCESS;
}

static TcsResult tcs_internal_packet_socket_impl_npcap(TcsSocket* out_socket,
                                                       TcsFamily family,
                                                       TcsSocketType type,
                                                       TcsProtocol protocol)
{
    (void)family; // Always TCS_FAMILY_PACKET
    if (type.native != SOCK_DGRAM && type.native != SOCK_RAW)
        return TCS_ERROR_NOT_SUPPORTED;
    size_t index = 0;
    while (index < TCS_CFG_PACKET_SOCKETS_MAX && g_npcap.sockets[index] != NULL)
        ++index;
    if (index == TCS_CFG_PACKET_SOCKETS_MAX)
        return TCS_ERROR_MEMORY;
    struct TcsInternalNpcapSocket* ps = (struct TcsInternalNpcapSocket*)malloc(sizeof *ps);
    if (ps == NULL)
        return TCS_ERROR_MEMORY;
    memset(ps, 0, sizeof *ps);
    ps->type = type.native;
    ps->protocol = protocol;
    g_npcap.sockets[index] = ps;
    *out_socket = TCS_PACKET_SOCKET_TAG | (TcsSocket)index;
    return TCS_SUCCESS;
}

static TcsResult tcs_internal_packet_close_impl_npcap(TcsSocket* socket)
{
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(*socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (ps->pcap != NULL)
        g_npcap.close(ps->pcap);
    free(ps->groups);
    free(ps);
    g_npcap.sockets[*socket & ~TCS_PACKET_SOCKET_TAG] = NULL;
    *socket = TCS_SOCKET_INVALID;
    return TCS_SUCCESS;
}

static TcsResult tcs_internal_packet_bind_impl_npcap(TcsSocket socket, const struct TcsAddress* address)
{
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (address->family.native != TCS_FAMILY_PACKET.native)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (address->data.packet.interface_id == 0)
        return TCS_ERROR_NOT_SUPPORTED; // An Npcap handle belongs to one adapter, there is no "all interfaces"
    ps->interface_id = address->data.packet.interface_id;
    if (address->data.packet.protocol != 0)
        ps->protocol = address->data.packet.protocol;
    return tcs_internal_npcap_socket_open(ps);
}

static TcsResult tcs_internal_packet_send_impl_npcap(TcsSocket socket,
                                                     const uint8_t* buffer,
                                                     size_t buffer_size,
                                                     uint32_t flags,
                                                     size_t* sent_size)
{
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    // Only whole frames can be sent without a destination
    if (ps->type != SOCK_RAW)
        return TCS_ERROR_INVALID_ARGUMENT;
    struct TcsAddress self = TCS_ADDRESS_NONE;
    self.family = TCS_FAMILY_PACKET;
    self.data.packet.interface_id = ps->interface_id;
    return tcs_send_to(socket, buffer, buffer_size, flags & ~TCS_MSG_SENDALL, &self, sent_size);
}

static TcsResult tcs_internal_packet_send_to_impl_npcap(TcsSocket socket,
                                                        const uint8_t* buffer,
                                                        size_t buffer_size,
                                                        uint32_t flags,
                                                        const struct TcsAddress* destination_address,
                                                        size_t* sent_size)
{
    (void)flags; // No send flag applies to a frame
    if (sent_size != NULL)
        *sent_size = 0;
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (ps->pcap == NULL)
        return TCS_ERROR_INVALID_ARGUMENT; // Not bound
    if (destination_address->family.native != TCS_FAMILY_PACKET.native)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (destination_address->data.packet.interface_id != 0 &&
        destination_address->data.packet.interface_id != ps->interface_id)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (buffer_size > INT_MAX - 14)
        return TCS_ERROR_INVALID_ARGUMENT;

    const uint8_t* frame = buffer;
    size_t frame_size = buffer_size;
    uint8_t* dgram_frame = NULL;
    if (ps->type == SOCK_DGRAM)
    {
        // Ethernet header: destination, source, EtherType. Like Linux, the socket's protocol is the fallback.
        uint16_t protocol =
            destination_address->data.packet.protocol != 0 ? destination_address->data.packet.protocol : ps->protocol;
        frame_size = buffer_size + 14;
        dgram_frame = (uint8_t*)malloc(frame_size);
        if (dgram_frame == NULL)
            return TCS_ERROR_MEMORY;
        memcpy(dgram_frame, destination_address->data.packet.mac, 6);
        memcpy(dgram_frame + 6, ps->mac, 6);
        dgram_frame[12] = (uint8_t)(protocol >> 8);
        dgram_frame[13] = (uint8_t)(protocol & 0xFF);
        if (buffer_size > 0)
            memcpy(dgram_frame + 14, buffer, buffer_size);
        frame = dgram_frame;
    }
    int send_sts = g_npcap.sendpacket(ps->pcap, frame, (int)frame_size);
    free(dgram_frame);
    if (send_sts != 0)
        return TCS_ERROR_SYSTEM;
    if (sent_size != NULL)
        *sent_size = buffer_size;
    return TCS_SUCCESS;
}

static TcsResult tcs_internal_packet_sendv_impl_npcap(TcsSocket socket,
                                                      const struct TcsIoVec* iov,
                                                      size_t iov_length,
                                                      uint32_t flags,
                                                      size_t* sent_size)
{
    // A frame is sent in one piece, so the buffers are gathered first
    size_t total_size = 0;
    for (size_t i = 0; i < iov_length; ++i)
    {
        if (iov[i].buffer == NULL && iov[i].buffer_size > 0)
            return TCS_ERROR_INVALID_ARGUMENT;
        total_size += iov[i].buffer_size;
    }
    uint8_t* gathered = (uint8_t*)malloc(total_size > 0 ? total_size : 1);
    if (gathered == NULL)
        return TCS_ERROR_MEMORY;
    size_t pos = 0;
    for (size_t i = 0; i < iov_length; ++i)
    {
        if (iov[i].buffer_size > 0)
            memcpy(gathered + pos, iov[i].buffer, iov[i].buffer_size);
        pos += iov[i].buffer_size;
    }
    TcsResult send_sts = tcs_send(socket, gathered, total_size, flags, sent_size);
    free(gathered);
    return send_sts;
}

static TcsResult tcs_internal_packet_receive_from_impl_npcap(TcsSocket socket,
                                                             uint8_t* buffer,
                                                             size_t buffer_size,
                                                             uint32_t flags,
                                                             struct TcsAddress* source_address,
                                                             size_t* received_size)
{
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (ps->pcap == NULL)
        return TCS_ERROR_INVALID_ARGUMENT; // Not bound
    if (flags & TCS_MSG_PEEK)
        return TCS_ERROR_NOT_SUPPORTED;

    DWORD start = GetTickCount();
    for (;;)
    {
        struct pcap_pkthdr* header = NULL;
        const unsigned char* frame = NULL;
        int next_sts = g_npcap.next_ex(ps->pcap, &header, &frame);
        if (next_sts < 0)
            return TCS_ERROR_SYSTEM;
        if (next_sts == 1)
        {
            size_t frame_size = header->caplen;
            size_t offset = 0;
            uint16_t protocol = frame_size >= 14 ? (uint16_t)((frame[12] << 8) | frame[13]) : 0;
            if (ps->type == SOCK_DGRAM)
            {
                if (frame_size < 14)
                    continue;
                offset = 14;
                if (protocol == 0x8100 && frame_size >= 18) // 802.1Q tag, stripped like Linux does
                {
                    protocol = (uint16_t)((frame[16] << 8) | frame[17]);
                    offset = 18;
                }
            }
            size_t payload_size = frame_size - offset;
            size_t copy_size = payload_size < buffer_size ? payload_size : buffer_size;
            if (copy_size > 0)
                memcpy(buffer, frame + offset, copy_size);
            if (received_size != NULL)
                *received_size = copy_size;
            if (source_address != NULL)
            {
                *source_address = TCS_ADDRESS_NONE;
                source_address->family = TCS_FAMILY_PACKET;
                source_address->data.packet.interface_id = ps->interface_id;
                source_address->data.packet.protocol = protocol;
                if (frame_size >= 12)
                    memcpy(source_address->data.packet.mac, frame + 6, 6);
            }
            return TCS_SUCCESS;
        }

        // Nothing queued. The event is signaled by the driver when a frame arrives.
        if (ps->nonblocking)
            return TCS_ERROR_WOULD_BLOCK;
        DWORD wait_ms = INFINITE;
        if (ps->receive_timeout_ms > 0)
        {
            DWORD elapsed_ms = GetTickCount() - start;
            if (elapsed_ms >= (DWORD)ps->receive_timeout_ms)
                return TCS_ERROR_TIMED_OUT;
            wait_ms = (DWORD)ps->receive_timeout_ms - elapsed_ms;
        }
        DWORD wait_sts = WaitForSingleObject(g_npcap.getevent(ps->pcap), wait_ms);
        if (wait_sts == WAIT_TIMEOUT)
            return TCS_ERROR_TIMED_OUT;
        if (wait_sts != WAIT_OBJECT_0)
            return TCS_ERROR_SYSTEM;
    }
}

static TcsResult tcs_internal_packet_opt_get_impl_npcap(TcsSocket socket,
                                                        int32_t level,
                                                        int32_t option_name,
                                                        void* out_option_value,
                                                        size_t* option_size)
{
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (level != TCS_SOL_SOCKET || option_name != TCS_SO_TYPE || *option_size < sizeof ps->type)
        return TCS_ERROR_NOT_SUPPORTED;
    memcpy(out_option_value, &ps->type, sizeof ps->type);
    *option_size = sizeof ps->type;
    return TCS_SUCCESS;
}

static TcsResult tcs_internal_packet_opt_receive_timeout_set_impl_npcap(TcsSocket socket, int timeout_ms)
{
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    ps->receive_timeout_ms = timeout_ms;
    return TCS_SUCCESS;
}

static TcsResult tcs_internal_packet_opt_receive_timeout_get_impl_npcap(TcsSocket socket, int* timeout_ms)
{
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    *timeout_ms = ps->receive_timeout_ms;
    return TCS_SUCCESS;
}

static TcsResult tcs_internal_packet_opt_nonblocking_set_impl_npcap(TcsSocket socket, bool do_non_blocking)
{
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    ps->nonblocking = do_non_blocking;
    return TCS_SUCCESS;
}

static TcsResult tcs_internal_packet_opt_nonblocking_get_impl_npcap(TcsSocket socket, bool* is_non_blocking)
{
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    *is_non_blocking = ps->nonblocking;
    return TCS_SUCCESS;
}

static TcsResult tcs_internal_packet_opt_membership_add_to_impl_npcap(TcsSocket socket,
                                                                      const struct TcsAddress* local_address,
                                                                      const struct TcsAddress* multicast_address)
{
    (void)local_address; // A packet socket is bound to one interface
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (multicast_address->family.native != TCS_FAMILY_PACKET.native)
        return TCS_ERROR_INVALID_ARGUMENT;
    uint8_t* groups = (uint8_t*)realloc(ps->groups, (ps->group_count + 1) * 6);
    if (groups == NULL)
        return TCS_ERROR_MEMORY;
    memcpy(groups + ps->group_count * 6, multicast_address->data.packet.mac, 6);
    ps->groups = groups;
    ps->group_count++;
    if (ps->pcap == NULL)
        return TCS_SUCCESS; // Applied by tcs_bind()
    // Promiscuous mode is fixed when a handle is opened, so the first group reopens it. Queued frames are lost.
    TcsResult sts =
        ps->group_count == 1 ? tcs_internal_npcap_socket_open(ps) : tcs_internal_npcap_socket_filter(ps, ps->pcap);
    if (sts != TCS_SUCCESS)
        ps->group_count--;
    return sts;
}

static TcsResult tcs_internal_packet_opt_membership_drop_from_impl_npcap(TcsSocket socket,
                                                                         const struct TcsAddress* local_address,
                                                                         const struct TcsAddress* multicast_address)
{
    (void)local_address; // A packet socket is bound to one interface
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (multicast_address->family.native != TCS_FAMILY_PACKET.native)
        return TCS_ERROR_INVALID_ARGUMENT;
    size_t i = 0;
    while (i < ps->group_count && memcmp(ps->groups + i * 6, multicast_address->data.packet.mac, 6) != 0)
        ++i;
    if (i == ps->group_count)
        return TCS_ERROR_INVALID_ARGUMENT;
    // The group is swapped to the end, so a failed update is undone by restoring the count
    size_t last = ps->group_count - 1;
    if (i != last)
    {
        uint8_t dropped[6];
        memcpy(dropped, ps->groups + i * 6, 6);
        memcpy(ps->groups + i * 6, ps->groups + last * 6, 6);
        memcpy(ps->groups + last * 6, dropped, 6);
    }
    ps->group_count--;
    if (ps->pcap == NULL)
        return TCS_SUCCESS;
    TcsResult sts =
        ps->group_count == 0 ? tcs_internal_npcap_socket_open(ps) : tcs_internal_npcap_socket_filter(ps, ps->pcap);
    if (sts != TCS_SUCCESS)
        ps->group_count++;
    return sts;
}

static TcsResult tcs_internal_packet_address_socket_local_impl_npcap(TcsSocket socket, struct TcsAddress* local_address)
{
    struct TcsInternalNpcapSocket* ps = tcs_internal_npcap_socket_get(socket);
    if (ps == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    *local_address = TCS_ADDRESS_NONE;
    local_address->family = TCS_FAMILY_PACKET;
    local_address->data.packet.interface_id = ps->interface_id;
    local_address->data.packet.protocol = ps->protocol;
    memcpy(local_address->data.packet.mac, ps->mac, 6);
    return TCS_SUCCESS;
}

static bool tcs_internal_packet_address_is_supported_impl_npcap(const struct TcsAddress* addr)
{
    return addr->family.native == TCS_FAMILY_PACKET.native;
}

static void tcs_internal_lib_init_impl_npcap(void)
{
    if (g_npcap.init_count++ != 0)
        return;
    // Wine ships its own wpcap.dll where pcap_getevent() is a stub that raises an exception, so packet sockets stay
    // unsupported there. Checking ntdll for wine_get_version() is the detection Wine's developer FAQ recommends.
    if (GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version") != NULL)
        return;
    // Npcap installs to System32\Npcap. LOAD_WITH_ALTERED_SEARCH_PATH makes the loader find Packet.dll next to
    // wpcap.dll without SetDllDirectory(), which would affect the whole process. The plain name is the fallback
    // recommended by Npcap, for installations in WinPcap compatible mode.
    const char suffix[] = "\\Npcap\\wpcap.dll";
    char path[MAX_PATH];
    UINT system_dir_size = GetSystemDirectoryA(path, MAX_PATH);
    HMODULE m = NULL;
    if (system_dir_size != 0 && system_dir_size + sizeof suffix <= MAX_PATH)
    {
        memcpy(path + system_dir_size, suffix, sizeof suffix);
        m = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }
    if (m == NULL)
        m = LoadLibraryA("wpcap.dll");
    if (m == NULL)
        return;

    // FARPROC is cast through void (*)(void), which gcc treats as compatible with every function pointer type
    g_npcap.open = (TcsInternalPcapOpen)(void (*)(void))GetProcAddress(m, "pcap_open");
    g_npcap.statustostr = (TcsInternalPcapStatusToStr)(void (*)(void))GetProcAddress(m, "pcap_statustostr");
    g_npcap.datalink = (TcsInternalPcapGetInt)(void (*)(void))GetProcAddress(m, "pcap_datalink");
    g_npcap.setnonblock = (TcsInternalPcapSetNonblock)(void (*)(void))GetProcAddress(m, "pcap_setnonblock");
    g_npcap.getevent = (TcsInternalPcapGetEvent)(void (*)(void))GetProcAddress(m, "pcap_getevent");
    g_npcap.compile = (TcsInternalPcapCompile)(void (*)(void))GetProcAddress(m, "pcap_compile");
    g_npcap.setfilter = (TcsInternalPcapSetFilter)(void (*)(void))GetProcAddress(m, "pcap_setfilter");
    g_npcap.freecode = (TcsInternalPcapFreeCode)(void (*)(void))GetProcAddress(m, "pcap_freecode");
    g_npcap.next_ex = (TcsInternalPcapNextEx)(void (*)(void))GetProcAddress(m, "pcap_next_ex");
    g_npcap.sendpacket = (TcsInternalPcapSendPacket)(void (*)(void))GetProcAddress(m, "pcap_sendpacket");
    g_npcap.close = (TcsInternalPcapClose)(void (*)(void))GetProcAddress(m, "pcap_close");

    if (g_npcap.open == NULL || g_npcap.statustostr == NULL || g_npcap.datalink == NULL ||
        g_npcap.setnonblock == NULL || g_npcap.getevent == NULL || g_npcap.compile == NULL ||
        g_npcap.setfilter == NULL || g_npcap.freecode == NULL || g_npcap.next_ex == NULL ||
        g_npcap.sendpacket == NULL || g_npcap.close == NULL)
    {
        FreeLibrary(m);
        return;
    }
    g_npcap.module = m;
    tcs_internal_packet_socket_impl = tcs_internal_packet_socket_impl_npcap;
    tcs_internal_packet_close_impl = tcs_internal_packet_close_impl_npcap;
    tcs_internal_packet_bind_impl = tcs_internal_packet_bind_impl_npcap;
    tcs_internal_packet_send_impl = tcs_internal_packet_send_impl_npcap;
    tcs_internal_packet_send_to_impl = tcs_internal_packet_send_to_impl_npcap;
    tcs_internal_packet_sendv_impl = tcs_internal_packet_sendv_impl_npcap;
    tcs_internal_packet_receive_from_impl = tcs_internal_packet_receive_from_impl_npcap;
    tcs_internal_packet_opt_get_impl = tcs_internal_packet_opt_get_impl_npcap;
    tcs_internal_packet_opt_receive_timeout_set_impl = tcs_internal_packet_opt_receive_timeout_set_impl_npcap;
    tcs_internal_packet_opt_receive_timeout_get_impl = tcs_internal_packet_opt_receive_timeout_get_impl_npcap;
    tcs_internal_packet_opt_nonblocking_set_impl = tcs_internal_packet_opt_nonblocking_set_impl_npcap;
    tcs_internal_packet_opt_nonblocking_get_impl = tcs_internal_packet_opt_nonblocking_get_impl_npcap;
    tcs_internal_packet_opt_membership_add_to_impl = tcs_internal_packet_opt_membership_add_to_impl_npcap;
    tcs_internal_packet_opt_membership_drop_from_impl = tcs_internal_packet_opt_membership_drop_from_impl_npcap;
    tcs_internal_packet_address_socket_local_impl = tcs_internal_packet_address_socket_local_impl_npcap;
    tcs_internal_packet_address_is_supported_impl = tcs_internal_packet_address_is_supported_impl_npcap;
}

static void tcs_internal_lib_cleanup_impl_npcap(void)
{
    if (g_npcap.init_count == 0 || --g_npcap.init_count != 0 || g_npcap.module == NULL)
        return;
    // Like WSACleanup(), the last cleanup closes every socket still open
    for (size_t i = 0; i < TCS_CFG_PACKET_SOCKETS_MAX; ++i)
    {
        TcsSocket socket = TCS_PACKET_SOCKET_TAG | (TcsSocket)i;
        if (g_npcap.sockets[i] != NULL)
            tcs_close(&socket);
    }
    tcs_internal_packet_socket_impl = tcs_internal_packet_socket_impl_not_supported;
    tcs_internal_packet_close_impl = tcs_internal_packet_close_impl_not_supported;
    tcs_internal_packet_bind_impl = tcs_internal_packet_bind_impl_not_supported;
    tcs_internal_packet_send_impl = tcs_internal_packet_send_impl_not_supported;
    tcs_internal_packet_send_to_impl = tcs_internal_packet_send_to_impl_not_supported;
    tcs_internal_packet_sendv_impl = tcs_internal_packet_sendv_impl_not_supported;
    tcs_internal_packet_receive_from_impl = tcs_internal_packet_receive_from_impl_not_supported;
    tcs_internal_packet_opt_get_impl = tcs_internal_packet_opt_get_impl_not_supported;
    tcs_internal_packet_opt_receive_timeout_set_impl = tcs_internal_packet_opt_receive_timeout_set_impl_not_supported;
    tcs_internal_packet_opt_receive_timeout_get_impl = tcs_internal_packet_opt_receive_timeout_get_impl_not_supported;
    tcs_internal_packet_opt_nonblocking_set_impl = tcs_internal_packet_opt_nonblocking_set_impl_not_supported;
    tcs_internal_packet_opt_nonblocking_get_impl = tcs_internal_packet_opt_nonblocking_get_impl_not_supported;
    tcs_internal_packet_opt_membership_add_to_impl = tcs_internal_packet_opt_membership_add_to_impl_not_supported;
    tcs_internal_packet_opt_membership_drop_from_impl = tcs_internal_packet_opt_membership_drop_from_impl_not_supported;
    tcs_internal_packet_address_socket_local_impl = tcs_internal_packet_address_socket_local_impl_not_supported;
    tcs_internal_packet_address_is_supported_impl = tcs_internal_packet_address_is_supported_impl_not_supported;
    FreeLibrary(g_npcap.module);
    memset(&g_npcap, 0, sizeof g_npcap);
}

TcsResult tcs_lib_init(void)
{
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        return TCS_ERROR_SYSTEM;
    tcs_internal_lib_init_impl_npcap(); // Npcap is optional, without it packet sockets are not supported
    return TCS_SUCCESS;
}

TcsResult tcs_lib_cleanup(void)
{
    tcs_internal_lib_cleanup_impl_npcap();
    WSACleanup();
    return TCS_SUCCESS;
}

TcsResult tcs_socket(TcsSocket* out_socket, TcsFamily family, TcsSocketType type, TcsProtocol protocol)
{
    if (out_socket == NULL || *out_socket != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (family.native == TCS_FAMILY_PACKET.native)
        return tcs_internal_packet_socket_impl(out_socket, family, type, protocol);

    TcsSocket new_socket = socket(family.native, type.native, (int)protocol);

    if (new_socket != INVALID_SOCKET)
    {
        *out_socket = new_socket;
        return TCS_SUCCESS;
    }
    else
    {
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }
}

// tcs_socket_tcp() is defined in tinycsocket_common.c
// tcs_socket_tcp_str() is defined in tinycsocket_common.c
// tcs_socket_udp() is defined in tinycsocket_common.c
// tcs_socket_udp_str() is defined in tinycsocket_common.c
// tcs_socket_packet() is defined in tinycsocket_common.c
// tcs_socket_packet_str() is defined in tinycsocket_common.c

TcsResult tcs_close(TcsSocket* socket)
{
    if (socket == NULL || *socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(*socket))
        return tcs_internal_packet_close_impl(socket);

    int close_status = closesocket(*socket);
    if (close_status != SOCKET_ERROR)
    {
        *socket = TCS_SOCKET_INVALID;
        return TCS_SUCCESS;
    }
    else
    {
        return socketstatus2retcode(close_status);
    }
}

// ######## Socket Operations ########

TcsResult tcs_bind(TcsSocket socket, const struct TcsAddress* address)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_bind_impl(socket, address);

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = 0;
    TcsResult convert_addr_status = sockaddr2native(address, (PSOCKADDR)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;
    int bind_status = bind(socket, (PSOCKADDR)&native_sockaddr, addrlen);
    return socketstatus2retcode(bind_status);
}

TcsResult tcs_connect(TcsSocket socket, const struct TcsAddress* address)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return TCS_ERROR_NOT_SUPPORTED;

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = 0;
    TcsResult convert_addr_status = sockaddr2native(address, (PSOCKADDR)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;
    int connect_status = connect(socket, (PSOCKADDR)&native_sockaddr, addrlen);
    if (connect_status == SOCKET_ERROR)
    {
        int error_code = WSAGetLastError();
        // Windows uses WSAEWOULDBLOCK for non-blocking connect in progress,
        // while POSIX uses EINPROGRESS. Normalize to TCS_IN_PROGRESS.
        if (error_code == WSAEWOULDBLOCK)
            return TCS_IN_PROGRESS;
        return wsaerror2retcode(error_code);
    }
    return TCS_SUCCESS;
}

// tcs_connect_str() is defined in tinycsocket_common.c

TcsResult tcs_listen(TcsSocket socket, int backlog)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (tcs_internal_is_socket_packet(socket))
        return TCS_ERROR_NOT_SUPPORTED;

    int status = listen(socket, (int)backlog);
    return socketstatus2retcode(status);
}

TcsResult tcs_accept(TcsSocket listener, TcsSocket* out_socket, struct TcsAddress* out_address)
{
    if (listener == TCS_SOCKET_INVALID || out_socket == NULL || *out_socket != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (tcs_internal_is_socket_packet(listener))
        return TCS_ERROR_NOT_SUPPORTED;

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addr_len = sizeof(native_sockaddr);
    *out_socket = accept(listener, (PSOCKADDR)&native_sockaddr, &addr_len);
    if (*out_socket != INVALID_SOCKET)
    {
        if (out_address != NULL)
        {
            TcsResult convert_addr_status = native2sockaddr((PSOCKADDR)&native_sockaddr, out_address);
            if (convert_addr_status != TCS_SUCCESS)
                return convert_addr_status;
        }
        return TCS_SUCCESS;
    }
    else
    {
        *out_socket = TCS_SOCKET_INVALID;
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }
}

TcsResult tcs_shutdown(TcsSocket socket, TcsShutdownDirection direction)
{
    const int LUT[] = {SD_RECEIVE, SD_SEND, SD_BOTH};

    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (direction < 0 || direction > 2)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (tcs_internal_is_socket_packet(socket))
        return TCS_ERROR_NOT_SUPPORTED;

    const int how = LUT[direction];
    int shutdown_status = shutdown(socket, how);
    return socketstatus2retcode(shutdown_status);
}

// ######## Data Transfer ########

TcsResult tcs_send(TcsSocket socket, const uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* sent_size)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (sent_size != NULL)
        *sent_size = 0;

    if (buffer == NULL || buffer_size == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_send_impl(socket, buffer, buffer_size, flags, sent_size);

    // Send all
    if (flags & TCS_MSG_SENDALL)
    {
        uint32_t new_flags = flags & ~TCS_MSG_SENDALL; // For recursive call
        size_t left = buffer_size;
        const uint8_t* iterator = buffer;

        while (left > 0)
        {
            size_t sent = 0;
            TcsResult sts = tcs_send(socket, iterator, left, new_flags, &sent);
            if (sent_size != NULL)
                *sent_size += sent;
            if (sts != TCS_SUCCESS)
                return sts;
            left -= sent;
            iterator += sent;
        }
        return TCS_SUCCESS;
    }
    else // Send
    {
        int send_status = send(socket, (const char*)buffer, (int)buffer_size, (int)flags);
        if (send_status != SOCKET_ERROR)
        {
            if (sent_size != NULL)
                *sent_size = (size_t)send_status;
            return TCS_SUCCESS;
        }
        else
        {
            if (sent_size != NULL)
                *sent_size = 0;

            return socketstatus2retcode(send_status);
        }
    }
}

TcsResult tcs_send_to(TcsSocket socket,
                      const uint8_t* buffer,
                      size_t buffer_size,
                      uint32_t flags,
                      const struct TcsAddress* destination_address,
                      size_t* sent_size)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (buffer == NULL && buffer_size > 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (destination_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (flags & TCS_MSG_SENDALL)
        return TCS_ERROR_NOT_IMPLEMENTED;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_send_to_impl(socket, buffer, buffer_size, flags, destination_address, sent_size);

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = 0;
    TcsResult convert_addr_status = sockaddr2native(destination_address, (PSOCKADDR)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;

    int sendto_status =
        sendto(socket, (const char*)buffer, (int)buffer_size, (int)flags, (PSOCKADDR)&native_sockaddr, addrlen);

    if (sendto_status != SOCKET_ERROR)
    {
        if (sent_size != NULL)
            *sent_size = (size_t)sendto_status;
        return TCS_SUCCESS;
    }
    else
    {
        if (sent_size != NULL)
            *sent_size = 0;

        return socketstatus2retcode(sendto_status);
    }
}

TcsResult tcs_sendv(TcsSocket socket, const struct TcsIoVec* iov, size_t iov_length, uint32_t flags, size_t* sent_size)
{
    if (socket == TCS_SOCKET_INVALID || iov == NULL || iov_length == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (flags & TCS_MSG_SENDALL)
        return TCS_ERROR_NOT_IMPLEMENTED;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_sendv_impl(socket, iov, iov_length, flags, sent_size);

    WSABUF stack_buffers[TCS_CFG_SENDV_STACK_MAX];
    WSABUF* native_buffers = stack_buffers;
    WSABUF* heap_buffers = NULL;

    if (iov_length > TCS_CFG_SENDV_STACK_MAX)
    {
        heap_buffers = (WSABUF*)malloc(sizeof(WSABUF) * iov_length);
        if (heap_buffers == NULL)
            return TCS_ERROR_MEMORY;
        native_buffers = heap_buffers;
    }

    for (size_t i = 0; i < iov_length; ++i)
    {
        if (iov[i].buffer == NULL && iov[i].buffer_size > 0)
        {
            free(heap_buffers);
            return TCS_ERROR_INVALID_ARGUMENT;
        }
        // WSABUF.buf is non-const by Windows API design, but WSASend does not modify the data.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
        native_buffers[i].buf = (CHAR*)iov[i].buffer;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
        native_buffers[i].len = (ULONG)iov[i].buffer_size;
    }

    DWORD sent = 0;
    int wsasend_status = WSASend(socket, native_buffers, (DWORD)iov_length, &sent, (DWORD)flags, NULL, NULL);

    free(heap_buffers);

    if (wsasend_status != SOCKET_ERROR)
    {
        if (sent_size != NULL)
            *sent_size = (size_t)sent;
        return TCS_SUCCESS;
    }
    else
    {
        if (sent_size != NULL)
            *sent_size = 0;
        return socketstatus2retcode(wsasend_status);
    }
}

TcsResult tcs_receive(TcsSocket socket, uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* received_size)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (buffer == NULL && buffer_size > 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_receive_from(socket, buffer, buffer_size, flags, NULL, received_size);

#if WINVER <= 0x501
    if (flags & TCS_MSG_WAITALL)
    {
        uint32_t new_flags = flags & ~TCS_MSG_WAITALL; // Unmask for recursive call
        size_t received_so_far = 0;
        while (received_so_far < buffer_size)
        {
            size_t received_now = 0;
            uint8_t* cursor = buffer + received_so_far;
            size_t left = buffer_size - received_so_far;
            TcsResult sts = tcs_receive(socket, cursor, left, new_flags, &received_now);
            if (sts != TCS_SUCCESS)
            {
                if (received_size != NULL)
                    *received_size = received_so_far;
                return sts;
            }
            received_so_far += received_now;
        }
        if (received_size != NULL)
            *received_size = received_so_far;
        return TCS_SUCCESS;
    }
#endif

    int recv_status = recv(socket, (char*)buffer, (int)buffer_size, (int)flags);

    if (recv_status == 0)
    {
        if (received_size != NULL)
            *received_size = 0;
        TcsSocketType sock_type = {0};
        if (tcs_opt_type_get(socket, &sock_type) == TCS_SUCCESS && sock_type.native == TCS_SOCKET_STREAM.native)
            return TCS_SHUTDOWN;
        return TCS_SUCCESS;
    }
    else if (recv_status != SOCKET_ERROR)
    {
        if (received_size != NULL)
            *received_size = (size_t)recv_status;
        return TCS_SUCCESS;
    }
    else
    {
        if (received_size != NULL)
            *received_size = 0;

        return socketstatus2retcode(recv_status);
    }
}

TcsResult tcs_receive_from(TcsSocket socket,
                           uint8_t* buffer,
                           size_t buffer_size,
                           uint32_t flags,
                           struct TcsAddress* source_address,
                           size_t* received_size)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (buffer == NULL && buffer_size > 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (received_size != NULL)
        *received_size = 0;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_receive_from_impl(socket, buffer, buffer_size, flags, source_address, received_size);

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = sizeof(native_sockaddr);

    int recvfrom_status =
        recvfrom(socket, (char*)buffer, (int)buffer_size, (int)flags, (PSOCKADDR)&native_sockaddr, &addrlen);

    if (recvfrom_status == 0)
    {
        if (received_size != NULL)
            *received_size = 0;
        TcsSocketType sock_type = {0};
        if (tcs_opt_type_get(socket, &sock_type) == TCS_SUCCESS && sock_type.native == TCS_SOCKET_STREAM.native)
            return TCS_SHUTDOWN;
        return TCS_SUCCESS;
    }
    else if (recvfrom_status != SOCKET_ERROR)
    {
        if (received_size != NULL)
            *received_size = (size_t)recvfrom_status;

        if (source_address != NULL)
        {
            TcsResult convert_address_status = native2sockaddr((PSOCKADDR)&native_sockaddr, source_address);
            return convert_address_status;
        }

        return TCS_SUCCESS;
    }
    else
    {
        if (received_size != NULL)
            *received_size = 0;

        return socketstatus2retcode(recvfrom_status);
    }
}

// tcs_receive_line() is defined in tinycsocket_common.c
// tcs_receive_netstring() is defined in tinycsocket_common.c

// ######## Socket Polling ########

TcsResult tcs_poll_create(struct TcsPoll** out_poll)
{
    if (out_poll == NULL || *out_poll != NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    *out_poll = (struct TcsPoll*)malloc(sizeof(struct TcsPoll));
    if (*out_poll == NULL)
        return TCS_ERROR_MEMORY;
    memset(*out_poll, 0, sizeof(struct TcsPoll));
    int sts_read_array = tds_ulist_soc_create(&(*out_poll)->read_sockets);
    int sts_write_array = tds_ulist_soc_create(&(*out_poll)->write_sockets);
    int sts_error_array = tds_ulist_soc_create(&(*out_poll)->error_sockets);
    int sts_user_data = tds_map_socket_user_create(&(*out_poll)->user_data);

    if (sts_read_array != 0 || sts_write_array != 0 || sts_error_array != 0 || sts_user_data != 0)
    {
        tcs_poll_destroy(out_poll);
        return TCS_ERROR_MEMORY;
    }
    return TCS_SUCCESS;
}

TcsResult tcs_poll_destroy(struct TcsPoll** poll)
{
    if (poll == NULL || *poll == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    tds_ulist_soc_destroy(&(*poll)->read_sockets);
    tds_ulist_soc_destroy(&(*poll)->write_sockets);
    tds_ulist_soc_destroy(&(*poll)->error_sockets);
    tds_map_socket_user_destroy(&(*poll)->user_data);

    free(*poll);
    *poll = NULL;

    return TCS_SUCCESS;
}

TcsResult tcs_poll_add(struct TcsPoll* poll, TcsSocket socket, void* user_data, uint32_t flags)
{
    if (poll == NULL || socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (tcs_internal_is_socket_packet(socket))
        return TCS_ERROR_NOT_SUPPORTED; // select() cannot wait on an Npcap handle

    int map_sts = tds_map_socket_user_add(&poll->user_data, socket, user_data);
    if (map_sts != 0)
        return TCS_ERROR_MEMORY;

    // Always monitor for errors
    {
        int sts = tds_ulist_soc_add(&poll->error_sockets, &socket, 1);
        if (sts != 0)
        {
            tds_map_socket_user_remove(&poll->user_data, poll->user_data.count - 1);
            return TCS_ERROR_MEMORY;
        }
    }
    if (flags & TCS_POLL_READ)
    {
        int sts = tds_ulist_soc_add(&poll->read_sockets, &socket, 1);
        if (sts != 0)
        {
            tds_ulist_soc_remove(&poll->error_sockets, poll->error_sockets.count - 1, 1);
            tds_map_socket_user_remove(&poll->user_data, poll->user_data.count - 1);
            return TCS_ERROR_MEMORY;
        }
    }
    if (flags & TCS_POLL_WRITE)
    {
        int sts = tds_ulist_soc_add(&poll->write_sockets, &socket, 1);
        if (sts != 0)
        {
            if (flags & TCS_POLL_READ)
                tds_ulist_soc_remove(&poll->read_sockets, poll->read_sockets.count - 1, 1);
            tds_ulist_soc_remove(&poll->error_sockets, poll->error_sockets.count - 1, 1);
            tds_map_socket_user_remove(&poll->user_data, poll->user_data.count - 1);
            return TCS_ERROR_MEMORY;
        }
    }
    return TCS_SUCCESS;
}

TcsResult tcs_poll_modify(struct TcsPoll* poll, TcsSocket socket, uint32_t flags)
{
    if (poll == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    bool found = false;
    for (size_t i = 0; i < poll->user_data.count; ++i)
    {
        if (poll->user_data.keys[i] == socket)
        {
            found = true;
            break;
        }
    }
    if (!found)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TdsUList_soc* const lists[2] = {&poll->read_sockets, &poll->write_sockets};
    const bool should_exist[2] = {(flags & TCS_POLL_READ) != 0, (flags & TCS_POLL_WRITE) != 0};
    for (int k = 0; k < 2; ++k)
    {
        struct TdsUList_soc* list = lists[k];
        bool already_in_list = false;
        for (size_t i = 0; i < list->count; ++i)
        {
            if (list->data[i] == socket)
            {
                if (!should_exist[k])
                    tds_ulist_soc_remove(list, i, 1);
                already_in_list = true;
                break;
            }
        }
        if (!already_in_list && should_exist[k])
            tds_ulist_soc_add(list, &socket, 1);
    }

    return TCS_SUCCESS;
}

TcsResult tcs_poll_remove(struct TcsPoll* poll, TcsSocket socket)
{
    if (poll == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    bool found = false;
    for (size_t i = 0; i < poll->read_sockets.count; ++i)
    {
        if (poll->read_sockets.data[i] == socket)
        {
            tds_ulist_soc_remove(&poll->read_sockets, i, 1);
            found = true;
            break;
        }
    }
    for (size_t i = 0; i < poll->write_sockets.count; ++i)
    {
        if (poll->write_sockets.data[i] == socket)
        {
            tds_ulist_soc_remove(&poll->write_sockets, i, 1);
            found = true;
            break;
        }
    }
    for (size_t i = 0; i < poll->error_sockets.count; ++i)
    {
        if (poll->error_sockets.data[i] == socket)
        {
            tds_ulist_soc_remove(&poll->error_sockets, i, 1);
            found = true;
            break;
        }
    }
    for (size_t i = 0; i < poll->user_data.count; ++i)
    {
        if (poll->user_data.keys[i] == socket)
        {
            tds_map_socket_user_remove(&poll->user_data, i);
            found = true;
            break;
        }
    }

    if (!found)
        return TCS_ERROR_INVALID_ARGUMENT;

    return TCS_SUCCESS;
}

TcsResult tcs_poll_wait(struct TcsPoll* poll,
                        struct TcsPollEvent* out_events,
                        size_t events_length,
                        size_t* out_events_length,
                        int timeout_ms)
{
    if (poll == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (out_events == NULL || out_events_length == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (timeout_ms < 0 && timeout_ms != TCS_WAIT_INF)
        return TCS_ERROR_INVALID_ARGUMENT;

    // Todo: add more modern implementation. Maybe dispatch att init?
    // SELECT IMPLEMENTATION

    // Copy fds, use auto storage if possible, otherwise use dynamic memory

    // Allocate so that we can access fd_array out of nominal bounds
    // We need this hack to be able to use dynamic memory for select
    const size_t data_offset = offsetof(struct tcs_fd_set, fd_array);

    struct fd_set rfds_stack;
    struct fd_set wfds_stack;
    struct fd_set efds_stack;

    struct tcs_fd_set* rfds_heap = NULL;
    struct tcs_fd_set* wfds_heap = NULL;
    struct tcs_fd_set* efds_heap = NULL;

    struct tcs_fd_set* rfds_cpy = NULL;
    struct tcs_fd_set* wfds_cpy = NULL;
    struct tcs_fd_set* efds_cpy = NULL;

    if (poll->read_sockets.count <= FD_SETSIZE)
    {
        FD_ZERO(&rfds_stack);
        rfds_cpy = (struct tcs_fd_set*)&rfds_stack;
    }
    else
    {
        rfds_heap = (struct tcs_fd_set*)malloc(data_offset + sizeof(SOCKET) * poll->read_sockets.count);
        if (rfds_heap == NULL)
            return TCS_ERROR_MEMORY;
        rfds_cpy = rfds_heap;
    }

    if (poll->write_sockets.count <= FD_SETSIZE)
    {
        FD_ZERO(&wfds_stack);
        wfds_cpy = (struct tcs_fd_set*)&wfds_stack;
    }
    else
    {
        wfds_heap = (struct tcs_fd_set*)malloc(data_offset + sizeof(SOCKET) * poll->write_sockets.count);
        if (wfds_heap == NULL)
        {
            free(rfds_heap);
            return TCS_ERROR_MEMORY;
        }
        wfds_cpy = wfds_heap;
    }

    if (poll->error_sockets.count <= FD_SETSIZE)
    {
        FD_ZERO(&efds_stack);
        efds_cpy = (struct tcs_fd_set*)&efds_stack;
    }
    else
    {
        efds_heap = (struct tcs_fd_set*)malloc(data_offset + sizeof(SOCKET) * poll->error_sockets.count);
        if (efds_heap == NULL)
        {
            free(rfds_heap);
            free(wfds_heap);
            return TCS_ERROR_MEMORY;
        }
        efds_cpy = efds_heap;
    }
    rfds_cpy->fd_count = (u_int)poll->read_sockets.count;
    wfds_cpy->fd_count = (u_int)poll->write_sockets.count;
    efds_cpy->fd_count = (u_int)poll->error_sockets.count;

    memcpy(rfds_cpy->fd_array, poll->read_sockets.data, sizeof(SOCKET) * poll->read_sockets.count);
    memcpy(wfds_cpy->fd_array, poll->write_sockets.data, sizeof(SOCKET) * poll->write_sockets.count);
    memcpy(efds_cpy->fd_array, poll->error_sockets.data, sizeof(SOCKET) * poll->error_sockets.count);

    memset(out_events, 0, sizeof(struct TcsPollEvent) * events_length);
    *out_events_length = 0;

    // Run select
    struct timeval* t_ptr = NULL;
    struct timeval t = {0, 0};
    if (timeout_ms != TCS_WAIT_INF)
    {
        t.tv_sec = (long)(timeout_ms / 1000);
        t.tv_usec = (long)(timeout_ms % 1000) * 1000;
        t_ptr = &t;
    }
    int no = select(IGNORE, (fd_set*)rfds_cpy, (fd_set*)wfds_cpy, (fd_set*)efds_cpy, t_ptr);

    size_t events_added = 0;

    if (no > 0)
    {
        for (u_int n = 0; n < rfds_cpy->fd_count && events_added < events_length; ++n)
        {
            out_events[events_added].socket = rfds_cpy->fd_array[n];
            out_events[events_added].can_read = true;
            for (size_t i = 0; i < poll->user_data.count; ++i)
            {
                if (out_events[events_added].socket == poll->user_data.keys[i])
                {
                    out_events[events_added].user_data = poll->user_data.values[i];
                    break;
                }
            }
            events_added++;
        }
        for (u_int n = 0; n < wfds_cpy->fd_count && events_added < events_length; ++n)
        {
            // Check already added events
            size_t new_n = events_added;
            for (size_t m = 0; m < events_added; ++m)
            {
                if (out_events[m].socket == wfds_cpy->fd_array[n])
                {
                    new_n = m;
                    break;
                }
            }
            out_events[new_n].can_write = true;
            // Check for new events
            if (events_added == new_n)
            {
                out_events[new_n].socket = wfds_cpy->fd_array[n];

                for (size_t i = 0; i < poll->user_data.count; ++i)
                {
                    if (out_events[new_n].socket == poll->user_data.keys[i])
                    {
                        out_events[new_n].user_data = poll->user_data.values[i];
                        break;
                    }
                }
                events_added++;
            }
        }
        for (u_int n = 0; n < efds_cpy->fd_count && events_added < events_length; ++n)
        {
            // Check already added events
            size_t new_n = events_added;
            for (size_t m = 0; m < events_added; ++m)
            {
                if (out_events[m].socket == efds_cpy->fd_array[n])
                {
                    new_n = m;
                    break;
                }
            }
            // Get the actual socket error
            int so_error = 0;
            int so_error_size = sizeof(so_error);
            if (getsockopt((SOCKET)efds_cpy->fd_array[n], SOL_SOCKET, SO_ERROR, (char*)&so_error, &so_error_size) != 0)
                out_events[new_n].error = wsaerror2retcode(WSAGetLastError());
            else
                out_events[new_n].error = so_error != 0 ? wsaerror2retcode(so_error) : TCS_ERROR_UNKNOWN;
            if (events_added == new_n)
            {
                out_events[new_n].socket = efds_cpy->fd_array[n];

                for (size_t i = 0; i < poll->user_data.count; ++i)
                {
                    if (out_events[new_n].socket == poll->user_data.keys[i])
                    {
                        out_events[new_n].user_data = poll->user_data.values[i];
                        break;
                    }
                }

                events_added++;
            }
        }
    }

    *out_events_length = events_added;

    // Clean up
    if (rfds_heap != NULL)
        free(rfds_heap);
    if (wfds_heap != NULL)
        free(wfds_heap);
    if (efds_heap != NULL)
        free(efds_heap);

    if (no == 0)
    {
        return TCS_ERROR_TIMED_OUT;
    }
    if (no == SOCKET_ERROR)
    {
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }

    return TCS_SUCCESS;
}

// ######## Socket Options ########

TcsResult tcs_opt_set(TcsSocket socket,
                      int32_t level,
                      int32_t option_name,
                      const void* option_value,
                      size_t option_size)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (option_name == -1)
        return TCS_ERROR_NOT_IMPLEMENTED;
    if (tcs_internal_is_socket_packet(socket))
        return TCS_ERROR_NOT_SUPPORTED;

    int sockopt_status = setsockopt(socket, (int)level, (int)option_name, (const char*)option_value, (int)option_size);
    return socketstatus2retcode(sockopt_status);
}

TcsResult tcs_opt_get(TcsSocket socket, int32_t level, int32_t option_name, void* out_option_value, size_t* option_size)
{
    if (socket == TCS_SOCKET_INVALID || out_option_value == NULL || option_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (option_name == -1)
        return TCS_ERROR_NOT_IMPLEMENTED;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_opt_get_impl(socket, level, option_name, out_option_value, option_size);

    int optlen = (int)*option_size;
    int sockopt_status = getsockopt(socket, (int)level, (int)option_name, (char*)out_option_value, &optlen);
    if (sockopt_status == 0)
        *option_size = (size_t)optlen;
    return socketstatus2retcode(sockopt_status);
}

// tcs_opt_broadcast_set() is defined in tinycsocket_common.c
// tcs_opt_broadcast_get() is defined in tinycsocket_common.c
// tcs_opt_keep_alive_set() is defined in tinycsocket_common.c
// tcs_opt_keep_alive_get() is defined in tinycsocket_common.c
TcsResult tcs_opt_reuse_address_set(TcsSocket socket, bool do_allow_reuse_address)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = do_allow_reuse_address ? 1 : 0;
    TcsResult sts = tcs_opt_set(socket, TCS_SOL_SOCKET, TCS_SO_REUSEADDR, &b, sizeof(b));
    if (sts != TCS_SUCCESS)
        return sts;

#if _WIN32_WINNT >= 0x0502
    TcsSocketType sock_type = {0};
    if (tcs_opt_type_get(socket, &sock_type) == TCS_SUCCESS && sock_type.native == TCS_SOCKET_STREAM.native)
        tcs_opt_set(socket, TCS_SOL_SOCKET, SO_EXCLUSIVEADDRUSE, &b, sizeof(b));
#endif

    return TCS_SUCCESS;
}

TcsResult tcs_opt_reuse_address_get(TcsSocket socket, bool* is_reuse_address_allowed)
{
    if (socket == TCS_SOCKET_INVALID || is_reuse_address_allowed == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsResult sts = tcs_opt_get(socket, TCS_SOL_SOCKET, TCS_SO_REUSEADDR, &b, &s);
    *is_reuse_address_allowed = b;
    return sts;
}

TcsResult tcs_opt_reuse_port_set(TcsSocket socket, bool do_allow_reuse_port)
{
    (void)socket;
    (void)do_allow_reuse_port;
    return TCS_ERROR_NOT_SUPPORTED;
}

TcsResult tcs_opt_reuse_port_get(TcsSocket socket, bool* is_reuse_port_allowed)
{
    (void)socket;
    (void)is_reuse_port_allowed;
    return TCS_ERROR_NOT_SUPPORTED;
}

// tcs_opt_send_buffer_size_set() is defined in tinycsocket_common.c
// tcs_opt_send_buffer_size_get() is defined in tinycsocket_common.c
// tcs_opt_receive_buffer_size_set() is defined in tinycsocket_common.c
// tcs_opt_receive_buffer_size_get() is defined in tinycsocket_common.c

TcsResult tcs_opt_receive_timeout_set(TcsSocket socket, int timeout_ms)
{
    if (socket == TCS_SOCKET_INVALID || timeout_ms < 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_opt_receive_timeout_set_impl(socket, timeout_ms);

    return tcs_opt_set(socket, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));
}

TcsResult tcs_opt_receive_timeout_get(TcsSocket socket, int* timeout_ms)
{
    if (socket == TCS_SOCKET_INVALID || timeout_ms == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_opt_receive_timeout_get_impl(socket, timeout_ms);

    DWORD t = 0;
    size_t t_size = sizeof(t);
    TcsResult sts = tcs_opt_get(socket, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &t, &t_size);

    if (sts == TCS_SUCCESS)
    {
        *timeout_ms = (int)t;
    }
    return sts;
}

TcsResult tcs_opt_linger_set(TcsSocket socket, bool do_linger, int timeout_seconds)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {(u_short)do_linger, (u_short)timeout_seconds};
    return tcs_opt_set(socket, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, sizeof(l));
}

TcsResult tcs_opt_linger_get(TcsSocket socket, bool* do_linger, int* timeout_seconds)
{
    if (socket == TCS_SOCKET_INVALID || (do_linger == NULL && timeout_seconds == NULL))
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {0, 0};
    size_t l_size = sizeof(l);
    TcsResult sts = tcs_opt_get(socket, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, &l_size);
    if (sts == TCS_SUCCESS)
    {
        if (do_linger)
            *do_linger = l.l_onoff;
        if (timeout_seconds)
            *timeout_seconds = l.l_linger;
    }

    return sts;
}

// tcs_opt_tcp_no_delay_set() is defined in tinycsocket_common.c
// tcs_opt_tcp_no_delay_get() is defined in tinycsocket_common.c
// tcs_opt_out_of_band_inline_set() is defined in tinycsocket_common.c
// tcs_opt_out_of_band_inline_get() is defined in tinycsocket_common.c
// tcs_opt_priority_set() is defined in tinycsocket_common.c
// tcs_opt_priority_get() is defined in tinycsocket_common.c

TcsResult tcs_opt_nonblocking_set(TcsSocket socket, bool do_non_blocking)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_opt_nonblocking_set_impl(socket, do_non_blocking);

    u_long mode = do_non_blocking ? 1 : 0;
    int sts = ioctlsocket((SOCKET)socket, (long)FIONBIO, &mode);
    return socketstatus2retcode(sts);
}

TcsResult tcs_opt_nonblocking_get(TcsSocket socket, bool* is_non_blocking)
{
    if (socket == TCS_SOCKET_INVALID || is_non_blocking == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_opt_nonblocking_get_impl(socket, is_non_blocking);

    // Win32 does not provide an API to query the non-blocking state of a socket
    return TCS_ERROR_NOT_SUPPORTED;
}

// tcs_opt_membership_add_str() is defined in tinycsocket_common.c

TcsResult tcs_opt_membership_add(TcsSocket socket, const struct TcsAddress* multicast_address)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_opt_membership_add_to(socket, NULL, multicast_address);

    SOCKADDR_STORAGE address_native_local;
    memset(&address_native_local, 0, sizeof address_native_local);
    int address_native_local_size = sizeof address_native_local;
    if (getsockname((SOCKET)socket, (PSOCKADDR)&address_native_local, &address_native_local_size) != 0)
        return wsaerror2retcode(WSAGetLastError());

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    TcsResult sts = native2sockaddr((PSOCKADDR)&address_native_local, &local_address);
    if (sts != TCS_SUCCESS)
        return sts;

    if (local_address.family.native != multicast_address->family.native)
        return TCS_ERROR_INVALID_ARGUMENT;

    return tcs_opt_membership_add_to(socket, &local_address, multicast_address);
}

TcsResult tcs_opt_membership_add_to(TcsSocket socket,
                                    const struct TcsAddress* local_address,
                                    const struct TcsAddress* multicast_address)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_opt_membership_add_to_impl(socket, local_address, multicast_address);

    if (multicast_address->family.native == TCS_FAMILY_IPV4.native)
    {
        struct ip_mreq imr;
        memset(&imr, 0, sizeof imr);
        imr.imr_multiaddr.s_addr = htonl(multicast_address->data.ipv4.address);
        if (local_address != NULL)
            imr.imr_interface.s_addr = htonl(local_address->data.ipv4.address);
        return tcs_opt_set(socket, TCS_SOL_IP, TCS_IP_MEMBERSHIP_ADD, &imr, sizeof(imr));
    }
    else if (multicast_address->family.native == TCS_FAMILY_IPV6.native)
    {
        struct ipv6_mreq imr6;
        memset(&imr6, 0, sizeof imr6);
        memcpy(&imr6.ipv6mr_multiaddr, multicast_address->data.ipv6.address.bytes, 16);
        if (local_address != NULL)
            imr6.ipv6mr_interface = (unsigned long)local_address->data.ipv6.scope_id;
        return tcs_opt_set(socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, &imr6, sizeof(imr6));
    }
    return TCS_ERROR_NOT_SUPPORTED;
}

// tcs_opt_membership_drop_str() is defined in tinycsocket_common.c

TcsResult tcs_opt_membership_drop(TcsSocket socket, const struct TcsAddress* multicast_address)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_opt_membership_drop_from(socket, NULL, multicast_address);

    SOCKADDR_STORAGE address_native_local;
    memset(&address_native_local, 0, sizeof address_native_local);
    int address_native_local_size = sizeof address_native_local;
    if (getsockname((SOCKET)socket, (PSOCKADDR)&address_native_local, &address_native_local_size) != 0)
        return wsaerror2retcode(WSAGetLastError());

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    TcsResult sts = native2sockaddr((PSOCKADDR)&address_native_local, &local_address);
    if (sts != TCS_SUCCESS)
        return sts;

    if (local_address.family.native != multicast_address->family.native)
        return TCS_ERROR_INVALID_ARGUMENT;

    return tcs_opt_membership_drop_from(socket, &local_address, multicast_address);
}

TcsResult tcs_opt_membership_drop_from(TcsSocket socket,
                                       const struct TcsAddress* local_address,
                                       const struct TcsAddress* multicast_address)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_opt_membership_drop_from_impl(socket, local_address, multicast_address);

    if (multicast_address->family.native == TCS_FAMILY_IPV4.native)
    {
        struct ip_mreq imr;
        memset(&imr, 0, sizeof imr);
        imr.imr_multiaddr.s_addr = htonl(multicast_address->data.ipv4.address);
        if (local_address != NULL)
            imr.imr_interface.s_addr = htonl(local_address->data.ipv4.address);
        return tcs_opt_set(socket, TCS_SOL_IP, TCS_IP_MEMBERSHIP_DROP, &imr, sizeof(imr));
    }
    else if (multicast_address->family.native == TCS_FAMILY_IPV6.native)
    {
        struct ipv6_mreq imr6;
        memset(&imr6, 0, sizeof imr6);
        memcpy(&imr6.ipv6mr_multiaddr, multicast_address->data.ipv6.address.bytes, 16);
        if (local_address != NULL)
            imr6.ipv6mr_interface = (unsigned long)local_address->data.ipv6.scope_id;
        return tcs_opt_set(socket, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &imr6, sizeof(imr6));
    }
    return TCS_ERROR_NOT_SUPPORTED;
}

TcsResult tcs_opt_multicast_interface_set(TcsSocket socket, const struct TcsAddress* local_address)
{
    if (socket == TCS_SOCKET_INVALID || local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (tcs_internal_is_socket_packet(socket))
        return TCS_ERROR_NOT_SUPPORTED;

    if (local_address->family.native == TCS_FAMILY_IPV4.native)
    {
        struct in_addr iface;
        iface.s_addr = htonl(local_address->data.ipv4.address);
        return tcs_opt_set(socket, TCS_SOL_IP, IP_MULTICAST_IF, &iface, sizeof(iface));
    }
    else if (local_address->family.native == TCS_FAMILY_IPV6.native)
    {
        unsigned long idx = (unsigned long)local_address->data.ipv6.scope_id;
        return tcs_opt_set(socket, IPPROTO_IPV6, IPV6_MULTICAST_IF, &idx, sizeof(idx));
    }
    return TCS_ERROR_INVALID_ARGUMENT;
}

TcsResult tcs_opt_multicast_loop_set(TcsSocket socket, bool do_loopback)
{
    if (socket == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (tcs_internal_is_socket_packet(socket))
        return TCS_ERROR_NOT_SUPPORTED;

    DWORD val = do_loopback ? 1 : 0;
    return tcs_opt_set(socket, TCS_SOL_IP, IP_MULTICAST_LOOP, &val, sizeof(val));
}

TcsResult tcs_opt_multicast_loop_get(TcsSocket socket, bool* is_loopback)
{
    if (socket == TCS_SOCKET_INVALID || is_loopback == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (tcs_internal_is_socket_packet(socket))
        return TCS_ERROR_NOT_SUPPORTED;

    DWORD val = 0;
    size_t s = sizeof(val);
    TcsResult sts = tcs_opt_get(socket, TCS_SOL_IP, IP_MULTICAST_LOOP, &val, &s);
    *is_loopback = val;
    return sts;
}

// ######## Address and Interface Utilities ########

// Retrieves the user-visible adapter name.
// On Vista+ this is the FriendlyName field in IP_ADAPTER_ADDRESSES.
// On pre-Vista the FriendlyName field does not exist, so we read the registry:
//   HKLM\SYSTEM\CurrentControlSet\Control\Network\{4D36E972-E325-11CE-BFC1-08002BE10318}\<GUID>\Connection
//   Value: "Name" (REG_SZ)
// Falls back to Description (hardware name) if FriendlyName / registry is unavailable.
static TcsResult adapter_get_friendly_name(PIP_ADAPTER_ADDRESSES adapter, char* out_name, int out_name_size)
{
    if (adapter == NULL || out_name == NULL || out_name_size <= 0)
        return TCS_ERROR_INVALID_ARGUMENT;
#if _WIN32_WINNT >= 0x0600
    WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, -1, out_name, out_name_size, NULL, NULL);
#else
    const char prefix[] =
        "SYSTEM\\CurrentControlSet\\Control\\Network\\"
        "{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
    const char suffix[] = "\\Connection";

    bool found = false;
    size_t guid_len = strlen(adapter->AdapterName);
    size_t key_len = sizeof(prefix) - 1 + guid_len + sizeof(suffix) - 1 + 1;

    char key_path[256];
    if (key_len <= sizeof key_path)
    {
        memcpy(key_path, prefix, sizeof(prefix) - 1);
        memcpy(key_path + sizeof(prefix) - 1, adapter->AdapterName, guid_len);
        memcpy(key_path + sizeof(prefix) - 1 + guid_len, suffix, sizeof(suffix)); // includes null

        HKEY hkey = NULL;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key_path, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
        {
            DWORD type = 0;
            DWORD data_size = (DWORD)out_name_size;
            if (RegQueryValueExA(hkey, "Name", NULL, &type, (LPBYTE)out_name, &data_size) == ERROR_SUCCESS &&
                type == REG_SZ && data_size > 1)
            {
                found = true;
            }
            RegCloseKey(hkey);
        }
    }

    if (!found)
        WideCharToMultiByte(CP_UTF8, 0, adapter->Description, -1, out_name, out_name_size, NULL, NULL);
#endif
    return TCS_SUCCESS;
}

static TcsResult adapter_is_up(PIP_ADAPTER_ADDRESSES adapter, bool* out_is_up)
{
    if (adapter == NULL || out_is_up == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
#if _WIN32_WINNT >= 0x0600
    *out_is_up = adapter->OperStatus == IfOperStatusUp;
#else
#ifndef MIB_IF_OPER_STATUS_OPERATIONAL
// Not always exposed in all mingw-w64 versions
#define MIB_IF_OPER_STATUS_OPERATIONAL 5
#endif
    MIB_IFROW ifrow;
    memset(&ifrow, 0, sizeof ifrow);
    ifrow.dwIndex = adapter->IfIndex;
    if (GetIfEntry(&ifrow) != NO_ERROR)
    {
        *out_is_up = false;
        return TCS_SUCCESS;
    }
    *out_is_up = ifrow.dwOperStatus >= MIB_IF_OPER_STATUS_OPERATIONAL;
#endif
    return TCS_SUCCESS;
}

TcsResult tcs_interface_list(struct TcsInterface out_interfaces[], size_t interfaces_length, size_t* out_length)
{
    if (out_interfaces == NULL && out_length == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (out_interfaces == NULL && interfaces_length != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (out_length != NULL)
        *out_length = 0;

    PIP_ADAPTER_ADDRESSES adapters = NULL;
    TcsResult adapters_sts = adapters_get(&adapters);
    if (adapters_sts != TCS_SUCCESS)
        return adapters_sts;
    if (adapters == NULL)
        return TCS_SUCCESS;

    {
        size_t i = 0;
        for (PIP_ADAPTER_ADDRESSES iter = adapters; iter != NULL; iter = iter->Next)
        {
            bool is_up = false;
            TcsResult up_sts = adapter_is_up(iter, &is_up);
            if (up_sts != TCS_SUCCESS)
            {
                free(adapters);
                return TCS_ERROR_SYSTEM;
            }
            if (!is_up)
                continue;

            if (out_interfaces != NULL && i < interfaces_length)
            {
                memset(out_interfaces[i].name, '\0', TCS_CFG_INTERFACE_NAME_SIZE);
                TcsResult name_sts =
                    adapter_get_friendly_name(iter, out_interfaces[i].name, TCS_CFG_INTERFACE_NAME_SIZE - 1);
                if (name_sts != TCS_SUCCESS)
                {
                    free(adapters);
                    return TCS_ERROR_SYSTEM;
                }
                out_interfaces[i].id = iter->IfIndex;
            }
            if (out_length != NULL)
                (*out_length)++;
            ++i;
        }
    }

    free(adapters);
    return TCS_SUCCESS;
}

TcsResult tcs_address_resolve(const char* hostname,
                              TcsFamily address_family,
                              struct TcsAddress out_addresses[],
                              size_t addresses_length,
                              size_t* out_length)
{
    if (hostname == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (out_addresses == NULL && out_length == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (out_length != NULL)
        *out_length = 0;

    // Fast path: try numeric/MAC parse first to avoid DNS lookup
    struct TcsAddress parsed = TCS_ADDRESS_NONE;
    if (tcs_address_parse(hostname, &parsed) == TCS_SUCCESS && parsed.family.native != TCS_FAMILY_ANY.native)
    {
        if (address_family.native == TCS_FAMILY_ANY.native || parsed.family.native == address_family.native)
        {
            if (out_addresses != NULL && addresses_length > 0)
                out_addresses[0] = parsed;
            if (out_length != NULL)
                *out_length = 1;
            return TCS_SUCCESS;
        }
    }

    if (address_family.native == TCS_FAMILY_PACKET.native) // Only the numeric fast path above can give a MAC address
        return TCS_ERROR_NOT_SUPPORTED;

    ADDRINFOA native_hints;
    memset(&native_hints, 0, sizeof native_hints);
    native_hints.ai_family = address_family.native;

    PADDRINFOA native_addrinfo_list = NULL;
    int getaddrinfo_status = getaddrinfo(hostname, NULL, &native_hints, &native_addrinfo_list);
    if (getaddrinfo_status != 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    if (native_addrinfo_list == NULL)
        return TCS_ERROR_UNKNOWN;

    size_t i = 0;
    if (out_addresses == NULL)
    {
        for (PADDRINFOA iter = native_addrinfo_list; iter != NULL; iter = iter->ai_next)
            i++;
    }
    else
    {
        for (PADDRINFOA iter = native_addrinfo_list; iter != NULL && i < addresses_length; iter = iter->ai_next)
        {
            if (iter->ai_addr == NULL)
                continue;
            TcsResult address_convert_status = native2sockaddr(iter->ai_addr, &out_addresses[i]);
            if (address_convert_status != TCS_SUCCESS)
                continue;
            i++;
        }
    }
    if (out_length != NULL)
        *out_length = i;

    freeaddrinfo(native_addrinfo_list);

    if (i == 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    return TCS_SUCCESS;
}

TcsResult tcs_address_list(unsigned int interface_id_filter,
                           TcsFamily address_family_filter,
                           struct TcsInterfaceAddress out_interface_addresses[],
                           size_t interface_addresses_length,
                           size_t* out_length)
{
    if (out_interface_addresses == NULL && out_length == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (out_interface_addresses == NULL && interface_addresses_length != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (out_length != NULL)
        *out_length = 0;

    PIP_ADAPTER_ADDRESSES adapters = NULL;
    TcsResult adapters_sts = adapters_get(&adapters);
    if (adapters_sts != TCS_SUCCESS)
        return adapters_sts;
    if (adapters == NULL)
        return TCS_SUCCESS;

    size_t populated = 0;
    for (PIP_ADAPTER_ADDRESSES iter = adapters; iter != NULL; iter = iter->Next)
    {
        bool is_up = false;
        TcsResult up_sts = adapter_is_up(iter, &is_up);
        if (up_sts != TCS_SUCCESS)
        {
            free(adapters);
            return TCS_ERROR_SYSTEM;
        }
        if (!is_up)
            continue;

        if (interface_id_filter != 0 && iter->IfIndex != interface_id_filter)
            continue;

        for (PIP_ADAPTER_UNICAST_ADDRESS address_iter = iter->FirstUnicastAddress; address_iter != NULL;
             address_iter = address_iter->Next)
        {
            if (address_iter->Address.lpSockaddr == NULL)
                continue;

            if (address_family_filter.native != TCS_FAMILY_ANY.native &&
                address_iter->Address.lpSockaddr->sa_family != address_family_filter.native)
                continue;

            struct TcsAddress address = TCS_ADDRESS_NONE;
            TcsResult convert_sts = native2sockaddr(address_iter->Address.lpSockaddr, &address);
            if (convert_sts != TCS_SUCCESS)
                continue; // skip entries we cannot represent (unknown family, malformed sockaddr, etc.)

            if (out_interface_addresses != NULL && populated < interface_addresses_length)
            {
                memset(out_interface_addresses[populated].iface.name, '\0', TCS_CFG_INTERFACE_NAME_SIZE);
                TcsResult name_sts = adapter_get_friendly_name(
                    iter, out_interface_addresses[populated].iface.name, TCS_CFG_INTERFACE_NAME_SIZE - 1);
                if (name_sts != TCS_SUCCESS)
                {
                    free(adapters);
                    return TCS_ERROR_SYSTEM;
                }
                out_interface_addresses[populated].iface.id = iter->IfIndex;
                out_interface_addresses[populated].address = address;
                populated++;

                if (out_length != NULL)
                    (*out_length)++;
            }
            else if (out_interface_addresses == NULL && out_length != NULL)
            {
                (*out_length)++;
            }
        }

        // The layer 2 address of adapters with an Ethernet MAC. Needs no Npcap, so it is listed even when packet
        // sockets are not supported.
        if ((address_family_filter.native == TCS_FAMILY_ANY.native ||
             address_family_filter.native == TCS_FAMILY_PACKET.native) &&
            iter->PhysicalAddressLength == 6)
        {
            if (out_interface_addresses != NULL && populated < interface_addresses_length)
            {
                memset(out_interface_addresses[populated].iface.name, '\0', TCS_CFG_INTERFACE_NAME_SIZE);
                TcsResult name_sts = adapter_get_friendly_name(
                    iter, out_interface_addresses[populated].iface.name, TCS_CFG_INTERFACE_NAME_SIZE - 1);
                if (name_sts != TCS_SUCCESS)
                {
                    free(adapters);
                    return TCS_ERROR_SYSTEM;
                }
                out_interface_addresses[populated].iface.id = iter->IfIndex;
                out_interface_addresses[populated].address = TCS_ADDRESS_NONE;
                out_interface_addresses[populated].address.family = TCS_FAMILY_PACKET;
                out_interface_addresses[populated].address.data.packet.interface_id = iter->IfIndex;
                memcpy(out_interface_addresses[populated].address.data.packet.mac, iter->PhysicalAddress, 6);
                populated++;

                if (out_length != NULL)
                    (*out_length)++;
            }
            else if (out_interface_addresses == NULL && out_length != NULL)
            {
                (*out_length)++;
            }
        }
    }

    free(adapters);
    return TCS_SUCCESS;
}

TcsResult tcs_address_socket_local(TcsSocket socket, struct TcsAddress* local_address)
{
    if (socket == TCS_SOCKET_INVALID || local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
        return tcs_internal_packet_address_socket_local_impl(socket, local_address);

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = sizeof native_sockaddr;
    if (getsockname((SOCKET)socket, (PSOCKADDR)&native_sockaddr, &addrlen) != 0)
        return wsaerror2retcode(WSAGetLastError());

    return native2sockaddr((PSOCKADDR)&native_sockaddr, local_address);
}

TcsResult tcs_address_socket_remote(TcsSocket socket, struct TcsAddress* remote_address)
{
    if (socket == TCS_SOCKET_INVALID || remote_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (tcs_internal_is_socket_packet(socket))
        return TCS_ERROR_NOT_CONNECTED;

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = sizeof native_sockaddr;
    if (getpeername((SOCKET)socket, (PSOCKADDR)&native_sockaddr, &addrlen) != 0)
        return wsaerror2retcode(WSAGetLastError());

    return native2sockaddr((PSOCKADDR)&native_sockaddr, remote_address);
}

TcsResult tcs_address_socket_family(TcsSocket socket, TcsFamily* out_family)
{
    if (socket == TCS_SOCKET_INVALID || out_family == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tcs_internal_is_socket_packet(socket))
    {
        *out_family = TCS_FAMILY_PACKET;
        return TCS_SUCCESS;
    }

    WSAPROTOCOL_INFOW info;
    memset(&info, 0, sizeof info);
    int info_size = sizeof info;
    if (getsockopt((SOCKET)socket, SOL_SOCKET, SO_PROTOCOL_INFOW, (char*)&info, &info_size) != 0)
        return wsaerror2retcode(WSAGetLastError());

    out_family->native = (int)info.iAddressFamily;
    return TCS_SUCCESS;
}

// tcs_address_parse() is defined in tinycsocket_common.c
// tcs_address_to_str() is defined in tinycsocket_common.c
// tcs_address_is_equal() is defined in tinycsocket_common.c
// tcs_address_is_any() is defined in tinycsocket_common.c
// tcs_address_is_link_local() is defined in tinycsocket_common.c
// tcs_address_is_loopback() is defined in tinycsocket_common.c
// tcs_address_is_multicast() is defined in tinycsocket_common.c
// tcs_address_is_broadcast() is defined in tinycsocket_common.c

bool tcs_address_is_supported(const struct TcsAddress* addr)
{
    if (addr == NULL)
        return false;
    if (addr->family.native == TCS_FAMILY_ANY.native)
        return true;
    if (addr->family.native == TCS_FAMILY_IPV4.native)
        return true;
    if (addr->family.native == TCS_FAMILY_IPV6.native)
        return true;
    if (addr->family.native == TCS_FAMILY_PACKET.native)
        return tcs_internal_packet_address_is_supported_impl(addr);
    return false;
}

#endif
