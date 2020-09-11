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

const tcs_socket TCS_NULLSOCKET = INVALID_SOCKET;

// Domain
const int TCS_AF_INET = AF_INET;

// Type
const int TCS_SOCK_STREAM = SOCK_STREAM;
const int TCS_SOCK_DGRAM = SOCK_DGRAM;

// Protocol
const int TCS_IPPROTO_TCP = IPPROTO_TCP;
const int TCS_IPPROTO_UDP = IPPROTO_UDP;

// Flags
const int TCS_AI_PASSIVE = AI_PASSIVE;

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

static int wsaerror2retcode(int wsa_error)
{
    switch (wsa_error)
    {
        case WSANOTINITIALISED:
            return TCS_ERROR_NOT_INITED;
        default:
            return TCS_ERROR_UNKNOWN;
    }
}

static int socketstatus2retcode(int status)
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

int tcs_lib_init()
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

int tcs_lib_free()
{
    g_init_count--;
    if (g_init_count <= 0)
    {
        WSACleanup();
    }
    return TCS_SUCCESS;
}

int tcs_create(tcs_socket* socket_ctx, int domain, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    tcs_socket new_socket = socket(domain, type, protocol);

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

int tcs_bind(tcs_socket socket_ctx, const struct tcs_sockaddr* address, size_t address_size)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = bind(socket_ctx, (const struct sockaddr*)address, (int)address_size);
    return socketstatus2retcode(status);
}

int tcs_connect(tcs_socket socket_ctx, const struct tcs_sockaddr* address, size_t address_size)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = connect(socket_ctx, (const struct sockaddr*)address, (int)address_size);
    return socketstatus2retcode(status);
}

int tcs_listen(tcs_socket socket_ctx, int backlog)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = listen(socket_ctx, (int)backlog);
    return socketstatus2retcode(status);
}

int tcs_accept(tcs_socket socket_ctx, tcs_socket* child_socket_ctx, struct tcs_sockaddr* address, size_t* address_size)
{
    if (socket_ctx == TCS_NULLSOCKET || child_socket_ctx == NULL || *child_socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    *child_socket_ctx = accept(socket_ctx, (struct sockaddr*)address, (int*)address_size);
    if (*child_socket_ctx != INVALID_SOCKET)
    {
        return TCS_SUCCESS;
    }
    else
    {
        *child_socket_ctx = TCS_NULLSOCKET;
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }
}

int tcs_send(tcs_socket socket_ctx, const uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_sent)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = send(socket_ctx, (const char*)buffer, (int)buffer_size, (int)flags);
    if (status != SOCKET_ERROR)
    {
        if (bytes_sent != NULL)
            *bytes_sent = status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return socketstatus2retcode(status);
    }
}

int tcs_sendto(tcs_socket socket_ctx,
               const uint8_t* buffer,
               size_t buffer_size,
               uint32_t flags,
               const struct tcs_sockaddr* destination_address,
               size_t destination_address_size,
               size_t* bytes_sent)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = sendto(socket_ctx,
                        (const char*)buffer,
                        (int)buffer_size,
                        (int)flags,
                        (const struct sockaddr*)destination_address,
                        (int)destination_address_size);

    if (status != SOCKET_ERROR)
    {
        if (bytes_sent != NULL)
            *bytes_sent = status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return socketstatus2retcode(status);
    }
}

int tcs_recv(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_received)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = recv(socket_ctx, (char*)buffer, (int)buffer_size, (int)flags);

    if (status == 0)
    {
        return TCS_ERROR_SOCKET_CLOSED;
    }
    else if (status != SOCKET_ERROR)
    {
        if (bytes_received != NULL)
            *bytes_received = status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_received != NULL)
            *bytes_received = 0;

        return socketstatus2retcode(status);
    }
}

int tcs_recvfrom(tcs_socket socket_ctx,
                 uint8_t* buffer,
                 size_t buffer_size,
                 uint32_t flags,
                 struct tcs_sockaddr* source_address,
                 size_t* source_address_size,
                 size_t* bytes_received)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = recvfrom(socket_ctx,
                          (char*)buffer,
                          (int)buffer_size,
                          (int)flags,
                          (struct sockaddr*)source_address,
                          (int*)source_address_size);

    if (status == 0)
    {
        return TCS_ERROR_SOCKET_CLOSED;
    }
    else if (status != SOCKET_ERROR)
    {
        if (bytes_received != NULL)
            *bytes_received = status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_received != NULL)
            *bytes_received = 0;

        return socketstatus2retcode(status);
    }
}

int tcs_setsockopt(tcs_socket socket_ctx, int32_t level, int32_t option_name, const void* option_value, int option_size)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = setsockopt(socket_ctx, (int)level, (int)option_name, (const char*)option_value, (int)option_size);
    return socketstatus2retcode(status);
}

int tcs_shutdown(tcs_socket socket_ctx, int how)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = shutdown(socket_ctx, how);
    return socketstatus2retcode(status);
}

int tcs_close(tcs_socket* socket_ctx)
{
    if (socket_ctx == NULL || *socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = closesocket(*socket_ctx);
    if (status != SOCKET_ERROR)
    {
        *socket_ctx = TCS_NULLSOCKET;
        return TCS_SUCCESS;
    }
    else
    {
        return socketstatus2retcode(status);
    }
}

int tcs_getaddrinfo(const char* node, const char* service, const struct tcs_addrinfo* hints, struct tcs_addrinfo** res)
{
    if ((node == NULL && service == NULL) || res == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    int ret = getaddrinfo(node, service, (const struct addrinfo*)hints, (struct addrinfo**)res);

    if (ret != 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    if (*res == NULL)
        return TCS_ERROR_UNKNOWN;

    return TCS_SUCCESS;
}

int tcs_freeaddrinfo(struct tcs_addrinfo** addressinfo)
{
    if (addressinfo == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    freeaddrinfo((PADDRINFOA)*addressinfo);
    *addressinfo = NULL;
    return TCS_SUCCESS;
}

#endif
