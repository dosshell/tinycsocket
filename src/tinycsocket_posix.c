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

#include "tinycsocket.h"

#ifdef TINYCSOCKET_USE_POSIX_IMPL

#include <errno.h>
#include <ifaddrs.h>     // getifaddr()
#include <net/if.h>      // Flags for ifaddrs (?)
#include <netdb.h>       // Protocols and custom return codes
#include <netinet/in.h>  // IPPROTO_XXP
#include <netinet/tcp.h> // TCP_NODELAY
#include <string.h>      // strcpy
#include <sys/ioctl.h>   // Flags for ifaddrs
#include <sys/socket.h>  // pretty much everything
#include <sys/types.h>   // POSIX.1 compatibility
#include <unistd.h>      // close()

const TcsSocket TCS_NULLSOCKET = -1;

// Type
const int TCS_SOCK_STREAM = SOCK_STREAM;
const int TCS_SOCK_DGRAM = SOCK_DGRAM;

// Protocol
const int TCS_IPPROTO_TCP = IPPROTO_TCP;
const int TCS_IPPROTO_UDP = IPPROTO_UDP;

// Flags
const uint32_t TCS_AI_PASSIVE = AI_PASSIVE;

// Recv flags
const int TCS_MSG_PEEK = MSG_PEEK;
const int TCS_MSG_OOB = MSG_OOB;

// Backlog
const int TCS_BACKLOG_SOMAXCONN = SOMAXCONN;

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

// IP options
const int TCS_SO_IP_NODELAY = TCP_NODELAY;
const int TCS_SO_IP_MEMBERSHIP_ADD = IP_ADD_MEMBERSHIP;
const int TCS_SO_IP_MEMBERSHIP_DROP = IP_DROP_MEMBERSHIP;
const int TCS_SO_IP_MULTICAST_LOOP = IP_MULTICAST_LOOP;

static TcsReturnCode family2native(const TcsAddressFamily family, sa_family_t* native_family)
{
    static uint16_t lut[TCS_AF_LENGTH] = {AF_UNSPEC, AF_INET, AF_INET6};
    if (native_family == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (family >= TCS_AF_LENGTH || family < 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    *native_family = lut[family];
    return TCS_SUCCESS;
}

static TcsReturnCode errno2retcode(int error_code)
{
    switch (error_code)
    {
        case ECONNREFUSED:
            return TCS_ERROR_CONNECTION_REFUSED;
        default:
            return TCS_ERROR_UNKNOWN;
    }
}

static TcsReturnCode sockaddr2native(const struct TcsAddress* in_addr,
                                     struct sockaddr* out_addr,
                                     socklen_t* out_addrlen)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->family == TCS_AF_IP4)
    {
        struct sockaddr_in* addr = (struct sockaddr_in*)out_addr;
        addr->sin_family = (sa_family_t)AF_INET;
        addr->sin_port = (in_port_t)htons(in_addr->data.af_inet.port);
        addr->sin_addr.s_addr = (in_addr_t)htonl(in_addr->data.af_inet.address);

        if (out_addrlen != NULL)
            *out_addrlen = sizeof(struct sockaddr_in);

        return TCS_SUCCESS;
    }
    else if (in_addr->family == TCS_AF_IP6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (in_addr->family == TCS_AF_ANY)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    return TCS_ERROR_NOT_IMPLEMENTED;
}

static TcsReturnCode native2sockaddr(const struct sockaddr* in_addr, struct TcsAddress* out_addr)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->sa_family == AF_INET)
    {
        struct sockaddr_in* addr = (struct sockaddr_in*)in_addr;
        out_addr->family = TCS_AF_IP4;
        out_addr->data.af_inet.port = ntohs((uint16_t)addr->sin_port);
        out_addr->data.af_inet.address = ntohl((uint32_t)addr->sin_addr.s_addr);
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

TcsReturnCode tcs_lib_init()
{
    // Not needed for posix
    return TCS_SUCCESS;
}

TcsReturnCode tcs_lib_free()
{
    // Not needed for posix
    return TCS_SUCCESS;
}

TcsReturnCode tcs_create_ext(TcsSocket* socket_ctx, TcsAddressFamily family, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;
    sa_family_t native_family;
    TcsReturnCode sts = family2native(family, &native_family);
    if (sts != TCS_SUCCESS)
        return sts;
    *socket_ctx = socket(native_family, type, protocol);

    if (*socket_ctx != -1) // Same as TCS_NULLSOCKET
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_bind(TcsSocket socket_ctx, const struct TcsAddress* address)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr = {0};
    socklen_t addrlen = 0;
    TcsReturnCode convert_address_status = sockaddr2native(address, (struct sockaddr*)&native_sockaddr, &addrlen);
    if (convert_address_status != TCS_SUCCESS)
        return convert_address_status;
    if (bind(socket_ctx, (struct sockaddr*)&native_sockaddr, addrlen) != -1)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_connect(TcsSocket socket_ctx, const struct TcsAddress* address)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr = {0};
    socklen_t address_length = 0;
    TcsReturnCode convert_address_status =
        sockaddr2native(address, (struct sockaddr*)&native_sockaddr, &address_length);
    if (convert_address_status != TCS_SUCCESS)
        return convert_address_status;

    if (connect(socket_ctx, (const struct sockaddr*)&native_sockaddr, address_length) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_listen(TcsSocket socket_ctx, int backlog)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (listen(socket_ctx, backlog) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_accept(TcsSocket socket_ctx, TcsSocket* child_socket_ctx, struct TcsAddress* address)
{
    if (socket_ctx == TCS_NULLSOCKET || child_socket_ctx == NULL || *child_socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_socket_address = {0};
    socklen_t address_length = sizeof(native_socket_address);

    *child_socket_ctx = accept(socket_ctx, (struct sockaddr*)&native_socket_address, &address_length);
    if (*child_socket_ctx != -1)
    {
        if (address != NULL)
        {
            TcsReturnCode convert_address_status = native2sockaddr((struct sockaddr*)&native_socket_address, address);
            if (convert_address_status != TCS_SUCCESS)
                return convert_address_status;
        }
        return TCS_SUCCESS;
    }
    else
    {
        *child_socket_ctx = TCS_NULLSOCKET;
        return errno2retcode(errno);
    }
}

TcsReturnCode tcs_send(TcsSocket socket_ctx,
                       const uint8_t* buffer,
                       size_t buffer_size,
                       uint32_t flags,
                       size_t* bytes_sent)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    ssize_t send_status = send(socket_ctx, (const char*)buffer, buffer_size, (int)flags);
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

TcsReturnCode tcs_send_to(TcsSocket socket_ctx,
                          const uint8_t* buffer,
                          size_t buffer_size,
                          uint32_t flags,
                          const struct TcsAddress* destination_address,
                          size_t* bytes_sent)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr = {0};
    socklen_t address_length = 0;
    TcsReturnCode convert_addr_status =
        sockaddr2native(destination_address, (struct sockaddr*)&native_sockaddr, &address_length);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;

    ssize_t sendto_status = sendto(socket_ctx,
                                   (const char*)buffer,
                                   buffer_size,
                                   (int)flags,
                                   (const struct sockaddr*)&native_sockaddr,
                                   (socklen_t)address_length);

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

TcsReturnCode tcs_receive(TcsSocket socket_ctx,
                          uint8_t* buffer,
                          size_t buffer_size,
                          uint32_t flags,
                          size_t* bytes_received)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    ssize_t recv_status = recv(socket_ctx, (char*)buffer, buffer_size, (int)flags);

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

TcsReturnCode tcs_receive_from(TcsSocket socket_ctx,
                               uint8_t* buffer,
                               size_t buffer_size,
                               uint32_t flags,
                               struct TcsAddress* source_address,
                               size_t* bytes_received)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr = {0};
    socklen_t addrlen = sizeof(native_sockaddr);

    ssize_t recvfrom_status = recvfrom(
        socket_ctx, (char*)buffer, buffer_size, (int)flags, (struct sockaddr*)&native_sockaddr, (socklen_t*)&addrlen);

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

TcsReturnCode tcs_set_option(TcsSocket socket_ctx,
                             int32_t level,
                             int32_t option_name,
                             const void* option_value,
                             size_t option_size)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (setsockopt(socket_ctx, (int)level, (int)option_name, (const char*)option_value, (socklen_t)option_size) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_get_option(TcsSocket socket_ctx,
                             int32_t level,
                             int32_t option_name,
                             void* option_value,
                             size_t* option_size)
{
    if (socket_ctx == TCS_NULLSOCKET || option_value == NULL || option_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (getsockopt(socket_ctx, (int)level, (int)option_name, (void*)option_value, (socklen_t*)option_size) == 0)
    {
        // Linux sets the buffer size to the doubled beacuse of internal use and returns the full doubled size including internal part
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

TcsReturnCode tcs_shutdown(TcsSocket socket_ctx, TcsSocketDirection direction)
{
    const int LUT[] = {SHUT_RD, SHUT_WR, SHUT_RDWR};

    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    const int how = LUT[direction];
    if (shutdown(socket_ctx, how) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_destroy(TcsSocket* socket_ctx)
{
    if (socket_ctx == NULL || *socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (close(*socket_ctx) == 0)
    {
        *socket_ctx = TCS_NULLSOCKET;
        return TCS_SUCCESS;
    }
    else
    {
        return errno2retcode(errno);
    }
}

TcsReturnCode tcs_get_addresses(const char* node,
                                const char* service,
                                TcsAddressFamily address_family,
                                struct TcsAddress found_addresses[],
                                size_t found_addresses_length,
                                size_t* no_of_found_addresses)
{
    if (node == NULL && service == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (found_addresses == NULL && no_of_found_addresses == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (no_of_found_addresses != NULL)
        *no_of_found_addresses = 0;

    struct addrinfo native_hints = {0};
    TcsReturnCode family_convert_status = family2native(address_family, (sa_family_t*)&native_hints.ai_family);
    if (family_convert_status != TCS_SUCCESS)
        return family_convert_status;
    if (node == NULL)
        native_hints.ai_flags = AI_PASSIVE;

    struct addrinfo* native_addrinfo_list = NULL;
    int sts = getaddrinfo(node, service, &native_hints, &native_addrinfo_list);
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
            TcsReturnCode convert_address_status = native2sockaddr(iter->ai_addr, &found_addresses[i]);
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

// This is not part of posix, we need to be platform specific here
#if TCS_HAVE_IFADDRS // acquired from CMake
TcsReturnCode tcs_get_interfaces(struct TcsInterface found_interfaces[],
                                 size_t found_interfaces_length,
                                 size_t* no_of_found_interfaces)
{
    if (found_interfaces == NULL && no_of_found_interfaces == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (found_interfaces == NULL && found_interfaces_length != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (no_of_found_interfaces != NULL)
        *no_of_found_interfaces = 0;

    struct ifaddrs* interfaces;
    getifaddrs(&interfaces);
    size_t i = 0;

    for (struct ifaddrs* iter = interfaces; iter != NULL && (found_interfaces == NULL || i < found_interfaces_length);
         iter = iter->ifa_next)
    {
        if (iter->ifa_flags & IFF_UP)
        {
            struct TcsAddress t;
            if (native2sockaddr(iter->ifa_addr, &t) != TCS_SUCCESS)
                continue;
            if (found_interfaces != NULL)
            {
                found_interfaces[i].address = t;
                strncpy(found_interfaces[i].name, iter->ifa_name, 31);
                found_interfaces[i].name[31] = '\0';
            }
            i++;
        }
    }
    freeifaddrs(interfaces);
    if (no_of_found_interfaces != NULL)
        *no_of_found_interfaces = i;
    return TCS_SUCCESS;
}
#else
// SunOS before 2010, HP and AIX does not support getifaddrs
// ioctl implementation,
// https://stackoverflow.com/questions/4139405/how-can-i-get-to-know-the-ip-address-for-interfaces-in-c/4139811#4139811
TcsReturnCode tcs_get_interfaces(struct TcsInterface found_interfaces[],
                                 size_t found_interfaces_length,
                                 size_t* no_of_found_interfaces)
{
    if (found_interfaces == NULL && no_of_found_interfaces == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (found_interfaces == NULL && found_interfaces_length != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (no_of_found_interfaces != NULL)
        *no_of_found_interfaces = 0;

    int fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return TCS_ERROR_UNKNOWN;

    struct ifconf adapters;
    char buf[16384];
    adapters.ifc_len = sizeof(buf);
    adapters.ifc_buf = buf;
    if (ioctl(fd, SIOCGIFCONF, &adapters) != 0) // todo(markus): add tries as for Windows
    {
        close(fd);
        return TCS_ERROR_UNKNOWN;
    }

    struct ifreq* ifr_iterator = adapters.ifc_req;
    int offset = 0;
    int len;
    size_t i = 0;
    while (offset < adapters.ifc_len)
    {
        // todo(markusl): we need to investigate which OS uses which system, scheduled to after CI with multiple OS
#ifndef __linux__
        len = IFNAMSIZ + ifr_iterator->ifr_addr.sa_len;
#else
        len = sizeof(struct ifreq);
#endif
        if (found_interfaces != NULL)
        {
            native2sockaddr((struct sockaddr*)&ifr_iterator->ifr_addr, &found_interfaces[i].address);
            strncpy(found_interfaces[i].name, ifr_iterator->ifr_name, 31);
            found_interfaces[i].name[31] = '\0';
        }
        ++i;
        ifr_iterator = (struct ifreq*)((uint8_t*)ifr_iterator + len);
        offset += len;
    }
    if (no_of_found_interfaces != NULL)
        *no_of_found_interfaces = i;
    close(fd);
    return TCS_SUCCESS;
}
#endif

TcsReturnCode tcs_set_linger(TcsSocket socket_ctx, bool do_linger, int timeout_seconds)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {(u_short)do_linger, (u_short)timeout_seconds};
    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, sizeof(l));
}

TcsReturnCode tcs_get_linger(TcsSocket socket_ctx, bool* do_linger, int* timeout_seconds)
{
    if (socket_ctx == TCS_NULLSOCKET || (do_linger == NULL && timeout_seconds == NULL))
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {0};
    size_t l_size = sizeof(l);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, &l_size);
    if (sts == TCS_SUCCESS)
    {
        if (do_linger)
            *do_linger = l.l_onoff;
        if (timeout_seconds)
            *timeout_seconds = l.l_linger;
    }

    return sts;
}

TcsReturnCode tcs_set_receive_timeout(TcsSocket socket_ctx, int timeout_ms)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &tv, sizeof(tv));
}

TcsReturnCode tcs_get_receive_timeout(TcsSocket socket_ctx, int* timeout_ms)
{
    if (socket_ctx == TCS_NULLSOCKET || timeout_ms == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct timeval tv = {0};
    size_t tv_size = sizeof(tv);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &tv, &tv_size);

    if (sts == TCS_SUCCESS)
    {
        int c = 0;
        c += (int)tv.tv_sec * 1000;
        c += (int)tv.tv_usec / 1000;
        *timeout_ms = c;
    }
    return sts;
}

TcsReturnCode tcs_set_ip_multicast_add(TcsSocket socket_ctx,
                                       const struct TcsAddress* local_address,
                                       const struct TcsAddress* multicast_address)
{
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // Override here to remove error

    struct sockaddr_storage address_native_multicast = {0};
    socklen_t address_native_multicast_size = 0;
    TcsReturnCode mutlicast_to_native_sts =
        sockaddr2native(multicast_address, (struct sockaddr*)&address_native_multicast, &address_native_multicast_size);
    if (mutlicast_to_native_sts != TCS_SUCCESS)
        return mutlicast_to_native_sts;

    struct sockaddr_storage address_native_local = {0};
    socklen_t address_native_local_size = 0;
    if (local_address != NULL)
    {
        TcsReturnCode mutlicast_to_local_sts =
            sockaddr2native(local_address, (struct sockaddr*)&address_native_local, &address_native_local_size);
        if (mutlicast_to_local_sts)
            return mutlicast_to_local_sts;
    }

    if (multicast_address->family == TCS_AF_IP4)
    {
        struct sockaddr_in* address_native_multicast_p = (struct sockaddr_in*)&address_native_multicast;
        struct sockaddr_in* address_native_local_p = (struct sockaddr_in*)&address_native_local;

        struct ip_mreq mreq = {0};

        mreq.imr_interface.s_addr = address_native_local_p->sin_addr.s_addr;
        mreq.imr_multiaddr.s_addr = address_native_multicast_p->sin_addr.s_addr;

        return tcs_set_option(socket_ctx, TCS_SOL_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    }
    else
    {
        // TODO(markusl): Add ipv6 support
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
}

TcsReturnCode tcs_set_ip_multicast_drop(TcsSocket socket_ctx,
                                        const struct TcsAddress* local_address,
                                        const struct TcsAddress* multicast_address)
{
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage address_native_multicast = {0};
    socklen_t address_native_multicast_size = 0;
    TcsReturnCode mutlicast_to_native_sts =
        sockaddr2native(multicast_address, (struct sockaddr*)&address_native_multicast, &address_native_multicast_size);
    if (mutlicast_to_native_sts != TCS_SUCCESS)
        return mutlicast_to_native_sts;

    struct sockaddr_storage address_native_local = {0};
    socklen_t address_native_local_size = 0;
    if (local_address != NULL)
    {
        TcsReturnCode mutlicast_to_local_sts =
            sockaddr2native(local_address, (struct sockaddr*)&address_native_local, &address_native_local_size);
        if (mutlicast_to_local_sts)
            return mutlicast_to_local_sts;
    }

    if (multicast_address->family == TCS_AF_IP4)
    {
        struct sockaddr_in* address_native_multicast_p = (struct sockaddr_in*)&address_native_multicast;
        struct sockaddr_in* address_native_local_p = (struct sockaddr_in*)&address_native_local;

        struct ip_mreq mreq = {0};

        mreq.imr_interface.s_addr = address_native_local_p->sin_addr.s_addr;
        mreq.imr_multiaddr.s_addr = address_native_multicast_p->sin_addr.s_addr;

        return tcs_set_option(socket_ctx, TCS_SOL_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    }
    else
    {
        // TODO(markusl): Add ipv6 support
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
}

#endif
