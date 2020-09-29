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

const TcsSocket TCS_NULLSOCKET = -1;

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

    if (in_addr->family == TCS_AF_INET)
    {
        struct sockaddr_in* addr = (struct sockaddr_in*)out_addr;
        addr->sin_family = (sa_family_t)AF_INET;
        addr->sin_port = (in_port_t)(in_addr->data.af_inet.port);
        addr->sin_addr.s_addr = (in_addr_t)in_addr->data.af_inet.address;

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

static TcsReturnCode native2sockaddr(const struct sockaddr* in_addr, struct TcsAddress* out_addr)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->sa_family == AF_INET)
    {
        struct sockaddr_in* addr = (struct sockaddr_in*)in_addr;
        out_addr->family = TCS_AF_INET;
        out_addr->data.af_inet.port = (uint16_t)addr->sin_port;
        out_addr->data.af_inet.address = (uint32_t)addr->sin_addr.s_addr;
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

TcsReturnCode tcs_create(TcsSocket* socket_ctx, int family, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    *socket_ctx = socket(family, type, protocol);

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

TcsReturnCode tcs_sendto(TcsSocket socket_ctx,
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

TcsReturnCode tcs_recv(TcsSocket socket_ctx,
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

TcsReturnCode tcs_recvfrom(TcsSocket socket_ctx,
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

TcsReturnCode tcs_setsockopt(TcsSocket socket_ctx,
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

TcsReturnCode tcs_shutdown(TcsSocket socket_ctx, int how)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (shutdown(socket_ctx, how) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_close(TcsSocket* socket_ctx)
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

TcsReturnCode tcs_getaddrinfo(const char* node,
                              const char* service,
                              uint16_t address_family,
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
    native_hints.ai_family = address_family;
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

#endif
