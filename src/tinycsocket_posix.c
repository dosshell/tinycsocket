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

#ifdef TINYCSOCKET_USE_POSIX_IMPL

#include <errno.h>
#include <netdb.h>      // Protocols and custom return codes
#include <netinet/in.h> // IPPROTO_XXP
#include <sys/socket.h> // pretty much everything
#include <sys/types.h>  // POSIX.1 compatibility
#include <unistd.h>     // close()

const tcs_socket TCS_NULLSOCKET = -1;

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
const int TCS_SD_RECIEVE = SHUT_RD;
const int TCS_SD_SEND = SHUT_WR;
const int TCS_SD_BOTH = SHUT_RDWR;

// Socket options
const int TCS_SOL_SOCKET = SOL_SOCKET;
const int TCS_SO_REUSEADDR = SO_REUSEADDR;
const int TCS_SO_RCVBUF = SO_RCVBUF;
const int TCS_SO_SNDBUF = SO_SNDBUF;

static int errno2retcode(int error_code)
{
    switch (error_code)
    {
        case ECONNREFUSED:
            return TCS_ERROR_CONNECTION_REFUSED;
        default:
            return TCS_ERROR_UNKNOWN;
    }
}

static int sockaddr2native(const struct tcs_sockaddr* in_addr, struct sockaddr* out_addr, socklen_t* out_addrlen)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->family == TCS_AF_INET)
    {
        struct sockaddr_in* addr = (struct sockaddr_in*)out_addr;
        addr->sin_family = (sa_family_t)AF_INET;
        addr->sin_port = (in_port_t)(in_addr->data.af_inet.port);
        addr->sin_addr.s_addr = (in_addr_t)in_addr->data.af_inet.addr;

        if (out_addrlen != NULL)
            *out_addrlen = sizeof(struct sockaddr_in);

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

static int native2sockaddr(const struct sockaddr* in_addr, struct tcs_sockaddr* out_addr)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->sa_family == AF_INET)
    {
        struct sockaddr_in* addr = (struct sockaddr_in*)in_addr;
        out_addr->family = TCS_AF_INET;
        out_addr->data.af_inet.port = (uint16_t)addr->sin_port;
        out_addr->data.af_inet.addr = (uint32_t)addr->sin_addr.s_addr;
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

static int addrinfo2native(const struct tcs_addrinfo* in_info, struct addrinfo* out_info)
{
    if (in_info == NULL || out_info == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    out_info->ai_family = in_info->family;
    out_info->ai_socktype = in_info->socktype;
    out_info->ai_protocol = in_info->protocol;
    out_info->ai_flags = (int)in_info->flags;
    out_info->ai_next = NULL;
    int convert_status = sockaddr2native(&in_info->address, out_info->ai_addr, &out_info->ai_addrlen);
    if (convert_status != TCS_SUCCESS)
        return convert_status;

    return TCS_SUCCESS;
}

static int native2addrinfo(const struct addrinfo* in_info, struct tcs_addrinfo* out_info)
{
    if (in_info == NULL || out_info == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    int convert_status = native2sockaddr(in_info->ai_addr, &out_info->address);
    if (convert_status != TCS_SUCCESS)
        return convert_status;

    out_info->family = (uint16_t)in_info->ai_family;
    out_info->socktype = in_info->ai_socktype;
    out_info->protocol = in_info->ai_protocol;
    out_info->flags = (uint32_t)in_info->ai_flags;

    return TCS_SUCCESS;
}

int tcs_lib_init()
{
    // Not needed for posix
    return TCS_SUCCESS;
}

int tcs_lib_free()
{
    // Not needed for posix
    return TCS_SUCCESS;
}

int tcs_create(tcs_socket* socket_ctx, int family, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    *socket_ctx = socket(family, type, protocol);

    if (*socket_ctx != -1) // Same as TCS_NULLSOCKET
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

int tcs_bind(tcs_socket socket_ctx, const struct tcs_sockaddr* address)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr = {0};
    socklen_t addrlen = 0;
    int convert_addr_status = sockaddr2native(address, (struct sockaddr*)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;
    if (bind(socket_ctx, (struct sockaddr*)&native_sockaddr, addrlen) != -1)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

int tcs_connect(tcs_socket socket_ctx, const struct tcs_sockaddr* address)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr = {0};
    socklen_t addrlen = 0;
    int convert_addr_status = sockaddr2native(address, (struct sockaddr*)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;

    if (connect(socket_ctx, (const struct sockaddr*)&native_sockaddr, addrlen) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

int tcs_listen(tcs_socket socket_ctx, int backlog)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (listen(socket_ctx, backlog) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

int tcs_accept(tcs_socket socket_ctx, tcs_socket* child_socket_ctx, struct tcs_sockaddr* address)
{
    if (socket_ctx == TCS_NULLSOCKET || child_socket_ctx == NULL || *child_socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr = {0};
    socklen_t addr_len = sizeof(native_sockaddr);

    *child_socket_ctx = accept(socket_ctx, (struct sockaddr*)&native_sockaddr, &addr_len);
    if (*child_socket_ctx != -1)
    {
        if (address != NULL)
        {
            int convert_addr_status = native2sockaddr((struct sockaddr*)&native_sockaddr, address);
            if (convert_addr_status != TCS_SUCCESS)
                return convert_addr_status;
        }
        return TCS_SUCCESS;
    }
    else
    {
        *child_socket_ctx = TCS_NULLSOCKET;
        return errno2retcode(errno);
    }
}

int tcs_send(tcs_socket socket_ctx, const uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_sent)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    ssize_t status = send(socket_ctx, (const char*)buffer, buffer_size, (int)flags);
    if (status >= 0)
    {
        if (bytes_sent != NULL)
            *bytes_sent = (size_t)status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;
        return errno2retcode(errno);
    }
}

int tcs_sendto(tcs_socket socket_ctx,
               const uint8_t* buffer,
               size_t buffer_size,
               uint32_t flags,
               const struct tcs_sockaddr* destination_address,
               size_t* bytes_sent)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr = {0};
    socklen_t addrlen = 0;
    int convert_addr_status = sockaddr2native(destination_address, (struct sockaddr*)&native_sockaddr, &addrlen);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;

    ssize_t status = sendto(socket_ctx,
                            (const char*)buffer,
                            buffer_size,
                            (int)flags,
                            (const struct sockaddr*)&native_sockaddr,
                            (socklen_t)addrlen);

    if (status >= 0)
    {
        if (bytes_sent != NULL)
            *bytes_sent = (size_t)status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return errno2retcode(errno);
    }
}

int tcs_recv(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_size, uint32_t flags, size_t* bytes_received)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    ssize_t status = recv(socket_ctx, (char*)buffer, buffer_size, (int)flags);

    if (status > 0)
    {
        if (bytes_received != NULL)
            *bytes_received = (size_t)status;
        return TCS_SUCCESS;
    }
    else if (status == 0)
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

int tcs_recvfrom(tcs_socket socket_ctx,
                 uint8_t* buffer,
                 size_t buffer_size,
                 uint32_t flags,
                 struct tcs_sockaddr* source_address,
                 size_t* bytes_received)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr = {0};
    socklen_t addrlen = sizeof(native_sockaddr);

    ssize_t status = recvfrom(
        socket_ctx, (char*)buffer, buffer_size, (int)flags, (struct sockaddr*)&native_sockaddr, (socklen_t*)&addrlen);

    if (status > 0)
    {
        if (source_address != NULL)
            native2sockaddr((struct sockaddr*)&native_sockaddr, source_address);
        if (bytes_received != NULL)
            *bytes_received = (size_t)status;
        return TCS_SUCCESS;
    }
    else if (status == 0)
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

int tcs_setsockopt(tcs_socket socket_ctx,
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

int tcs_shutdown(tcs_socket socket_ctx, int how)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (shutdown(socket_ctx, how) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

int tcs_close(tcs_socket* socket_ctx)
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

int tcs_getaddrinfo(const char* node,
                    const char* service,
                    const struct tcs_addrinfo* hints,
                    struct tcs_addrinfo res[],
                    size_t res_count,
                    size_t* used_count)
{
    if (node == NULL && service == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (res == NULL && used_count == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct addrinfo* phints = NULL;
    struct addrinfo native_hints = {0};
    if (hints != NULL)
    {
        addrinfo2native(hints, &native_hints);
        phints = &native_hints;
    }

    if (used_count != NULL)
        *used_count = 0;

    struct addrinfo* native_res = NULL;
    int sts = getaddrinfo(node, service, phints, &native_res);
    if (sts == EAI_SYSTEM)
        return errno2retcode(errno);
    else if (native_res == NULL)
        return TCS_ERROR_UNKNOWN;
    else if (sts != 0)
        return TCS_ERROR_UNKNOWN;

    size_t i = 0;
    if (res == NULL)
    {
        for (struct addrinfo* iter = native_res; iter != NULL; iter = iter->ai_next)
            i++;
    }
    else
    {
        for (struct addrinfo* iter = native_res; iter != NULL && i < res_count; iter = iter->ai_next)
        {
            int status = native2addrinfo(iter, &res[i]);
            if (status != TCS_SUCCESS)
                continue;
            i++;
        }
    }
    if (used_count != NULL)
        *used_count = i;

    freeaddrinfo(native_res);

    if (i == 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    return TCS_SUCCESS;
}

#endif
