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

#define __socklen_t_defined
#include <sys/socket.h> // pretty much everything
#include <netdb.h> // Protocols and custom return codes
#include <unistd.h> // close()
#include <errno.h>

const tcs_socket TCS_NULLSOCKET = -1;

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

int errno2retcode(int error_code)
{
    return TCS_ERROR_UNKNOWN;
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

int tcs_create(tcs_socket* socket_ctx, int domain, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    *socket_ctx = socket(domain, type, protocol);

    if (*socket_ctx != -1) // Same as TCS_NULLSOCKET
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

int tcs_bind(tcs_socket socket_ctx,
                     const struct tcs_sockaddr* address,
                     socklen_t address_length)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (bind(socket_ctx, (const struct sockaddr*)address, (int)address_length) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

int tcs_connect(tcs_socket socket_ctx,
                        const struct tcs_sockaddr* address,
                        socklen_t address_length)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (connect(socket_ctx, (const struct sockaddr*)address, address_length) == 0)
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

int tcs_accept(tcs_socket socket_ctx,
                       tcs_socket* child_socket_ctx,
                       struct tcs_sockaddr* address,
                       socklen_t* address_length)
{
    if (socket_ctx == TCS_NULLSOCKET || child_socket_ctx == NULL ||
        *child_socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int new_child_socket = accept(socket_ctx, (struct sockaddr*)address, address_length);
    if (new_child_socket != -1)
    {
        *child_socket_ctx = new_child_socket;
        return TCS_SUCCESS;
    }
    else
    {
        return errno2retcode(errno);
    }
}

int tcs_send(tcs_socket socket_ctx,
                     const uint8_t* buffer,
                     size_t buffer_length,
                     uint32_t flags,
                     size_t* bytes_sent)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = send(socket_ctx, (const char*)buffer, (int)buffer_length, (int)flags);
    if (status != -1)
    {
        if (bytes_sent != NULL)
            *bytes_sent = status;
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
                       size_t buffer_length,
                       uint32_t flags,
                       const struct tcs_sockaddr* destination_address,
                       size_t destination_address_length,
                       size_t* bytes_sent)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = sendto(socket_ctx,
                        (const char*)buffer,
                        (int)buffer_length,
                        (int)flags,
                        (const struct sockaddr*)destination_address,
                        (int)destination_address_length);

    if (status != -1)
    {
        if (bytes_sent != NULL)
            *bytes_sent = status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return errno2retcode(errno);
    }
}

int tcs_recv(tcs_socket socket_ctx,
                     uint8_t* buffer,
                     size_t buffer_length,
                     uint32_t flags,
                     size_t* bytes_recieved)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = recv(socket_ctx, (char*)buffer, (int)buffer_length, (int)flags);

    if (status > 0)
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = status;
        return TCS_SUCCESS;
    }
    else if (status == 0)
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = 0;
        return TCS_ERROR_NOT_CONNECTED; // TODO: think about this
    }
    else
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = 0;
        return errno2retcode(errno);
    }
}

int tcs_recvfrom(tcs_socket socket_ctx,
                         uint8_t* buffer,
                         size_t buffer_length,
                         uint32_t flags,
                         struct tcs_sockaddr* source_address,
                         size_t* source_address_length,
                         size_t* bytes_recieved)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = recvfrom(socket_ctx,
                          (char*)buffer,
                          (int)buffer_length,
                          (int)flags,
                          (struct sockaddr*)source_address,
                          (int*)source_address_length);

    if (status > 0)
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = status;
        return TCS_SUCCESS;
    }
    else if (status == 0)
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = 0;
        return TCS_ERROR_NOT_CONNECTED; // TODO: think about this
    }
    else
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = 0;
        return errno2retcode(errno);
    }
}

int tcs_setsockopt(tcs_socket socket_ctx,
                           int32_t level,
                           int32_t option_name,
                           const void* option_value,
                           socklen_t option_length)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (setsockopt(socket_ctx, (int)level, (int)option_name, (const char*)option_value, (int)option_length) == 0)
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
                            struct tcs_addrinfo** res)
{
    if ((node == NULL && service == NULL) || res == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    int status = getaddrinfo(node, service, (const struct addrinfo*)hints, (struct addrinfo**)res);
    if (status == 0)
        return TCS_SUCCESS;
    else if (status == EAI_SYSTEM)
        return errno2retcode(errno);
    else
        return TCS_ERROR_UNKNOWN;
}

int tcs_freeaddrinfo(struct tcs_addrinfo** addressinfo)
{
    if (addressinfo == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    freeaddrinfo((struct addrinfo*)*addressinfo);

    *addressinfo = NULL;
    return TCS_SUCCESS;
}

#endif
