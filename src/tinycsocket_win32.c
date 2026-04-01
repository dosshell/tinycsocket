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

#define WIN32_LEAN_AND_MEAN
// Header only should not need other files
#ifndef TINYDATASTRUCTURES_H_
#include "tinydatastructures.h"
#endif
// before windows.h
#include <winsock2.h> // sockets

#include <windows.h>

// after windows.h
#include <iphlpapi.h> // GetAdaptersAddresses
#include <ws2tcpip.h> // getaddrinfo

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

#ifdef __cplusplus
using std::min;
#endif

#ifndef ULIST_SOC
#define ULIST_SOC
TDS_ULIST_IMPL(SOCKET, soc)
#endif

#ifndef ULIST_PVOID
#define ULIST_PVOID
TDS_ULIST_IMPL(void*, pvoid)
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

struct TcsPool
{
    struct TdsUList_soc read_sockets;
    struct TdsUList_soc write_sockets;
    struct TdsUList_soc error_sockets;
    struct TdsMap_socket_user user_data;
};

const TcsSocket TCS_SOCKET_INVALID = INVALID_SOCKET;
const int TCS_WAIT_INF = -1;

// Addresses
const uint32_t TCS_ADDRESS_ANY_IP4 = INADDR_ANY;
const uint32_t TCS_ADDRESS_LOOPBACK_IP4 = INADDR_LOOPBACK;
const uint32_t TCS_ADDRESS_BROADCAST_IP4 = INADDR_BROADCAST;
const uint32_t TCS_ADDRESS_NONE_IP4 = INADDR_NONE;

// Type
const int TCS_SOCK_STREAM = SOCK_STREAM;
const int TCS_SOCK_DGRAM = SOCK_DGRAM;
const int TCS_SOCK_RAW = SOCK_RAW;

// Protocol
const uint16_t TCS_PROTOCOL_IP_TCP = IPPROTO_TCP;
const uint16_t TCS_PROTOCOL_IP_UDP = IPPROTO_UDP;

// Flags
const uint32_t TCS_AI_PASSIVE = AI_PASSIVE;

// Recv flags
const uint32_t TCS_MSG_PEEK = MSG_PEEK;
const uint32_t TCS_MSG_OOB = MSG_OOB;
const uint32_t TCS_MSG_WAITALL = 0x8; // Binary compatible when it does not exist

// Send flags
const uint32_t TCS_MSG_SENDALL = 0x80000000;

// Backlog
const int TCS_BACKLOG_MAX = SOMAXCONN;

// Option levels
const int TCS_SOL_SOCKET = SOL_SOCKET;
const int TCS_SOL_IP = IPPROTO_IP;

// Socket options
const int TCS_SO_BROADCAST = SO_BROADCAST;
const int TCS_SO_KEEPALIVE = SO_KEEPALIVE;
const int TCS_SO_LINGER = SO_LINGER;
const int TCS_SO_REUSEADDR = SO_REUSEADDR;
const int TCS_SO_RCVBUF = SO_RCVBUF;
const int TCS_SO_RCVTIMEO = SO_RCVTIMEO;
const int TCS_SO_SNDBUF = SO_SNDBUF;
const int TCS_SO_OOBINLINE = SO_OOBINLINE;
const int TCS_SO_PRIORITY = -1;

// IP options
const int TCS_SO_IP_NODELAY = TCP_NODELAY;
const int TCS_SO_IP_MEMBERSHIP_ADD = IP_ADD_MEMBERSHIP;
const int TCS_SO_IP_MEMBERSHIP_DROP = IP_DROP_MEMBERSHIP;
const int TCS_SO_IP_MULTICAST_LOOP = IP_MULTICAST_LOOP;

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

static TcsResult family2native(const TcsAddressFamily family, short* native_family)
{
    switch (family)
    {
        case TCS_AF_ANY:
            *native_family = AF_UNSPEC;
            return TCS_SUCCESS;
        case TCS_AF_IP4:
            *native_family = AF_INET;
            return TCS_SUCCESS;
        case TCS_AF_IP6:
            *native_family = AF_INET6;
            return TCS_SUCCESS;

        case TCS_AF_PACKET:
#if TCS_AVAILABLE_AF_PACKET
            *native_family = AF_PACKET;
#else
            return TCS_ERROR_NOT_IMPLEMENTED;
#endif
            return TCS_SUCCESS;
        default:
            return TCS_ERROR_INVALID_ARGUMENT;
    }
    return TCS_SUCCESS;
}

static TcsResult sockaddr2native(const struct TcsAddress* in_addr, PSOCKADDR out_addr, int* out_addrlen)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->family == TCS_AF_ANY)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    if (in_addr->family == TCS_AF_IP4)
    {
        PSOCKADDR_IN addr = (PSOCKADDR_IN)out_addr;
        addr->sin_family = (ADDRESS_FAMILY)AF_INET;
        addr->sin_port = htons((USHORT)in_addr->data.ip4.port);
        addr->sin_addr.S_un.S_addr = htonl((ULONG)in_addr->data.ip4.address);

        if (out_addrlen != NULL)
            *out_addrlen = sizeof(SOCKADDR_IN);

        return TCS_SUCCESS;
    }
    else if (in_addr->family == TCS_AF_IP6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (in_addr->family == TCS_AF_PACKET)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    return TCS_ERROR_NOT_IMPLEMENTED;
}

static TcsResult native2family(const short native_family, TcsAddressFamily* family)
{
    switch (native_family)
    {
        case AF_UNSPEC:
            *family = TCS_AF_ANY;
            return TCS_SUCCESS;
        case AF_INET:
            *family = TCS_AF_IP4;
            return TCS_SUCCESS;
        case AF_INET6:
            *family = TCS_AF_IP6;
            return TCS_SUCCESS;
        default:
            return TCS_ERROR_NOT_IMPLEMENTED;
    }
    return TCS_ERROR_UNKNOWN;
}

static TcsResult native2sockaddr(const PSOCKADDR in_addr, struct TcsAddress* out_addr)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->sa_family == AF_INET)
    {
        PSOCKADDR_IN addr = (PSOCKADDR_IN)in_addr;
        out_addr->family = TCS_AF_IP4;
        out_addr->data.ip4.port = ntohs((uint16_t)addr->sin_port);
        out_addr->data.ip4.address = ntohl((uint32_t)addr->sin_addr.S_un.S_addr);
    }
    else if (in_addr->sa_family == AF_INET6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
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

TcsResult tcs_lib_init(void)
{
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        return TCS_ERROR_SYSTEM;
    return TCS_SUCCESS;
}

TcsResult tcs_lib_free(void)
{
    WSACleanup();
    return TCS_SUCCESS;
}

TcsResult tcs_socket(TcsSocket* socket_ctx, TcsAddressFamily family, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    short native_family = AF_UNSPEC;
    TcsResult sts = family2native(family, &native_family);
    if (sts != TCS_SUCCESS)
        return sts;

    TcsSocket new_socket = socket(native_family, type, protocol);

    if (new_socket != INVALID_SOCKET)
    {
        *socket_ctx = new_socket;
        return TCS_SUCCESS;
    }
    else
    {
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }
}

// tcs_socket_preset() is defined in tinycsocket_common.c

TcsResult tcs_close(TcsSocket* socket_ctx)
{
    if (socket_ctx == NULL || *socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    int close_status = closesocket(*socket_ctx);
    if (close_status != SOCKET_ERROR)
    {
        *socket_ctx = TCS_SOCKET_INVALID;
        return TCS_SUCCESS;
    }
    else
    {
        return socketstatus2retcode(close_status);
    }
}

// ######## High-level Socket Creation ########

// tcs_tcp_server_str() is defined in tinycsocket_common.c
// tcs_tcp_server() is defined in tinycsocket_common.c
// tcs_tcp_client_str() is defined in tinycsocket_common.c
// tcs_tcp_client() is defined in tinycsocket_common.c
// tcs_udp_receiver_str() is defined in tinycsocket_common.c
// tcs_udp_receiver() is defined in tinycsocket_common.c
// tcs_udp_sender_str() is defined in tinycsocket_common.c
// tcs_udp_sender() is defined in tinycsocket_common.c
// tcs_udp_peer_str() is defined in tinycsocket_common.c
// tcs_udp_peer() is defined in tinycsocket_common.c

// ######## High-level Raw L2-Packet Sockets (Experimental) ########

// tcs_raw() is defined in tinycsocket_common.c
// tcs_raw_str() is defined in tinycsocket_common.c

// ######## High-level L2-Packet DGRAM Sockets (Experimental) ########

// tcs_packet() is defined in tinycsocket_common.c
// tcs_packet_str() is defined in tinycsocket_common.c

// ######## Socket Operations ########

TcsResult tcs_bind(TcsSocket socket_ctx, const struct TcsAddress* address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = 0;
    TcsResult convert_addr_status = sockaddr2native(address, (PSOCKADDR)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;
    int bind_status = bind(socket_ctx, (PSOCKADDR)&native_sockaddr, addrlen);
    return socketstatus2retcode(bind_status);
}

TcsResult tcs_connect(TcsSocket socket_ctx, const struct TcsAddress* address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = 0;
    TcsResult convert_addr_status = sockaddr2native(address, (PSOCKADDR)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;
    int connect_status = connect(socket_ctx, (PSOCKADDR)&native_sockaddr, addrlen);
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

TcsResult tcs_listen(TcsSocket socket_ctx, int backlog)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = listen(socket_ctx, (int)backlog);
    return socketstatus2retcode(status);
}

TcsResult tcs_accept(TcsSocket socket_ctx, TcsSocket* child_socket_ctx, struct TcsAddress* address)
{
    if (socket_ctx == TCS_SOCKET_INVALID || child_socket_ctx == NULL || *child_socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addr_len = sizeof(native_sockaddr);
    *child_socket_ctx = accept(socket_ctx, (PSOCKADDR)&native_sockaddr, &addr_len);
    if (*child_socket_ctx != INVALID_SOCKET)
    {
        if (address != NULL)
        {
            TcsResult convert_addr_status = native2sockaddr((PSOCKADDR)&native_sockaddr, address);
            if (convert_addr_status != TCS_SUCCESS)
                return convert_addr_status;
        }
        return TCS_SUCCESS;
    }
    else
    {
        *child_socket_ctx = TCS_SOCKET_INVALID;
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }
}

TcsResult tcs_shutdown(TcsSocket socket_ctx, TcsSocketDirection direction)
{
    const int LUT[] = {SD_RECEIVE, SD_SEND, SD_BOTH};

    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (direction < 0 || direction > 2)
        return TCS_ERROR_INVALID_ARGUMENT;

    const int how = LUT[direction];
    int shutdown_status = shutdown(socket_ctx, how);
    return socketstatus2retcode(shutdown_status);
}

// ######## Data Transfer ########

TcsResult tcs_send(TcsSocket socket_ctx, const uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_sent)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (bytes_sent != NULL)
        *bytes_sent = 0;

    if (buffer == NULL || buffer_size == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    // Send all
    if (flags & TCS_MSG_SENDALL)
    {
        uint32_t new_flags = flags & ~TCS_MSG_SENDALL; // For recursive call
        size_t left = buffer_size;
        const uint8_t* iterator = buffer;

        while (left > 0)
        {
            size_t sent = 0;
            TcsResult sts = tcs_send(socket_ctx, iterator, left, new_flags, &sent);
            if (bytes_sent != NULL)
                *bytes_sent += sent;
            if (sts != TCS_SUCCESS)
                return sts;
            left -= sent;
            iterator += sent;
        }
        return TCS_SUCCESS;
    }
    else // Send
    {
        int send_status = send(socket_ctx, (const char*)buffer, (int)buffer_size, (int)flags);
        if (send_status != SOCKET_ERROR)
        {
            if (bytes_sent != NULL)
                *bytes_sent = (size_t)send_status;
            return TCS_SUCCESS;
        }
        else
        {
            if (bytes_sent != NULL)
                *bytes_sent = 0;

            return socketstatus2retcode(send_status);
        }
    }
}

TcsResult tcs_send_to(TcsSocket socket_ctx,
                      const uint8_t* buffer,
                      size_t buffer_size,
                      uint32_t flags,
                      const struct TcsAddress* destination_address,
                      size_t* bytes_sent)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (flags & TCS_MSG_SENDALL)
        return TCS_ERROR_NOT_IMPLEMENTED;

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = 0;
    TcsResult convert_addr_status = sockaddr2native(destination_address, (PSOCKADDR)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;

    int sendto_status =
        sendto(socket_ctx, (const char*)buffer, (int)buffer_size, (int)flags, (PSOCKADDR)&native_sockaddr, addrlen);

    if (sendto_status != SOCKET_ERROR)
    {
        if (bytes_sent != NULL)
            *bytes_sent = (size_t)sendto_status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return socketstatus2retcode(sendto_status);
    }
}

TcsResult tcs_sendv(TcsSocket socket_ctx,
                    const struct TcsBuffer* buffers,
                    size_t buffer_count,
                    uint32_t flags,
                    size_t* bytes_sent)
{
    if (socket_ctx == TCS_SOCKET_INVALID || buffers == NULL || buffer_count == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (flags & TCS_MSG_SENDALL)
        return TCS_ERROR_NOT_IMPLEMENTED;

    WSABUF stack_buffers[TCS_SENDV_STACK_MAX];
    WSABUF* native_buffers = stack_buffers;
    WSABUF* heap_buffers = NULL;

    if (buffer_count > TCS_SENDV_STACK_MAX)
    {
        heap_buffers = (WSABUF*)malloc(sizeof(WSABUF) * buffer_count);
        if (heap_buffers == NULL)
            return TCS_ERROR_MEMORY;
        native_buffers = heap_buffers;
    }

    for (size_t i = 0; i < buffer_count; ++i)
    {
        native_buffers[i].buf = (CHAR*)buffers[i].data;
        native_buffers[i].len = (ULONG)buffers[i].size;
    }

    DWORD sent = 0;
    int wsasend_status = WSASend(socket_ctx, native_buffers, (DWORD)buffer_count, &sent, (DWORD)flags, NULL, NULL);

    free(heap_buffers);

    if (bytes_sent != NULL)
        *bytes_sent = (size_t)sent;

    if (wsasend_status != SOCKET_ERROR)
        return TCS_SUCCESS;
    else
        return socketstatus2retcode(wsasend_status);
}

TcsResult tcs_receive(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_received)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

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
            TcsResult sts = tcs_receive(socket_ctx, cursor, left, new_flags, &received_now);
            if (sts != TCS_SUCCESS)
            {
                if (bytes_received != NULL)
                    *bytes_received = received_so_far;
                return sts;
            }
            received_so_far += received_now;
        }
        if (bytes_received != NULL)
            *bytes_received = received_so_far;
        return TCS_SUCCESS;
    }
#endif

    int recv_status = recv(socket_ctx, (char*)buffer, (int)buffer_size, (int)flags);

    if (recv_status == 0)
    {
        if (bytes_received != NULL)
            *bytes_received = 0;
        return TCS_ERROR_SOCKET_CLOSED;
    }
    else if (recv_status != SOCKET_ERROR)
    {
        if (bytes_received != NULL)
            *bytes_received = (size_t)recv_status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_received != NULL)
            *bytes_received = 0;

        return socketstatus2retcode(recv_status);
    }
}

TcsResult tcs_receive_from(TcsSocket socket_ctx,
                           uint8_t* buffer,
                           size_t buffer_size,
                           uint32_t flags,
                           struct TcsAddress* source_address,
                           size_t* bytes_received)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = sizeof(native_sockaddr);

    int recvfrom_status =
        recvfrom(socket_ctx, (char*)buffer, (int)buffer_size, (int)flags, (PSOCKADDR)&native_sockaddr, &addrlen);

    if (recvfrom_status == 0)
    {
        return TCS_ERROR_SOCKET_CLOSED;
    }
    else if (recvfrom_status != SOCKET_ERROR)
    {
        if (bytes_received != NULL)
            *bytes_received = (size_t)recvfrom_status;

        if (source_address != NULL)
        {
            TcsResult convert_address_status = native2sockaddr((PSOCKADDR)&native_sockaddr, source_address);
            return convert_address_status;
        }

        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_received != NULL)
            *bytes_received = 0;

        return socketstatus2retcode(recvfrom_status);
    }
}

// tcs_receive_line() is defined in tinycsocket_common.c
// tcs_receive_netstring() is defined in tinycsocket_common.c

// ######## Socket Pooling ########

TcsResult tcs_pool_create(struct TcsPool** pool)
{
    if (pool == NULL || *pool != NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    *pool = (struct TcsPool*)malloc(sizeof(struct TcsPool));
    if (*pool == NULL)
        return TCS_ERROR_MEMORY;
    memset(*pool, 0, sizeof(struct TcsPool)); // Just to be safe
    int sts_read_array = tds_ulist_soc_create(&(*pool)->read_sockets);
    int sts_write_array = tds_ulist_soc_create(&(*pool)->write_sockets);
    int sts_error_array = tds_ulist_soc_create(&(*pool)->error_sockets);
    int sts_user_data = tds_map_socket_user_create(&(*pool)->user_data);

    if (sts_read_array != 0 || sts_write_array != 0 || sts_error_array != 0 || sts_user_data != 0)
    {
        tcs_pool_destroy(pool);
        return TCS_ERROR_MEMORY;
    }
    return TCS_SUCCESS;
}

TcsResult tcs_pool_destroy(struct TcsPool** pool)
{
    if (pool == NULL || *pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // Free away!
    tds_ulist_soc_destroy(&(*pool)->read_sockets);
    tds_ulist_soc_destroy(&(*pool)->write_sockets);
    tds_ulist_soc_destroy(&(*pool)->error_sockets);
    tds_map_socket_user_destroy(&(*pool)->user_data);

    free(*pool);
    *pool = NULL;

    return TCS_SUCCESS;
}

TcsResult tcs_pool_add(struct TcsPool* pool,
                       TcsSocket socket_ctx,
                       void* user_data,
                       bool poll_can_read,
                       bool poll_can_write,
                       bool poll_error)
{
    if (pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (!poll_can_read && !poll_can_write && !poll_error)
        return TCS_ERROR_INVALID_ARGUMENT;

    int map_sts = tds_map_socket_user_add(&pool->user_data, socket_ctx, user_data);
    if (map_sts != 0)
        return TCS_ERROR_MEMORY;

    if (poll_can_read)
    {
        int sts = tds_ulist_soc_add(&pool->read_sockets, &socket_ctx, 1);
        if (sts != 0)
        {
            return TCS_ERROR_MEMORY;
        }
    }
    if (poll_can_write)
    {
        int sts = tds_ulist_soc_add(&pool->write_sockets, &socket_ctx, 1);
        if (sts != 0)
        {
            tds_ulist_soc_remove(&pool->read_sockets, pool->read_sockets.count, 1);
            return TCS_ERROR_MEMORY;
        }
    }
    if (poll_error)
    {
        int sts = tds_ulist_soc_add(&pool->error_sockets, &socket_ctx, 1);
        if (sts != 0)
        {
            tds_ulist_soc_remove(&pool->read_sockets, pool->read_sockets.count, 1);
            tds_ulist_soc_remove(&pool->write_sockets, pool->write_sockets.count, 1);
            return TCS_ERROR_MEMORY;
        }
    }
    return TCS_SUCCESS;
}

TcsResult tcs_pool_remove(struct TcsPool* pool, TcsSocket socket_ctx)
{
    for (size_t i = 0; i < pool->read_sockets.count; ++i)
    {
        if (pool->read_sockets.data[i] == socket_ctx)
        {
            tds_ulist_soc_remove(&pool->read_sockets, i, 1);
            break;
        }
    }
    for (size_t i = 0; i < pool->write_sockets.count; ++i)
    {
        if (pool->write_sockets.data[i] == socket_ctx)
        {
            tds_ulist_soc_remove(&pool->write_sockets, i, 1);
            break;
        }
    }
    for (size_t i = 0; i < pool->error_sockets.count; ++i)
    {
        if (pool->error_sockets.data[i] == socket_ctx)
        {
            tds_ulist_soc_remove(&pool->error_sockets, i, 1);
            break;
        }
    }
    for (size_t i = 0; i < pool->user_data.count; ++i)
    {
        if (pool->user_data.keys[i] == socket_ctx)
        {
            tds_map_socket_user_remove(&pool->user_data, i);
            break;
        }
    }

    return TCS_SUCCESS;
}

TcsResult tcs_pool_poll(struct TcsPool* pool,
                        struct TcsPollEvent* events,
                        size_t events_capacity,
                        size_t* events_populated,
                        int64_t timeout_in_ms)
{
    if (pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (timeout_in_ms != TCS_WAIT_INF && (timeout_in_ms > 0xffffffff || timeout_in_ms < 0))
        return TCS_ERROR_INVALID_ARGUMENT;
    if (events == NULL || events_populated == NULL)
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

    if (pool->read_sockets.count <= FD_SETSIZE)
    {
        FD_ZERO(&rfds_stack);
        rfds_cpy = (struct tcs_fd_set*)&rfds_stack;
    }
    else
    {
        rfds_heap = (struct tcs_fd_set*)malloc(data_offset + sizeof(SOCKET) * pool->read_sockets.count);
        rfds_cpy = rfds_heap;
    }

    if (pool->write_sockets.count <= FD_SETSIZE)
    {
        FD_ZERO(&wfds_stack);
        wfds_cpy = (struct tcs_fd_set*)&wfds_stack;
    }
    else
    {
        wfds_heap = (struct tcs_fd_set*)malloc(data_offset + sizeof(SOCKET) * pool->write_sockets.count);
        wfds_cpy = wfds_heap;
    }

    if (pool->error_sockets.count <= FD_SETSIZE)
    {
        FD_ZERO(&efds_stack);
        efds_cpy = (struct tcs_fd_set*)&efds_stack;
    }
    else
    {
        efds_heap = (struct tcs_fd_set*)malloc(data_offset + sizeof(SOCKET) * pool->error_sockets.count);
        efds_cpy = efds_heap;
    }

    if (rfds_cpy == NULL || wfds_cpy == NULL || efds_cpy == NULL)
    {
        if (rfds_heap != NULL)
            free(rfds_heap);
        if (wfds_heap != NULL)
            free(wfds_heap);
        if (efds_heap != NULL)
            free(efds_heap);

        return TCS_ERROR_MEMORY;
    }
    rfds_cpy->fd_count = (u_int)pool->read_sockets.count;
    wfds_cpy->fd_count = (u_int)pool->write_sockets.count;
    efds_cpy->fd_count = (u_int)pool->error_sockets.count;

    memcpy(rfds_cpy->fd_array, pool->read_sockets.data, sizeof(SOCKET) * pool->read_sockets.count);
    memcpy(wfds_cpy->fd_array, pool->write_sockets.data, sizeof(SOCKET) * pool->write_sockets.count);
    memcpy(efds_cpy->fd_array, pool->error_sockets.data, sizeof(SOCKET) * pool->error_sockets.count);

    memset(events, 0, sizeof(struct TcsPollEvent) * events_capacity);
    *events_populated = 0;

    // Run select
    struct timeval* t_ptr = NULL;
    struct timeval t = {0, 0};
    if (timeout_in_ms != TCS_WAIT_INF)
    {
        t.tv_sec = (long)(timeout_in_ms / 1000);
        t.tv_usec = (long)(timeout_in_ms % 1000) * 1000;
        t_ptr = &t;
    }
    int no = select(IGNORE, (fd_set*)rfds_cpy, (fd_set*)wfds_cpy, (fd_set*)efds_cpy, t_ptr);

    size_t events_added = 0;

    if (no > 0)
    {
        for (u_int n = 0; n < rfds_cpy->fd_count && events_added < events_capacity; ++n)
        {
            events[events_added].socket = rfds_cpy->fd_array[n];
            events[events_added].can_read = true;
            for (size_t i = 0; i < pool->user_data.count; ++i)
            {
                if (events[events_added].socket == pool->user_data.keys[i])
                {
                    events[events_added].user_data = pool->user_data.values[i];
                    break;
                }
            }
            events_added++;
        }
        for (u_int n = 0; n < wfds_cpy->fd_count && events_added < events_capacity; ++n)
        {
            // Check already added events
            size_t new_n = events_added;
            for (size_t m = 0; m < events_added; ++m)
            {
                if (events[m].socket == wfds_cpy->fd_array[n])
                {
                    new_n = m;
                    break;
                }
            }
            events[new_n].can_write = true;
            // Check for new events
            if (events_added == new_n)
            {
                events[new_n].socket = wfds_cpy->fd_array[n];

                for (size_t i = 0; i < pool->user_data.count; ++i)
                {
                    if (events[new_n].socket == pool->user_data.keys[i])
                    {
                        events[new_n].user_data = pool->user_data.values[i];
                        break;
                    }
                }
                events_added++;
            }
        }
        for (u_int n = 0; n < efds_cpy->fd_count && events_added < events_capacity; ++n)
        {
            // Check already added events
            size_t new_n = events_added;
            for (size_t m = 0; m < events_added; ++m)
            {
                if (events[m].socket == efds_cpy->fd_array[n])
                {
                    new_n = m;
                    break;
                }
            }
            // Check for new events
            events[new_n].error = TCS_ERROR_NOT_IMPLEMENTED; //TODO(markusl): Make this a proper error
            if (events_added == new_n)
            {
                events[new_n].socket = efds_cpy->fd_array[n];

                for (size_t i = 0; i < pool->user_data.count; ++i)
                {
                    if (events[new_n].socket == pool->user_data.keys[i])
                    {
                        events[new_n].user_data = pool->user_data.values[i];
                        break;
                    }
                }

                events_added++;
            }
        }
    }

    *events_populated = events_added;

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

TcsResult tcs_opt_set(TcsSocket socket_ctx,
                      int32_t level,
                      int32_t option_name,
                      const void* option_value,
                      size_t option_size)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (option_name == -1)
        return TCS_ERROR_NOT_IMPLEMENTED;

    int sockopt_status =
        setsockopt(socket_ctx, (int)level, (int)option_name, (const char*)option_value, (int)option_size);
    return socketstatus2retcode(sockopt_status);
}

TcsResult tcs_opt_get(TcsSocket socket_ctx, int32_t level, int32_t option_name, void* option_value, size_t* option_size)
{
    if (socket_ctx == TCS_SOCKET_INVALID || option_value == NULL || option_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (option_name == -1)
        return TCS_ERROR_NOT_IMPLEMENTED;

    int sockopt_status = getsockopt(socket_ctx, (int)level, (int)option_name, (char*)option_value, (int*)option_size);
    return socketstatus2retcode(sockopt_status);
}

// tcs_opt_broadcast_set() is defined in tinycsocket_common.c
// tcs_opt_broadcast_get() is defined in tinycsocket_common.c
// tcs_opt_keep_alive_set() is defined in tinycsocket_common.c
// tcs_opt_keep_alive_get() is defined in tinycsocket_common.c
// tcs_opt_reuse_address_set() is defined in tinycsocket_common.c
// tcs_opt_reuse_address_get() is defined in tinycsocket_common.c
// tcs_opt_send_buffer_size_set() is defined in tinycsocket_common.c
// tcs_opt_send_buffer_size_get() is defined in tinycsocket_common.c
// tcs_opt_receive_buffer_size_set() is defined in tinycsocket_common.c
// tcs_opt_receive_buffer_size_get() is defined in tinycsocket_common.c

TcsResult tcs_opt_receive_timeout_set(TcsSocket socket_ctx, int timeout_ms)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    return tcs_opt_set(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));
}

TcsResult tcs_opt_receive_timeout_get(TcsSocket socket_ctx, int* timeout_ms)
{
    if (socket_ctx == TCS_SOCKET_INVALID || timeout_ms == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    DWORD t = 0;
    size_t t_size = sizeof(t);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &t, &t_size);

    if (sts == TCS_SUCCESS)
    {
        *timeout_ms = (int)t;
    }
    return sts;
}

TcsResult tcs_opt_linger_set(TcsSocket socket_ctx, bool do_linger, int timeout_seconds)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {(u_short)do_linger, (u_short)timeout_seconds};
    return tcs_opt_set(socket_ctx, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, sizeof(l));
}

TcsResult tcs_opt_linger_get(TcsSocket socket_ctx, bool* do_linger, int* timeout_seconds)
{
    if (socket_ctx == TCS_SOCKET_INVALID || (do_linger == NULL && timeout_seconds == NULL))
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {0, 0};
    size_t l_size = sizeof(l);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, &l_size);
    if (sts == TCS_SUCCESS)
    {
        if (do_linger)
            *do_linger = l.l_onoff;
        if (timeout_seconds)
            *timeout_seconds = l.l_linger;
    }

    return sts;
}

// tcs_opt_ip_no_delay_set() is defined in tinycsocket_common.c
// tcs_opt_ip_no_delay_get() is defined in tinycsocket_common.c
// tcs_opt_out_of_band_inline_set() is defined in tinycsocket_common.c
// tcs_opt_out_of_band_inline_get() is defined in tinycsocket_common.c
// tcs_opt_priority_set() is defined in tinycsocket_common.c
// tcs_opt_priority_get() is defined in tinycsocket_common.c

TcsResult tcs_opt_nonblocking_set(TcsSocket socket_ctx, bool do_non_blocking)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    u_long mode = do_non_blocking ? 1 : 0;
    int sts = ioctlsocket((SOCKET)socket_ctx, (long)FIONBIO, &mode);
    return socketstatus2retcode(sts);
}

TcsResult tcs_opt_nonblocking_get(TcsSocket socket_ctx, bool* is_non_blocking)
{
    // Win32 does not provide an API to query the non-blocking state of a socket
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_opt_membership_add(TcsSocket socket_ctx, const struct TcsAddress* multicast_address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE address_native_local;
    memset(&address_native_local, 0, sizeof address_native_local);
    int address_native_local_size = sizeof address_native_local;
    if (getsockname((SOCKET)socket_ctx, (PSOCKADDR)&address_native_local, &address_native_local_size) != 0)
        return wsaerror2retcode(WSAGetLastError());

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    TcsResult sts = native2sockaddr((PSOCKADDR)&address_native_local, &local_address);
    if (sts != TCS_SUCCESS)
        return sts;

    if (local_address.family != multicast_address->family)
        return TCS_ERROR_INVALID_ARGUMENT;

    return tcs_opt_membership_add_to(socket_ctx, &local_address, multicast_address);
}

TcsResult tcs_opt_membership_add_to(TcsSocket socket_ctx,
                                    const struct TcsAddress* local_address,
                                    const struct TcsAddress* multicast_address)
{
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // TODO(markusl): Add ipv6 support
    if (multicast_address->family != TCS_AF_IP4)
        return TCS_ERROR_NOT_IMPLEMENTED;

    struct ip_mreq imr;
    memset(&imr, 0, sizeof imr);
    imr.imr_multiaddr.s_addr = htonl(multicast_address->data.ip4.address);
    if (local_address != NULL)
        imr.imr_interface.s_addr = htonl(local_address->data.ip4.address);

    return tcs_opt_set(socket_ctx, TCS_SOL_IP, TCS_SO_IP_MEMBERSHIP_ADD, &imr, sizeof(imr));
}

TcsResult tcs_opt_membership_drop(TcsSocket socket_ctx, const struct TcsAddress* multicast_address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE address_native_local;
    memset(&address_native_local, 0, sizeof address_native_local);
    int address_native_local_size = sizeof address_native_local;
    if (getsockname((SOCKET)socket_ctx, (PSOCKADDR)&address_native_local, &address_native_local_size) != 0)
        return wsaerror2retcode(WSAGetLastError());

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    TcsResult sts = native2sockaddr((PSOCKADDR)&address_native_local, &local_address);
    if (sts != TCS_SUCCESS)
        return sts;

    if (local_address.family != multicast_address->family)
        return TCS_ERROR_INVALID_ARGUMENT;

    return tcs_opt_membership_drop_from(socket_ctx, &local_address, multicast_address);
}

TcsResult tcs_opt_membership_drop_from(TcsSocket socket_ctx,
                                       const struct TcsAddress* local_address,
                                       const struct TcsAddress* multicast_address)
{
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // TODO(markusl): Add ipv6 support
    if (multicast_address->family != TCS_AF_IP4)
        return TCS_ERROR_NOT_IMPLEMENTED;

    struct ip_mreq imr;
    memset(&imr, 0, sizeof imr);
    imr.imr_multiaddr.s_addr = htonl(multicast_address->data.ip4.address);
    if (local_address != NULL)
        imr.imr_interface.s_addr = htonl(local_address->data.ip4.address);

    return tcs_opt_set(socket_ctx, TCS_SOL_IP, TCS_SO_IP_MEMBERSHIP_DROP, &imr, sizeof(imr));
}

// ######## Address and Interface Utilities ########

// Retrieves the user-visible adapter name.
// On Vista+ this is the FriendlyName field in IP_ADAPTER_ADDRESSES.
// On pre-Vista the FriendlyName field does not exist, so we read the registry:
//   HKLM\SYSTEM\CurrentControlSet\Control\Network\{4D36E972-E325-11CE-BFC1-08002BE10318}\<GUID>\Connection
//   Value: "Name" (REG_SZ)
// Falls back to Description (hardware name) if FriendlyName / registry is unavailable.
static void adapter_get_friendly_name(PIP_ADAPTER_ADDRESSES adapter, char* out_name, int out_name_size)
{
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
}

static bool adapter_is_up(PIP_ADAPTER_ADDRESSES adapter)
{
#if _WIN32_WINNT >= 0x0600
    return adapter->OperStatus == IfOperStatusUp;
#else
#ifndef MIB_IF_OPER_STATUS_OPERATIONAL
// Not always exposed in all mingw-w64 versions
#define MIB_IF_OPER_STATUS_OPERATIONAL 5
#endif
    MIB_IFROW ifrow;
    memset(&ifrow, 0, sizeof ifrow);
    ifrow.dwIndex = adapter->IfIndex;
    if (GetIfEntry(&ifrow) != NO_ERROR)
        return false;
    return ifrow.dwOperStatus >= MIB_IF_OPER_STATUS_OPERATIONAL;
#endif
}

TcsResult tcs_interface_list(struct TcsInterface interfaces[], size_t capacity, size_t* out_count)
{
    if (interfaces == NULL && out_count == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (interfaces == NULL && capacity != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (out_count != NULL)
        *out_count = 0;

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
        if (adapter_sts == ERROR_BUFFER_OVERFLOW)
        {
            free(adapters);
            adapters = NULL;
        }
        else
        {
            break;
        }
    }
    if (adapter_sts == ERROR_NO_DATA)
    {
        if (adapters != NULL)
            free(adapters);
        return TCS_SUCCESS;
    }
    if (adapter_sts != NO_ERROR)
    {
        // Debug: log the actual Windows error code for diagnostics
        fprintf(stderr, "GetAdaptersAddresses failed with error: %lu (0x%lX)\n", adapter_sts, adapter_sts);
        if (adapters != NULL)
            free(adapters);
        return TCS_ERROR_UNKNOWN;
    }

    if (interfaces != NULL)
    {
        size_t i = 0;
        for (PIP_ADAPTER_ADDRESSES iter = adapters; iter != NULL && i < capacity; iter = iter->Next)
        {
            if (!adapter_is_up(iter))
                continue;

            memset(interfaces[i].name, '\0', 32);
            adapter_get_friendly_name(iter, interfaces[i].name, 31);
            interfaces[i].id = iter->IfIndex;
            if (out_count != NULL)
                (*out_count)++;
            ++i;
        }
    }
    else
    {
        size_t i = 0;
        for (PIP_ADAPTER_ADDRESSES iter = adapters; iter != NULL; iter = iter->Next)
        {
            if (!adapter_is_up(iter))
                continue;
            ++i;
        }
        if (out_count != NULL)
            *out_count = i;
    }

    free(adapters);
    return TCS_SUCCESS;
}

TcsResult tcs_address_resolve(const char* hostname,
                              TcsAddressFamily address_family,
                              struct TcsAddress addresses[],
                              size_t capacity,
                              size_t* out_count)
{
    if (hostname == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (addresses == NULL && out_count == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (out_count != NULL)
        *out_count = 0;

    // Fast path: try numeric/MAC parse first to avoid DNS lookup
    struct TcsAddress parsed = TCS_ADDRESS_NONE;
    if (tcs_address_parse(hostname, &parsed) == TCS_SUCCESS && parsed.family != TCS_AF_ANY)
    {
        if (address_family == TCS_AF_ANY || parsed.family == address_family)
        {
            if (addresses != NULL && capacity > 0)
                addresses[0] = parsed;
            if (out_count != NULL)
                *out_count = 1;
            return TCS_SUCCESS;
        }
    }

    ADDRINFOA native_hints;
    memset(&native_hints, 0, sizeof native_hints);
    TcsResult sts = family2native(address_family, (short*)&native_hints.ai_family);
    if (sts != TCS_SUCCESS)
        return sts;

    PADDRINFOA native_addrinfo_list = NULL;
    int getaddrinfo_status = getaddrinfo(hostname, NULL, &native_hints, &native_addrinfo_list);
    if (getaddrinfo_status != 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    if (native_addrinfo_list == NULL)
        return TCS_ERROR_UNKNOWN;

    size_t i = 0;
    if (addresses == NULL)
    {
        for (PADDRINFOA iter = native_addrinfo_list; iter != NULL; iter = iter->ai_next)
            i++;
    }
    else
    {
        for (PADDRINFOA iter = native_addrinfo_list; iter != NULL && i < capacity; iter = iter->ai_next)
        {
            TcsResult address_convert_status = native2sockaddr(iter->ai_addr, &addresses[i]);
            if (address_convert_status != TCS_SUCCESS)
                continue;
            i++;
        }
    }
    if (out_count != NULL)
        *out_count = i;

    freeaddrinfo(native_addrinfo_list);

    if (i == 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    return TCS_SUCCESS;
}

TcsResult tcs_address_resolve_timeout(const char* hostname,
                                      TcsAddressFamily address_family,
                                      struct TcsAddress addresses[],
                                      size_t capacity,
                                      size_t* out_count,
                                      int timeout_ms)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_address_list(unsigned int interface_id_filter,
                           TcsAddressFamily address_family_filter,
                           struct TcsInterfaceAddress interface_addresses[],
                           size_t capacity,
                           size_t* out_count)
{
    if (interface_addresses == NULL && out_count == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (interface_addresses == NULL && capacity != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (out_count != NULL)
        *out_count = 0;

    const int MAX_TRIES = 5;
    ULONG buffer_size = 15000;
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
        if (adapter_sts == ERROR_BUFFER_OVERFLOW)
        {
            free(adapters);
            adapters = NULL;
        }
        else
        {
            break;
        }
    }
    if (adapter_sts == ERROR_NO_DATA)
    {
        if (adapters != NULL)
            free(adapters);
        return TCS_SUCCESS;
    }
    if (adapter_sts != NO_ERROR)
    {
        if (adapters != NULL)
            free(adapters);
        return TCS_ERROR_UNKNOWN;
    }

    size_t populated = 0;
    for (PIP_ADAPTER_ADDRESSES iter = adapters; iter != NULL; iter = iter->Next)
    {
        if (!adapter_is_up(iter))
            continue;

        if (interface_id_filter != 0 && iter->IfIndex != interface_id_filter)
            continue;

        for (PIP_ADAPTER_UNICAST_ADDRESS address_iter = iter->FirstUnicastAddress; address_iter != NULL;
             address_iter = address_iter->Next)
        {
            if (address_iter->Address.lpSockaddr == NULL)
                continue;

            if (address_family_filter != TCS_AF_ANY)
            {
                short native_family;
                TcsResult family_sts = family2native(address_family_filter, &native_family);
                if (family_sts == TCS_ERROR_NOT_IMPLEMENTED)
                    continue;
                if (family_sts != TCS_SUCCESS)
                {
                    free(adapters);
                    return family_sts;
                }
                if (address_iter->Address.lpSockaddr->sa_family != native_family)
                    continue;
            }

            struct TcsAddress address = TCS_ADDRESS_NONE;
            TcsResult convert_sts = native2sockaddr(address_iter->Address.lpSockaddr, &address);
            if (convert_sts == TCS_ERROR_NOT_IMPLEMENTED)
                continue;
            if (convert_sts != TCS_SUCCESS)
            {
                free(adapters);
                return convert_sts;
            }

            if (interface_addresses != NULL && populated < capacity)
            {
                memset(interface_addresses[populated].iface.name, '\0', 32);
                adapter_get_friendly_name(iter, interface_addresses[populated].iface.name, 31);
                interface_addresses[populated].iface.id = iter->IfIndex;
                interface_addresses[populated].address = address;
                populated++;

                if (out_count != NULL)
                    (*out_count)++;
            }
            else if (interface_addresses == NULL && out_count != NULL)
            {
                (*out_count)++;
            }
        }
    }

    free(adapters);
    return TCS_SUCCESS;
}

TcsResult tcs_address_socket_local(TcsSocket socket_ctx, struct TcsAddress* local_address)
{
    if (socket_ctx == TCS_SOCKET_INVALID || local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = sizeof native_sockaddr;
    if (getsockname((SOCKET)socket_ctx, (PSOCKADDR)&native_sockaddr, &addrlen) != 0)
        return wsaerror2retcode(WSAGetLastError());

    return native2sockaddr((PSOCKADDR)&native_sockaddr, local_address);
}

TcsResult tcs_address_socket_remote(TcsSocket socket_ctx, struct TcsAddress* remote_address)
{
    if (socket_ctx == TCS_SOCKET_INVALID || remote_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    int addrlen = sizeof native_sockaddr;
    if (getpeername((SOCKET)socket_ctx, (PSOCKADDR)&native_sockaddr, &addrlen) != 0)
        return wsaerror2retcode(WSAGetLastError());

    return native2sockaddr((PSOCKADDR)&native_sockaddr, remote_address);
}

TcsResult tcs_address_socket_family(TcsSocket socket_ctx, TcsAddressFamily* out_family)
{
    if (socket_ctx == TCS_SOCKET_INVALID || out_family == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    WSAPROTOCOL_INFOA info;
    memset(&info, 0, sizeof info);
    int info_size = sizeof info;
    if (getsockopt((SOCKET)socket_ctx, SOL_SOCKET, SO_PROTOCOL_INFOA, (char*)&info, &info_size) != 0)
        return wsaerror2retcode(WSAGetLastError());

    TcsAddressFamily family = TCS_AF_ANY;
    TcsResult sts = native2family((short)info.iAddressFamily, &family);
    if (sts != TCS_SUCCESS)
        return sts;
    *out_family = family;
    return TCS_SUCCESS;
}

// tcs_address_parse() is defined in tinycsocket_common.c
// tcs_address_to_str() is defined in tinycsocket_common.c
// tcs_address_is_equal() is defined in tinycsocket_common.c
// tcs_address_is_any() is defined in tinycsocket_common.c
// tcs_address_is_local() is defined in tinycsocket_common.c
// tcs_address_is_loopback() is defined in tinycsocket_common.c
// tcs_address_is_multicast() is defined in tinycsocket_common.c
// tcs_address_is_broadcast() is defined in tinycsocket_common.c

#endif
