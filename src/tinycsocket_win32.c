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

#include "tinycsocket.h"

#ifdef TINYCSOCKET_USE_WIN32_IMPL

#if !defined(NTDDI_VERSION) && !defined(WINVER) && !defined(NTDDI_VERSION)

#ifdef _WIN64
#define NTDDI_VERSION 0x05020000
#define _WIN32_WINNT 0x0502
#define WINVER 0x0502
#else
#define NTDDI_VERSION 0x05010300
#define _WIN32_WINNT 0x0501
#define WINVER 0x0501
#endif

#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// after windows.h
#include <winsock2.h> // sockets
#include <ws2tcpip.h> // getaddrinfo

// after winsock2
#include <iphlpapi.h> // GetAdaptersAddresses

#include <stdlib.h> // Malloc for GetAdaptersAddresses

#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#endif

const TcsSocket TCS_NULLSOCKET = INVALID_SOCKET;

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

// IP options
const int TCS_SO_IP_NODELAY = TCP_NODELAY;
const int TCS_SO_IP_MEMBERSHIP_ADD = IP_ADD_MEMBERSHIP;
const int TCS_SO_IP_MEMBERSHIP_DROP = IP_DROP_MEMBERSHIP;
const int TCS_SO_IP_MULTICAST_LOOP = IP_MULTICAST_LOOP;

int g_init_count = 0;

static TcsReturnCode wsaerror2retcode(int wsa_error)
{
    switch (wsa_error)
    {
        case WSANOTINITIALISED:
            return TCS_ERROR_NOT_INITED;
        default:
            return TCS_ERROR_UNKNOWN;
    }
}

static TcsReturnCode socketstatus2retcode(int status)
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

static TcsReturnCode family2native(const TcsAddressFamily family, short* native_family)
{
    static uint16_t lut[TCS_AF_LENGTH] = {AF_UNSPEC, AF_INET, AF_INET6};
    if (native_family == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (family >= TCS_AF_LENGTH || family < 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    *native_family = lut[family];
    return TCS_SUCCESS;
}

static TcsReturnCode sockaddr2native(const struct TcsAddress* in_addr, PSOCKADDR out_addr, int* out_addrlen)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->family == TCS_AF_IP4)
    {
        PSOCKADDR_IN addr = (PSOCKADDR_IN)out_addr;
        addr->sin_family = (ADDRESS_FAMILY)AF_INET;
        addr->sin_port = htons((USHORT)in_addr->data.af_inet.port);
        addr->sin_addr.S_un.S_addr = htonl((ULONG)in_addr->data.af_inet.address);

        if (out_addrlen != NULL)
            *out_addrlen = sizeof(SOCKADDR_IN);

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

static TcsReturnCode native2sockaddr(const PSOCKADDR in_addr, struct TcsAddress* out_addr)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->sa_family == AF_INET)
    {
        PSOCKADDR_IN addr = (PSOCKADDR_IN)in_addr;
        out_addr->family = TCS_AF_IP4;
        out_addr->data.af_inet.port = ntohs((uint16_t)addr->sin_port);
        out_addr->data.af_inet.address = ntohl((uint32_t)addr->sin_addr.S_un.S_addr);
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
    if (g_init_count <= 0)
    {
        WSADATA wsa_data;
        int wsa_startup_status_code = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (wsa_startup_status_code != 0)
        {
            return TCS_ERROR_KERNEL;
        }
    }
    ++g_init_count;
    return TCS_SUCCESS;
}

TcsReturnCode tcs_lib_free()
{
    g_init_count--;
    if (g_init_count <= 0)
    {
        WSACleanup();
    }
    return TCS_SUCCESS;
}

TcsReturnCode tcs_create(TcsSocket* socket_ctx, TcsAddressFamily family, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    short native_family = AF_UNSPEC;
    TcsReturnCode sts = family2native(family, &native_family);
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

TcsReturnCode tcs_bind(TcsSocket socket_ctx, const struct TcsAddress* address)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE native_sockaddr = {0};
    int addrlen = 0;
    TcsReturnCode convert_addr_status = sockaddr2native(address, (PSOCKADDR)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;
    int bind_status = bind(socket_ctx, (PSOCKADDR)&native_sockaddr, addrlen);
    return socketstatus2retcode(bind_status);
}

TcsReturnCode tcs_connect(TcsSocket socket_ctx, const struct TcsAddress* address)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE native_sockaddr = {0};
    int addrlen = 0;
    TcsReturnCode convert_addr_status = sockaddr2native(address, (PSOCKADDR)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;
    int connect_status = connect(socket_ctx, (PSOCKADDR)&native_sockaddr, addrlen);
    return socketstatus2retcode(connect_status);
}

TcsReturnCode tcs_listen(TcsSocket socket_ctx, int backlog)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = listen(socket_ctx, (int)backlog);
    return socketstatus2retcode(status);
}

TcsReturnCode tcs_accept(TcsSocket socket_ctx, TcsSocket* child_socket_ctx, struct TcsAddress* address)
{
    if (socket_ctx == TCS_NULLSOCKET || child_socket_ctx == NULL || *child_socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    SOCKADDR_STORAGE native_sockaddr = {0};
    int addr_len = sizeof(native_sockaddr);
    *child_socket_ctx = accept(socket_ctx, (PSOCKADDR)&native_sockaddr, &addr_len);
    if (*child_socket_ctx != INVALID_SOCKET)
    {
        if (address != NULL)
        {
            TcsReturnCode convert_addr_status = native2sockaddr((PSOCKADDR)&native_sockaddr, address);
            if (convert_addr_status != TCS_SUCCESS)
                return convert_addr_status;
        }
        return TCS_SUCCESS;
    }
    else
    {
        *child_socket_ctx = TCS_NULLSOCKET;
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
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

    int send_status = send(socket_ctx, (const char*)buffer, (int)buffer_size, (int)flags);
    if (send_status != SOCKET_ERROR)
    {
        if (bytes_sent != NULL)
            *bytes_sent = send_status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return socketstatus2retcode(send_status);
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

    SOCKADDR_STORAGE native_sockaddr = {0};
    int addrlen = 0;
    TcsReturnCode convert_addr_status = sockaddr2native(destination_address, (PSOCKADDR)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;

    int sendto_status =
        sendto(socket_ctx, (const char*)buffer, (int)buffer_size, (int)flags, (PSOCKADDR)&native_sockaddr, addrlen);

    if (sendto_status != SOCKET_ERROR)
    {
        if (bytes_sent != NULL)
            *bytes_sent = sendto_status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return socketstatus2retcode(sendto_status);
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

    int recv_status = recv(socket_ctx, (char*)buffer, (int)buffer_size, (int)flags);

    if (recv_status == 0)
    {
        return TCS_ERROR_SOCKET_CLOSED;
    }
    else if (recv_status != SOCKET_ERROR)
    {
        if (bytes_received != NULL)
            *bytes_received = recv_status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_received != NULL)
            *bytes_received = 0;

        return socketstatus2retcode(recv_status);
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

    SOCKADDR_STORAGE native_sockaddr = {0};
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
            *bytes_received = recvfrom_status;

        if (source_address != NULL)
        {
            TcsReturnCode convert_address_status = native2sockaddr((PSOCKADDR)&native_sockaddr, source_address);
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

TcsReturnCode tcs_setsockopt(TcsSocket socket_ctx,
                             int32_t level,
                             int32_t option_name,
                             const void* option_value,
                             size_t option_size)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int sockopt_status =
        setsockopt(socket_ctx, (int)level, (int)option_name, (const char*)option_value, (int)option_size);
    return socketstatus2retcode(sockopt_status);
}

TcsReturnCode tcs_getsockopt(TcsSocket socket_ctx,
                             int32_t level,
                             int32_t option_name,
                             void* option_value,
                             size_t* option_size)
{
    if (socket_ctx == TCS_NULLSOCKET || option_value == NULL || option_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    int sockopt_status = getsockopt(socket_ctx, (int)level, (int)option_name, (char*)option_value, (int*)option_size);
    return socketstatus2retcode(sockopt_status);
}

TcsReturnCode tcs_shutdown(TcsSocket socket_ctx, TcsSocketDirection direction)
{
    const int LUT[] = {SD_RECEIVE, SD_SEND, SD_BOTH};

    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    const int how = LUT[direction];
    int shutdown_status = shutdown(socket_ctx, how);
    return socketstatus2retcode(shutdown_status);
}

TcsReturnCode tcs_destroy(TcsSocket* socket_ctx)
{
    if (socket_ctx == NULL || *socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int close_status = closesocket(*socket_ctx);
    if (close_status != SOCKET_ERROR)
    {
        *socket_ctx = TCS_NULLSOCKET;
        return TCS_SUCCESS;
    }
    else
    {
        return socketstatus2retcode(close_status);
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

    ADDRINFOA native_hints = {0};
    TcsReturnCode sts = family2native(address_family, (short*)&native_hints.ai_family);
    if (sts != TCS_SUCCESS)
        return sts;

    if (node == NULL)
        native_hints.ai_flags = AI_PASSIVE;

    PADDRINFOA native_addrinfo_list = NULL;
    int getaddrinfo_status = getaddrinfo(node, service, &native_hints, &native_addrinfo_list);
    if (getaddrinfo_status != 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    if (native_addrinfo_list == NULL)
        return TCS_ERROR_UNKNOWN;

    size_t i = 0;
    if (found_addresses == NULL)
    {
        for (PADDRINFOA iter = native_addrinfo_list; iter != NULL; iter = iter->ai_next)
            i++;
    }
    else
    {
        for (PADDRINFOA iter = native_addrinfo_list; iter != NULL && i < found_addresses_length; iter = iter->ai_next)
        {
            TcsReturnCode address_convert_status = native2sockaddr(iter->ai_addr, &found_addresses[i]);
            if (address_convert_status != TCS_SUCCESS)
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

    const int MAX_TRIES = 3;
    ULONG adapeters_buffer_size = 15000;
    PIP_ADAPTER_ADDRESSES adapters = NULL;
    ULONG adapter_sts = ERROR_NO_DATA;
    for (int i = 0; i < MAX_TRIES; ++i)
    {
        adapters = malloc(adapeters_buffer_size);
        adapter_sts = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapters, &adapeters_buffer_size);
        if (adapter_sts == ERROR_BUFFER_OVERFLOW)
        {
            free(adapters);
            adapters = NULL;
        }
    }
    if (adapter_sts != NO_ERROR)
    {
        if (adapters != NULL)
            free(adapters);
        return TCS_ERROR_UNKNOWN;
    }

    size_t i = 0;
    for (PIP_ADAPTER_ADDRESSES_XP iter = adapters;
         iter != NULL && (found_interfaces == NULL || i < found_interfaces_length);
         iter = iter->Next)
    {
        if (iter->OperStatus != IfOperStatusUp)
            continue;
        for (PIP_ADAPTER_UNICAST_ADDRESS_XP address_iter = iter->FirstUnicastAddress; address_iter != NULL;
             address_iter = address_iter->Next)
        {
            struct TcsAddress t;
            if (native2sockaddr(address_iter->Address.lpSockaddr, &t) != TCS_SUCCESS)
                continue;
            if (found_interfaces != NULL)
            {
                found_interfaces[i].address = t;
                memset(found_interfaces[i].name, '\0', 32);
                WideCharToMultiByte(CP_UTF8, 0, iter->FriendlyName, -1, found_interfaces[i].name, 31, NULL, NULL);
            }
            ++i;
        }
    }
    if (adapters != NULL)
        free(adapters);
    if (no_of_found_interfaces != NULL)
        *no_of_found_interfaces = i;

    return TCS_SUCCESS;
}

TcsReturnCode tcs_set_linger(TcsSocket socket_ctx, bool do_linger, int timeout_seconds)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {0};
    l.l_onoff = (u_short)do_linger;
    l.l_linger = (u_short)timeout_seconds;
    return tcs_setsockopt(socket_ctx, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, sizeof(l));
}

TcsReturnCode tcs_get_linger(TcsSocket socket_ctx, bool* do_linger, int* timeout_seconds)
{
    if (socket_ctx == TCS_NULLSOCKET || (do_linger == NULL && timeout_seconds == NULL))
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {0};
    size_t l_size = sizeof(l);
    TcsReturnCode sts = tcs_getsockopt(socket_ctx, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, &l_size);
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

    return tcs_setsockopt(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));
}

TcsReturnCode tcs_get_receive_timeout(TcsSocket socket_ctx, int* timeout_ms)
{
    if (socket_ctx == TCS_NULLSOCKET || timeout_ms == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    DWORD t = 0;
    size_t t_size = sizeof(t);
    TcsReturnCode sts = tcs_getsockopt(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &t, &t_size);

    if (sts == TCS_SUCCESS)
    {
        *timeout_ms = (int)t;
    }
    return sts;
}

TcsReturnCode tcs_set_ip_multicast_add(TcsSocket socket_ctx,
                                       const struct TcsAddress* local_address,
                                       const struct TcsAddress* multicast_address)
{
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // TODO(markusl): Add ipv6 support
    if (multicast_address->family != TCS_AF_IP4)
        return TCS_ERROR_NOT_IMPLEMENTED;

    struct ip_mreq imr = {0};
    imr.imr_multiaddr.s_addr = htonl(multicast_address->data.af_inet.address);
    if (local_address != NULL)
        imr.imr_interface.s_addr = htonl(local_address->data.af_inet.address);

    return tcs_setsockopt(socket_ctx, TCS_SOL_IP, TCS_SO_IP_MEMBERSHIP_ADD, &imr, sizeof(imr));
}

TcsReturnCode tcs_set_ip_multicast_drop(TcsSocket socket_ctx,
                                        const struct TcsAddress* local_address,
                                        const struct TcsAddress* multicast_address)
{
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // TODO(markusl): Add ipv6 support
    if (multicast_address->family != TCS_AF_IP4)
        return TCS_ERROR_NOT_IMPLEMENTED;

    struct ip_mreq imr = {0};
    imr.imr_multiaddr.s_addr = htonl(multicast_address->data.af_inet.address);
    if (local_address != NULL)
        imr.imr_interface.s_addr = htonl(local_address->data.af_inet.address);

    return tcs_setsockopt(socket_ctx, TCS_SOL_IP, TCS_SO_IP_MEMBERSHIP_DROP, &imr, sizeof(imr));
}
#endif
