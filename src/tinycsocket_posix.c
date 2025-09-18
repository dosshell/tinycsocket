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

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

// Header only should not need other files
#ifndef TINYCSOCKET_INTERNAL_H_
#include "tinycsocket_internal.h"
#endif
#ifdef TINYCSOCKET_USE_POSIX_IMPL

#ifndef TINYDATASTRUCTURES_H_
#include "tinydatastructures.h"
#endif

#ifdef DO_WRAP
#include "dbg_wrap.h"
#endif

#include <errno.h>
#include <ifaddrs.h>     // getifaddr()
#include <limits.h>      // IOV_MAX
#include <net/if.h>      // Flags for ifaddrs (?)
#include <netdb.h>       // Protocols and custom return codes
#include <netinet/in.h>  // IPPROTO_XXP
#include <netinet/tcp.h> // TCP_NODELAY
#include <poll.h>        // poll()
#include <stdlib.h>      // malloc()/free()
#include <string.h>      // strcpy, memset
#include <sys/ioctl.h>   // Flags for ifaddrs
#include <sys/socket.h>  // pretty much everything
#include <sys/types.h>   // POSIX.1 compatibility
#include <sys/uio.h>     // UIO_MAXIOV
#include <unistd.h>      // close()

// The logic might seem a bit reversed but it is to allow header only usage without defining TCS_AVAILABLE_XXX
// You need to disable the default if your system does not support it. (Optimized for most common usage and easy to detect)

// If you no not use cmake you may need to define TCS_MISSING_AF_PACKET yourself if your system does not support it
#if defined(__linux__) && !defined(TCS_MISSING_AF_PACKET) // __linux__ to disable default for header only no-linux users
#define TCS_AVAILABLE_AF_PACKET 1
#else
#define TCS_AVAILABLE_AF_PACKET 0
#endif

// If you no not use cmake you may need to define TCS_MISSING_IFADDRS yourself if your system does not support it
#if !defined(TCS_MISSING_IFADDRS)
#define TCS_AVAILABLE_IFADDRS 1
#else
#define TCS_AVAILABLE_IFADDRS 0
#endif

#if TCS_AVAILABLE_AF_PACKET
#include <linux/if_arp.h>    // sll_hatype (ethernet and not can or firewire etc.)
#include <linux/if_packet.h> // struct sockaddr_ll
#endif

#ifndef TDS_MAP_pollfd_pvoid
#define TDS_MAP_pollfd_pvoid
TDS_MAP_IMPL(struct pollfd, void*, poll)
#endif

struct TcsPool
{
    union __backend
    {
        struct __poll
        {
            struct TdsMap_poll map;
        } poll;
    } backend;
};

const TcsSocket TCS_SOCKET_INVALID = -1;
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
const uint16_t TCS_PROTOCOL_TSN = 0xF022; // htons(ETH_P_TSN)

// Flags
const uint32_t TCS_AI_PASSIVE = AI_PASSIVE;

// Recv flags
const uint32_t TCS_MSG_PEEK = MSG_PEEK;
const uint32_t TCS_MSG_OOB = MSG_OOB;
const uint32_t TCS_MSG_WAITALL = MSG_WAITALL;

// Send flags
const uint32_t TCS_MSG_SENDALL = 0x80000000;

// Backlog
const int TCS_BACKLOG_MAX = SOMAXCONN;

// Option levels
const int TCS_SOL_SOCKET = SOL_SOCKET;
const int TCS_SOL_IP = IPPROTO_IP; // Same as SOL_IP but crossplatform (BSD)

// Socket options
const int TCS_SO_BROADCAST = SO_BROADCAST;
const int TCS_SO_KEEPALIVE = SO_KEEPALIVE;
const int TCS_SO_LINGER = SO_LINGER;
const int TCS_SO_REUSEADDR = SO_REUSEADDR;
const int TCS_SO_RCVBUF = SO_RCVBUF;
const int TCS_SO_RCVTIMEO = SO_RCVTIMEO;
const int TCS_SO_SNDBUF = SO_SNDBUF;
const int TCS_SO_OOBINLINE = SO_OOBINLINE;
const int TCS_SO_PRIORITY = SO_PRIORITY;

// IP options
const int TCS_SO_IP_NODELAY = TCP_NODELAY;
const int TCS_SO_IP_MEMBERSHIP_ADD = IP_ADD_MEMBERSHIP;
const int TCS_SO_IP_MEMBERSHIP_DROP = IP_DROP_MEMBERSHIP;
const int TCS_SO_IP_MULTICAST_LOOP = IP_MULTICAST_LOOP;

#if TCS_AVAILABLE_AF_PACKET
const int TCS_SO_PACKET_MEMBERSHIP_ADD = PACKET_ADD_MEMBERSHIP;
const int TCS_SO_PACKET_MEMBERSHIP_DROP = PACKET_DROP_MEMBERSHIP;
#else
const int TCS_SO_PACKET_MEMBERSHIP_ADD = -1;
const int TCS_SO_PACKET_MEMBERSHIP_DROP = -1;
#endif

// Default flags
const int TCS_DEFAULT_SEND_FLAGS = MSG_NOSIGNAL;
const int TCS_DEFAULT_RECV_FLAGS = 0;

// ######## Internal Helpers ########

static TcsResult errno2retcode(int error_code)
{
    switch (error_code)
    {
        case EPERM:
            return TCS_ERROR_PERMISSION_DENIED;
        case ECONNREFUSED:
            return TCS_ERROR_CONNECTION_REFUSED;
        case EAGAIN:
            return TCS_ERROR_TIMED_OUT;
        case EINVAL:
            return TCS_ERROR_INVALID_ARGUMENT;
        case ENOMEM:
            return TCS_ERROR_MEMORY;
        default:
            return TCS_ERROR_UNKNOWN;
    }
}

static TcsResult family2native(const TcsAddressFamily family, sa_family_t* native_family)
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

static TcsResult sockaddr2native(const struct TcsAddress* tcs_address,
                                 struct sockaddr_storage* out_address,
                                 socklen_t* out_address_size)
{
    if (tcs_address == NULL || out_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    memset(out_address, 0, sizeof(struct sockaddr_storage));
    if (out_address_size != NULL)
        *out_address_size = 0;

    if (tcs_address->family == TCS_AF_ANY)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (tcs_address->family == TCS_AF_IP4)
    {
        struct sockaddr_in* addr = (struct sockaddr_in*)out_address;
        addr->sin_family = (sa_family_t)AF_INET;
        addr->sin_port = (in_port_t)htons(tcs_address->data.ip4.port);
        addr->sin_addr.s_addr = (in_addr_t)htonl(tcs_address->data.ip4.address);
        if (out_address_size != NULL)
            *out_address_size = sizeof(struct sockaddr_in);
        return TCS_SUCCESS;
    }
    else if (tcs_address->family == TCS_AF_IP6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (tcs_address->family == TCS_AF_PACKET)
    {
#if TCS_AVAILABLE_AF_PACKET
        struct sockaddr_ll* addr = (struct sockaddr_ll*)out_address;
        addr->sll_family = (sa_family_t)AF_PACKET;
        addr->sll_ifindex = (int)tcs_address->data.packet.interface_id;
        addr->sll_protocol = (uint16_t)htons(tcs_address->data.packet.protocol);
        addr->sll_halen = 6; // MAC address length
        memcpy(addr->sll_addr, tcs_address->data.packet.mac, 6);
        if (out_address_size != NULL)
            *out_address_size = sizeof(struct sockaddr_ll);
        return TCS_SUCCESS;
#else
        return TCS_ERROR_NOT_IMPLEMENTED;
#endif
    }

    return TCS_ERROR_NOT_IMPLEMENTED;
}

static TcsResult native2sockaddr(const struct sockaddr* in_addr, struct TcsAddress* out_addr)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->sa_family == AF_INET)
    {
        // (const void*) Supresses false positive alignment warning, the creator of the sockaddr is responsible for the alignment.
        struct sockaddr_in const* addr = (struct sockaddr_in const*)(const void*)in_addr;
        out_addr->family = TCS_AF_IP4;
        out_addr->data.ip4.port = ntohs((uint16_t)addr->sin_port);
        out_addr->data.ip4.address = ntohl((uint32_t)addr->sin_addr.s_addr);
    }
    else if (in_addr->sa_family == AF_INET6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
#if TCS_AVAILABLE_AF_PACKET
    else if (in_addr->sa_family == AF_PACKET)
    {
        struct sockaddr_ll const* addr = (struct sockaddr_ll const*)(const void*)in_addr;
        if (addr->sll_family != AF_PACKET)
            return TCS_ERROR_NOT_IMPLEMENTED;
        if (addr->sll_hatype != ARPHRD_ETHER && addr->sll_hatype != ARPHRD_LOOPBACK)
            return TCS_ERROR_NOT_IMPLEMENTED; // Not ethernet or loopback
        if (addr->sll_halen > 6)
            return TCS_ERROR_INVALID_ARGUMENT;
        if (addr->sll_ifindex < 0)
            return TCS_ERROR_INVALID_ARGUMENT;

        out_addr->family = TCS_AF_PACKET;
        out_addr->data.packet.interface_id = (unsigned int)addr->sll_ifindex;

        memcpy(out_addr->data.packet.mac, addr->sll_addr, 6);
        out_addr->data.packet.protocol = ntohs((uint16_t)addr->sll_protocol);
    }
#endif
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

// ######## Library Management ########

TcsResult tcs_lib_init(void)
{
    // Not needed for posix
    return TCS_SUCCESS;
}

TcsResult tcs_lib_free(void)
{
    // Not needed for posix
    return TCS_SUCCESS;
}

// ######## Socket Creation ########

TcsResult tcs_socket(TcsSocket* socket_ctx, TcsAddressFamily family, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    sa_family_t native_family;
    TcsResult sts = family2native(family, &native_family);
    if (sts != TCS_SUCCESS)
        return sts;
    *socket_ctx = socket(native_family, type, protocol);

    if (*socket_ctx != -1) // Same as TCS_NULLSOCKET
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

// tcs_socket_preset() is defined in tinycsocket_common.c

TcsResult tcs_close(TcsSocket* socket_ctx)
{
    if (socket_ctx == NULL || *socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (close(*socket_ctx) == 0)
    {
        *socket_ctx = TCS_SOCKET_INVALID;
        return TCS_SUCCESS;
    }
    else
    {
        return errno2retcode(errno);
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

// tcs_packet_sender_str() is defined in tinycsocket_common.c
// tcs_packet_sender() is defined in tinycsocket_common.c
// tcs_packet_peer_str() is defined in tinycsocket_common.c
// tcs_packet_peer() is defined in tinycsocket_common.c
// tcs_packet_capture_iface() is defined in tinycsocket_common.c
// tcs_packet_capture_ifname() is defined in tinycsocket_common.c

// ######## Socket Operations ########

TcsResult tcs_bind(TcsSocket socket_ctx, const struct TcsAddress* address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    socklen_t sockaddr_size = 0;
    TcsResult convert_address_status = sockaddr2native(address, &native_sockaddr, &sockaddr_size);
    if (convert_address_status != TCS_SUCCESS)
        return convert_address_status;

    if (bind(socket_ctx, (struct sockaddr*)&native_sockaddr, sockaddr_size) != -1)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsResult tcs_connect(TcsSocket socket_ctx, const struct TcsAddress* address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    socklen_t sockaddr_size = 0;
    TcsResult convert_address_status = sockaddr2native(address, &native_sockaddr, &sockaddr_size);
    if (convert_address_status != TCS_SUCCESS)
        return convert_address_status;

    if (connect(socket_ctx, (const struct sockaddr*)&native_sockaddr, sockaddr_size) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

// tcs_connect_str() is defined in tinycsocket_common.c

TcsResult tcs_listen(TcsSocket socket_ctx, int backlog)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (backlog < 1 || backlog > TCS_BACKLOG_MAX)
        backlog = TCS_BACKLOG_MAX;

    if (listen(socket_ctx, backlog) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsResult tcs_accept(TcsSocket socket_ctx, TcsSocket* child_socket_ctx, struct TcsAddress* address)
{
    if (socket_ctx == TCS_SOCKET_INVALID || child_socket_ctx == NULL || *child_socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (address != NULL)
        *address = TCS_ADDRESS_NONE;

    struct sockaddr_storage native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    socklen_t sockaddr_size = sizeof native_sockaddr;

    *child_socket_ctx = accept(socket_ctx, (struct sockaddr*)&native_sockaddr, &sockaddr_size);
    if (*child_socket_ctx != -1)
    {
        if (address != NULL)
        {
            TcsResult convert_address_status = native2sockaddr((struct sockaddr*)&native_sockaddr, address);
            if (convert_address_status != TCS_SUCCESS)
                return convert_address_status;
        }
        return TCS_SUCCESS;
    }
    else
    {
        *child_socket_ctx = TCS_SOCKET_INVALID;
        return errno2retcode(errno);
    }
}

TcsResult tcs_shutdown(TcsSocket socket_ctx, TcsSocketDirection direction)
{
    const int LUT[] = {SHUT_RD, SHUT_WR, SHUT_RDWR};

    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    const int how = LUT[direction];
    if (shutdown(socket_ctx, how) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
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
        ssize_t send_status = send(socket_ctx, (const char*)buffer, buffer_size, TCS_DEFAULT_SEND_FLAGS | (int)flags);
        if (send_status >= 0)
        {
            if (bytes_sent != NULL)
                *bytes_sent = (size_t)send_status;
            return TCS_SUCCESS;
        }
        else
        {
            if (bytes_sent != NULL)
                *bytes_sent = 0;
            return errno2retcode(errno);
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

    struct sockaddr_storage native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    socklen_t sockaddr_size = 0;
    TcsResult convert_addr_status = sockaddr2native(destination_address, &native_sockaddr, &sockaddr_size);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;

    ssize_t sendto_status = sendto(socket_ctx,
                                   (const char*)buffer,
                                   buffer_size,
                                   TCS_DEFAULT_SEND_FLAGS | (int)flags,
                                   (const struct sockaddr*)&native_sockaddr,
                                   sockaddr_size);

    if (sendto_status >= 0)
    {
        if (bytes_sent != NULL)
            *bytes_sent = (size_t)sendto_status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return errno2retcode(errno);
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

    // TCS_SENDV_MAX is default set to 1024. define TCS_SMALL_STACK to use a value of 128.
    const size_t max_supported_iov =
        UIO_MAXIOV > TCS_SENDV_MAX ? TCS_SENDV_MAX : UIO_MAXIOV; // min(TCS_SENDV_MAX, UIO_MAXIOV)
    if (buffer_count > max_supported_iov)
        return TCS_ERROR_INVALID_ARGUMENT;

    // Some plattforms (Glibc) do not follow posix. We have mixed size_t and int types for msg_iovlen.
    // We cast it to a narrower unsigned type to avoid warnings for later assignment.
    // If you read this and doesn't like it, you can use probably use the compiler extension typeof() instead
    // to cast it to correct type without warnigns. We can not, since we want to support more compilers.

// Check if buffer_count can be placed in an unsigned short
#if (UIO_MAXIOV > USHRT_MAX && TCS_SENDV_MAX > USHRT_MAX)
    // You are using a plattform with very narrow unsigned short. Let's hope that your plattform follows POSIX standards here.
    typedef int SAFE_IOVLEN;
#else
    typedef unsigned short SAFE_IOVLEN_TYPE;
#endif

    SAFE_IOVLEN_TYPE narrow_casted_iovlen = (SAFE_IOVLEN_TYPE)buffer_count;

    static struct iovec my_iovec[TCS_SENDV_MAX];
    for (size_t i = 0; i < buffer_count; i++)
    {
        // We know that sendmsg() does not modify the data, so we can safely cast away the const here.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
        my_iovec[i].iov_base = (void*)buffers[i].data;
#pragma GCC diagnostic pop
        my_iovec[i].iov_len = buffers[i].size;
    }

    struct msghdr msg;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = my_iovec;
    msg.msg_iovlen = narrow_casted_iovlen; // msg_iovlen is size_t or int type.
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    ssize_t ret = 0;
    ret = sendmsg(socket_ctx, &msg, TCS_DEFAULT_SEND_FLAGS | (int)flags);

    if (ret >= 0)
    {
        if (bytes_sent != NULL)
            *bytes_sent = (size_t)ret;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;
        return errno2retcode(errno);
    }
}

// tcs_send_netstring() is defined in tinycsocket_common.c

TcsResult tcs_receive(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_received)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    ssize_t recv_status = recv(socket_ctx, (char*)buffer, buffer_size, TCS_DEFAULT_RECV_FLAGS | (int)flags);

    if (recv_status > 0)
    {
        if (bytes_received != NULL)
            *bytes_received = (size_t)recv_status;
        return TCS_SUCCESS;
    }
    else if (recv_status == 0)
    {
        if (bytes_received != NULL)
            *bytes_received = 0;
        return TCS_ERROR_NOT_CONNECTED; // TODO: think about this
    }
    else
    {
        if (bytes_received != NULL)
            *bytes_received = 0;
        return errno2retcode(errno);
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

    struct sockaddr_storage native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    socklen_t addrlen = sizeof native_sockaddr;

    ssize_t recvfrom_status = recvfrom(socket_ctx,
                                       (char*)buffer,
                                       buffer_size,
                                       TCS_DEFAULT_RECV_FLAGS | (int)flags,
                                       (struct sockaddr*)&native_sockaddr,
                                       &addrlen);

    if (recvfrom_status > 0)
    {
        if (bytes_received != NULL)
            *bytes_received = (size_t)recvfrom_status;
        if (source_address != NULL)
            return native2sockaddr((struct sockaddr*)&native_sockaddr, source_address);
        return TCS_SUCCESS;
    }
    else if (recvfrom_status == 0)
    {
        if (bytes_received != NULL)
            *bytes_received = 0;
        return TCS_ERROR_SOCKET_CLOSED; // TODO: think about this
    }
    else
    {
        if (bytes_received != NULL)
            *bytes_received = 0;
        return errno2retcode(errno);
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
    memset(*pool, 0, sizeof(struct TcsPool));

    if (tds_map_poll_create(&(*pool)->backend.poll.map) != 0)
    {
        free(*pool);
        *pool = NULL;
        return TCS_ERROR_MEMORY;
    }

    return TCS_SUCCESS;
}

TcsResult tcs_pool_destroy(struct TcsPool** pool)
{
    if (pool == NULL || *pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (tds_map_poll_destroy(&(*pool)->backend.poll.map) != 0)
    {
        // Should not happen, but if it does, we may leak memory.
        // We can not do anything about it.
        return TCS_ERROR_MEMORY;
    }

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
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    // todo(markusl): Add more events that is input and output events
    short ev = 0;
    if (poll_can_read)
        ev |= POLLIN;
    if (poll_can_write)
        ev |= POLLOUT;
    if (poll_error)
        ev |= POLLERR;

    struct pollfd pfd;
    pfd.fd = socket_ctx;
    pfd.events = ev;
    pfd.revents = 0;

    if (tds_map_poll_addp(&pool->backend.poll.map, &pfd, &user_data) != 0)
        return TCS_ERROR_MEMORY;

    return TCS_SUCCESS;
}

TcsResult tcs_pool_remove(struct TcsPool* pool, TcsSocket socket_ctx)
{
    if (pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    struct TdsMap_poll const* map = &pool->backend.poll.map;

    bool found = false;
    for (size_t i = 0; i < map->count; ++i)
    {
        if (socket_ctx == map->keys[i].fd)
        {
            if (tds_map_poll_remove(&pool->backend.poll.map, i) != 0)
                return TCS_ERROR_MEMORY;
            found = true;
            break;
        }
    }
    if (!found)
        return TCS_ERROR_INVALID_ARGUMENT;

    return TCS_SUCCESS;
}

TcsResult tcs_pool_poll(struct TcsPool* pool,
                        struct TcsPollEvent* events,
                        size_t events_count,
                        size_t* events_populated,
                        int64_t timeout_in_ms)
{
    if (pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (events_populated == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // We do not support more more elements or time than signed int32 supports.
    // todo(markusl): Add support for int64 timeout
    if (events_count > 0x7FFFFFFF || timeout_in_ms > 0x7FFFFFFF)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TdsMap_poll* map = &pool->backend.poll.map;

    int poll_ret = poll(map->keys, map->count, (int)timeout_in_ms);
    *events_populated = 0;
    if (poll_ret < 0)
    {
        return errno2retcode(errno);
    }
    if ((size_t)poll_ret > map->count)
    {
        return TCS_ERROR_UNKNOWN; // Corruption
    }

    int fill_max = poll_ret > (int)events_count ? (int)events_count : poll_ret; // min(ret, events_count)
    int filled = 0;
    for (size_t i = 0; filled < fill_max; ++i)
    {
        if (i >= map->count)
            return TCS_ERROR_UNKNOWN;

        if (map->keys[i].revents != 0)
        {
            events[filled].socket = map->keys[i].fd;
            events[filled].user_data = map->values[i];
            events[filled].can_read = map->keys[i].revents & POLLIN;
            events[filled].can_write = map->keys[i].revents & POLLOUT;
            events[filled].error = (map->keys[i].revents & POLLERR) == 0
                                       ? TCS_SUCCESS
                                       : TCS_ERROR_NOT_IMPLEMENTED; // TODO: implement error codes
            map->keys[i].revents = 0;                               // Reset revents
            ++filled;
        }
    }
    *events_populated = (size_t)filled;

    if (poll_ret == 0)
        return TCS_ERROR_TIMED_OUT;
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

    if (setsockopt(socket_ctx, (int)level, (int)option_name, (const char*)option_value, (socklen_t)option_size) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsResult tcs_opt_get(TcsSocket socket_ctx, int32_t level, int32_t option_name, void* option_value, size_t* option_size)
{
    if (socket_ctx == TCS_SOCKET_INVALID || option_value == NULL || option_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (getsockopt(socket_ctx, (int)level, (int)option_name, (void*)option_value, (socklen_t*)option_size) == 0)
    {
        // Linux sets the buffer size to the doubled because of internal use and returns the full doubled size including internal part
#ifdef __linux__
        if (option_name == TCS_SO_RCVBUF || option_name == TCS_SO_SNDBUF)
        {
            *(unsigned int*)option_value /= 2;
        }
#endif
        return TCS_SUCCESS;
    }
    else
    {
        return errno2retcode(errno);
    }
}

// tcs_opt_broadcast_set() is defined in tinycocket_common.c
// tcs_opt_broadcast_get() is defined in tinycocket_common.c
// tcs_opt_keep_alive_set() is defined in inycocket_common.c
// tcs_opt_keep_alive_get() is defined in tinycocket_common.c
// tcs_opt_reuse_address_set() is defined in tiinycocket_common.c
// tcs_opt_reuse_address_get() is defined in tiinycocket_common.c
// tcs_opt_send_buffer_size_set() is defined in tininycocket_common.c
// tcs_opt_send_buffer_size_get() is defined in tininycocket_common.c
// tcs_opt_receive_buffer_size_set() is defined in tinyinycocket_common.c
// tcs_opt_receive_buffer_size_get() is defined in tinyinycocket_common.c

TcsResult tcs_opt_receive_timeout_set(TcsSocket socket_ctx, int timeout_ms)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    return tcs_opt_set(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &tv, sizeof(tv));
}

TcsResult tcs_opt_receive_timeout_get(TcsSocket socket_ctx, int* timeout_ms)
{
    if (socket_ctx == TCS_SOCKET_INVALID || timeout_ms == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct timeval tv = {0, 0};
    size_t tv_size = sizeof(tv);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &tv, &tv_size);

    if (sts == TCS_SUCCESS)
    {
        int c = 0;
        c += (int)tv.tv_sec * 1000;
        c += (int)tv.tv_usec / 1000;
        *timeout_ms = c;
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
// tcs_opt_nonblocking_set() is defined in tinycsocket_common.c
// tcs_opt_nonblocking_get() is defined in tinycsocket_common.c

TcsResult tcs_opt_membership_add(TcsSocket socket_ctx, const struct TcsAddress* multicast_address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // Todo: Replace with tcs_address_socket() when implemented
    struct sockaddr_storage address_native_local;
    memset(&address_native_local, 0, sizeof address_native_local);
    socklen_t address_native_local_size = 0;
    if (getsockname(socket_ctx, (struct sockaddr*)&address_native_local, &address_native_local_size) != 0)
        return errno2retcode(errno);

    if (address_native_local.ss_family != multicast_address->family)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    TcsResult sts = native2sockaddr((struct sockaddr*)&address_native_local, &local_address);
    if (sts != TCS_SUCCESS)
        return sts;

    return tcs_opt_membership_add_to(socket_ctx, &local_address, multicast_address);
}

TcsResult tcs_opt_membership_add_to(TcsSocket socket_ctx,
                                    const struct TcsAddress* local_address,
                                    const struct TcsAddress* multicast_address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (local_address->family != multicast_address->family)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage local_address_native;
    memset(&local_address_native, 0, sizeof local_address_native);
    socklen_t local_address_native_size = 0;
    TcsResult sts_la2n = sockaddr2native(local_address, &local_address_native, &local_address_native_size);
    if (sts_la2n != TCS_SUCCESS)
        return sts_la2n;

    struct sockaddr_storage multicast_address_native;
    memset(&multicast_address_native, 0, sizeof multicast_address_native);
    socklen_t multicast_address_native_size = 0;
    TcsResult sts_ma2n = sockaddr2native(multicast_address, &multicast_address_native, &multicast_address_native_size);
    if (sts_ma2n != TCS_SUCCESS)
        return sts_ma2n;

    if (multicast_address->family == TCS_AF_IP4)
    {
        struct sockaddr_in* address_native_local_p = (struct sockaddr_in*)&local_address_native;
        struct sockaddr_in* address_native_multicast_p = (struct sockaddr_in*)&multicast_address_native;

        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof mreq);

        mreq.imr_interface.s_addr = address_native_local_p->sin_addr.s_addr;
        mreq.imr_multiaddr.s_addr = address_native_multicast_p->sin_addr.s_addr;

        TcsResult sts_opt = tcs_opt_set(socket_ctx, TCS_SOL_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        if (sts_opt != TCS_SUCCESS)
            return sts_opt;
        return TCS_SUCCESS;
    }
    else if (multicast_address->family == TCS_AF_IP6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (multicast_address->family == TCS_AF_PACKET)
    {
#if TCS_AVAILABLE_AF_PACKET
        struct sockaddr_ll* address_native_local_p = (struct sockaddr_ll*)&local_address_native;
        struct sockaddr_ll* address_native_multicast_p = (struct sockaddr_ll*)&multicast_address_native;

        struct packet_mreq mreq;
        memset(&mreq, 0, sizeof mreq);
        mreq.mr_ifindex = address_native_local_p->sll_ifindex;
        mreq.mr_type = PACKET_MR_MULTICAST;
        memcpy(mreq.mr_address, address_native_multicast_p->sll_addr, ETH_ALEN);
        mreq.mr_alen = ETH_ALEN;

        TcsResult sts_opt = tcs_opt_set(socket_ctx, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        if (sts_opt != TCS_SUCCESS)
            return sts_opt;
        return TCS_SUCCESS;
#else
        return TCS_ERROR_NOT_IMPLEMENTED;
#endif
    }
    else
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }

    return TCS_ERROR_UNKNOWN; // Not reachable
}

TcsResult tcs_opt_membership_drop(TcsSocket socket_ctx, const struct TcsAddress* multicast_address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // Todo: Replace with tcs_address_socket() when implemented
    struct sockaddr_storage address_native_local;
    memset(&address_native_local, 0, sizeof address_native_local);
    socklen_t address_native_local_size = 0;
    if (getsockname(socket_ctx, (struct sockaddr*)&address_native_local, &address_native_local_size) != 0)
        return errno2retcode(errno);

    if (address_native_local.ss_family != multicast_address->family)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    TcsResult sts = native2sockaddr((struct sockaddr*)&address_native_local, &local_address);
    if (sts != TCS_SUCCESS)
        return sts;

    return tcs_opt_membership_drop_from(socket_ctx, &local_address, multicast_address);
}

TcsResult tcs_opt_membership_drop_from(TcsSocket socket_ctx,
                                       const struct TcsAddress* local_address,
                                       const struct TcsAddress* multicast_address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (local_address->family != multicast_address->family)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage local_address_native;
    memset(&local_address_native, 0, sizeof local_address_native);
    socklen_t local_address_native_size = 0;
    TcsResult sts_la2n = sockaddr2native(local_address, &local_address_native, &local_address_native_size);
    if (sts_la2n != TCS_SUCCESS)
        return sts_la2n;

    struct sockaddr_storage multicast_address_native;
    memset(&multicast_address_native, 0, sizeof multicast_address_native);
    socklen_t multicast_address_native_size = 0;
    TcsResult sts_ma2n = sockaddr2native(multicast_address, &multicast_address_native, &multicast_address_native_size);
    if (sts_ma2n != TCS_SUCCESS)
        return sts_ma2n;

    if (multicast_address->family == TCS_AF_IP4)
    {
        struct sockaddr_in* address_native_local_p = (struct sockaddr_in*)&local_address_native;
        struct sockaddr_in* address_native_multicast_p = (struct sockaddr_in*)&multicast_address_native;

        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof mreq);

        mreq.imr_interface.s_addr = address_native_local_p->sin_addr.s_addr;
        mreq.imr_multiaddr.s_addr = address_native_multicast_p->sin_addr.s_addr;

        TcsResult sts_opt = tcs_opt_set(socket_ctx, TCS_SOL_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        if (sts_opt != TCS_SUCCESS)
            return sts_opt;
        return TCS_SUCCESS;
    }
    else if (multicast_address->family == TCS_AF_IP6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (multicast_address->family == TCS_AF_PACKET)
    {
#if TCS_AVAILABLE_AF_PACKET
        struct sockaddr_ll* address_native_local_p = (struct sockaddr_ll*)&local_address_native;
        struct sockaddr_ll* address_native_multicast_p = (struct sockaddr_ll*)&multicast_address_native;

        struct packet_mreq mreq;
        memset(&mreq, 0, sizeof mreq);
        mreq.mr_ifindex = address_native_local_p->sll_ifindex;
        mreq.mr_type = PACKET_MR_MULTICAST;
        memcpy(mreq.mr_address, address_native_multicast_p->sll_addr, ETH_ALEN);
        mreq.mr_alen = ETH_ALEN;

        TcsResult sts_opt = tcs_opt_set(socket_ctx, SOL_PACKET, PACKET_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        if (sts_opt != TCS_SUCCESS)
            return sts_opt;
        return TCS_SUCCESS;
#else
        return TCS_ERROR_NOT_IMPLEMENTED;
#endif
    }
    else
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }

    return TCS_ERROR_UNKNOWN; // Not reachable
}

// ######## Address and Interface Utilities ########

TcsResult tcs_interface_list(struct TcsInterface* found_interfaces,
                             size_t interfaces_length,
                             size_t* interfaces_populated)
{
    if (found_interfaces == NULL && interfaces_populated == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (found_interfaces == NULL && interfaces_length != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (interfaces_populated != NULL)
        *interfaces_populated = 0;

    struct if_nameindex* interfaces = if_nameindex();
    if (interfaces == NULL)
        return errno2retcode(errno);

    if (found_interfaces != NULL)
    {
        for (size_t i = 0; i < interfaces_length && interfaces[i].if_index != 0; ++i)
        {
            strncpy(found_interfaces[i].name, interfaces[i].if_name, 31);
            found_interfaces[i].name[31] = '\0';
            found_interfaces[i].id = interfaces[i].if_index;
            if (interfaces_populated != NULL)
                *interfaces_populated += 1;
        }
    }
    else // found_interfaces == NULL && interface_populated != NULL
    {
        for (size_t i = 0; interfaces[i].if_index != 0; ++i)
            *interfaces_populated += 1;
    }

    if_freenameindex(interfaces);
    return TCS_SUCCESS;
}

TcsResult tcs_address_resolve(const char* hostname,
                              TcsAddressFamily address_family,
                              struct TcsAddress found_addresses[],
                              size_t found_addresses_length,
                              size_t* no_of_found_addresses)
{
    if (hostname == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (found_addresses == NULL && no_of_found_addresses == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (no_of_found_addresses != NULL)
        *no_of_found_addresses = 0;

    struct addrinfo native_hints;
    memset(&native_hints, 0, sizeof native_hints);
    TcsResult family_convert_status = family2native(address_family, (sa_family_t*)&native_hints.ai_family);
    if (family_convert_status != TCS_SUCCESS)
        return family_convert_status;

    struct addrinfo* native_addrinfo_list = NULL;
    int sts = getaddrinfo(hostname, NULL, &native_hints, &native_addrinfo_list);
    if (sts == EAI_SYSTEM)
        return errno2retcode(errno);
    else if (native_addrinfo_list == NULL)
        return TCS_ERROR_UNKNOWN;
    else if (sts != 0)
        return TCS_ERROR_UNKNOWN;

    size_t i = 0;
    if (found_addresses == NULL)
    {
        for (struct addrinfo* iter = native_addrinfo_list; iter != NULL; iter = iter->ai_next)
            i++;
    }
    else
    {
        for (struct addrinfo* iter = native_addrinfo_list; iter != NULL && i < found_addresses_length;
             iter = iter->ai_next)
        {
            TcsResult convert_address_status = native2sockaddr(iter->ai_addr, &found_addresses[i]);
            if (convert_address_status != TCS_SUCCESS)
                continue;
            i++;
        }
    }
    if (no_of_found_addresses != NULL)
        *no_of_found_addresses = i;

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

#if TCS_AVAILABLE_IFADDRS // acquired from CMake
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

    struct ifaddrs* ifap = NULL;
    if (getifaddrs(&ifap) == -1)
    {
        if (ifap != NULL)
            freeifaddrs(ifap);
        return errno2retcode(errno);
    }

    if (ifap == NULL)
        return TCS_ERROR_UNKNOWN;

    size_t populated = 0;
    for (struct ifaddrs* iter = ifap; iter != NULL; iter = iter->ifa_next)
    {
        if (iter->ifa_addr == NULL)
            continue;

        if (interface_id_filter != 0)
        {
            unsigned int id = if_nametoindex(iter->ifa_name);
            if (id == 0)
            {
                freeifaddrs(ifap);
                return errno2retcode(errno);
            }
            if (id != interface_id_filter)
                continue;
        }
        if (address_family_filter != TCS_AF_ANY)
        {
            sa_family_t native_family;
            TcsResult family_convert_status = family2native(address_family_filter, &native_family);
            if (family_convert_status == TCS_ERROR_NOT_IMPLEMENTED)
                continue;
            if (family_convert_status != TCS_SUCCESS)
            {
                freeifaddrs(ifap);
                return family_convert_status;
            }
            if (iter->ifa_addr->sa_family != native_family)
                continue;
        }

        struct TcsAddress address = TCS_ADDRESS_NONE;
        TcsResult convert_address_status = native2sockaddr(iter->ifa_addr, &address);
        if (convert_address_status == TCS_ERROR_NOT_IMPLEMENTED)
            continue;
        if (convert_address_status != TCS_SUCCESS)
        {
            freeifaddrs(ifap);
            return convert_address_status;
        }

        if (interface_addresses != NULL && populated < capacity)
        {
            unsigned int interface_id = if_nametoindex(iter->ifa_name);
            if (interface_id == 0)
            {
                freeifaddrs(ifap);
                return errno2retcode(errno);
            }

            strncpy(interface_addresses[populated].iface.name, iter->ifa_name, 31);
            interface_addresses[populated].iface.name[31] = '\0';
            interface_addresses[populated].iface.id = interface_id;
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

    freeifaddrs(ifap);
    return TCS_SUCCESS;
}
#else
// SunOS before 2010, HP and AIX does not support getifaddrs
// ioctl implementation,
// https://stackoverflow.com/questions/4139405/how-can-i-get-to-know-the-ip-address-for-interfaces-in-c/4139811#4139811
TcsResult tcs_address_list(unsigned int interface_id_filter,
                           TcsAddressFamily address_family_filter,
                           struct TcsInterfaceAddress interface_addresses[],
                           size_t capacity,
                           size_t* out_count)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}
#endif

TcsResult tcs_address_socket_local(TcsSocket socket_ctx, struct TcsAddress* local_address)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_address_socket_remote(TcsSocket socket_ctx, struct TcsAddress* remote_address)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_address_socket_family(TcsSocket socket_ctx, TcsAddressFamily* out_family)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
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
