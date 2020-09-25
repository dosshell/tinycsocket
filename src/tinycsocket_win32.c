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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <mstcpip.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

const TcsSocket TCS_NULLSOCKET = INVALID_SOCKET;

// Family
const uint16_t TCS_AF_UNSPEC = AF_UNSPEC;
const uint16_t TCS_AF_INET = AF_INET;
const uint16_t TCS_AF_INET6 = AF_INET6;

// Type
const int TCS_SOCK_STREAM = SOCK_STREAM;
const int TCS_SOCK_DGRAM = SOCK_DGRAM;

// Protocol
const int TCS_IPPROTO_TCP = IPPROTO_TCP;
const int TCS_IPPROTO_UDP = IPPROTO_UDP;

// Flags
const uint32_t TCS_AI_PASSIVE = AI_PASSIVE;

// Recv flags
const int TCS_MSG_WAITALL = MSG_WAITALL;
const int TCS_MSG_PEEK = MSG_PEEK;
const int TCS_MSG_OOB = MSG_OOB;

// Backlog
const int TCS_BACKLOG_SOMAXCONN = SOMAXCONN;

// How
const int TCS_SD_RECIEVE = SD_RECEIVE;
const int TCS_SD_SEND = SD_SEND;
const int TCS_SD_BOTH = SD_BOTH;

// Socket options
const int TCS_SOL_SOCKET = SOL_SOCKET;
const int TCS_SO_REUSEADDR = SO_REUSEADDR;
const int TCS_SO_RCVBUF = SO_RCVBUF;
const int TCS_SO_SNDBUF = SO_SNDBUF;

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

int g_init_count = 0;

static TcsReturnCode sockaddr2native(const struct TcsAddress* in_addr, PSOCKADDR out_addr, int* out_addrlen)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->family == TCS_AF_INET)
    {
        PSOCKADDR_IN addr = (PSOCKADDR_IN)out_addr;
        addr->sin_family = (ADDRESS_FAMILY)AF_INET;
        addr->sin_port = (USHORT)(in_addr->data.af_inet.port);
        addr->sin_addr.S_un.S_addr = (ULONG)in_addr->data.af_inet.address;

        if (out_addrlen != NULL)
            *out_addrlen = sizeof(SOCKADDR_IN);

        return TCS_SUCCESS;
    }
    else if (in_addr->family == TCS_AF_INET6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (in_addr->family == TCS_AF_UNSPEC)
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
        out_addr->family = TCS_AF_INET;
        out_addr->data.af_inet.port = (uint16_t)addr->sin_port;
        out_addr->data.af_inet.address = (uint32_t)addr->sin_addr.S_un.S_addr;
    }
    else if (in_addr->sa_family == TCS_AF_INET6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (in_addr->sa_family == TCS_AF_UNSPEC)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }

    return 0;
}

static TcsReturnCode addrinfo2native(const struct TcsAddressInfo* in_info, PADDRINFOA out_info)
{
    if (in_info == NULL || out_info == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    int convert_status = sockaddr2native(&in_info->address, out_info->ai_addr, (int*)&out_info->ai_addrlen);
    if (convert_status != TCS_SUCCESS)
        return convert_status;

    out_info->ai_family = in_info->family;
    out_info->ai_socktype = in_info->socktype;
    out_info->ai_protocol = in_info->protocol;
    out_info->ai_flags = in_info->flags;
    out_info->ai_next = NULL;

    return TCS_SUCCESS;
}

static TcsReturnCode native2addrinfo(const PADDRINFOA in_info, struct TcsAddressInfo* out_info)
{
    if (in_info == NULL || out_info == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    out_info->family = (uint16_t)in_info->ai_family;
    out_info->socktype = in_info->ai_socktype;
    out_info->protocol = in_info->ai_protocol;
    out_info->flags = (uint32_t)in_info->ai_flags;
    TcsReturnCode convert_status = native2sockaddr(in_info->ai_addr, &out_info->address);
    if (convert_status != TCS_SUCCESS)
        return convert_status;

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

TcsReturnCode tcs_create(TcsSocket* socket_ctx, int family, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsSocket new_socket = socket(family, type, protocol);

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

TcsReturnCode tcs_sendto(TcsSocket socket_ctx,
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

TcsReturnCode tcs_recv(TcsSocket socket_ctx,
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

TcsReturnCode tcs_recvfrom(TcsSocket socket_ctx,
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

TcsReturnCode tcs_shutdown(TcsSocket socket_ctx, int how)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int shutdown_status = shutdown(socket_ctx, how);
    return socketstatus2retcode(shutdown_status);
}

TcsReturnCode tcs_close(TcsSocket* socket_ctx)
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

TcsReturnCode tcs_getaddrinfo(const char* node,
                              const char* service,
                              const struct TcsAddressInfo* hints,
                              struct TcsAddressInfo res[],
                              size_t res_count,
                              size_t* used_count)
{
    if (node == NULL && service == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (res == NULL && used_count == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    PADDRINFOA phints = NULL;
    ADDRINFOA native_hints = {0};
    if (hints != NULL)
    {
        TcsReturnCode address_convert_status = addrinfo2native(hints, &native_hints);
        if (address_convert_status != TCS_SUCCESS)
            return address_convert_status;

        phints = &native_hints;
    }

    if (used_count != NULL)
        *used_count = 0;

    PADDRINFOA native_addrinfo_list = NULL;
    int ret = getaddrinfo(node, service, phints, &native_addrinfo_list);
    if (ret != 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    if (native_addrinfo_list == NULL)
        return TCS_ERROR_UNKNOWN;

    size_t i = 0;
    if (res == NULL)
    {
        for (PADDRINFOA iter = native_addrinfo_list; iter != NULL; iter = iter->ai_next)
            i++;
    }
    else
    {
        for (PADDRINFOA iter = native_addrinfo_list; iter != NULL && i < res_count; iter = iter->ai_next)
        {
            TcsReturnCode address_convert_status = native2addrinfo(iter, &res[i]);
            if (address_convert_status != TCS_SUCCESS)
                continue;
            i++;
        }
    }
    if (used_count != NULL)
        *used_count = i;

    freeaddrinfo(native_addrinfo_list);

    if (i == 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    return TCS_SUCCESS;
}

#endif
