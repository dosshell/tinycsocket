#include "tinycsocket.h"

#ifdef TINYCSOCKET_USE_POSIX_IMPL

#define __socklen_t_defined
#include <sys/socket.h> // pretty much everything
#include <netdb.h> // Protocols and custom return codes
#include <unistd.h> // close()
#include <errno.h>

const TinyCSocketCtx TINYCSOCKET_NULLSOCKET = -1;

// Domain
const int TINYCSOCKET_AF_INET = AF_INET;

// Type
const int TINYCSOCKET_SOCK_STREAM = SOCK_STREAM;
const int TINYCSOCKET_SOCK_DGRAM = SOCK_DGRAM;

// Protocol
const int TINYCSOCKET_IPPROTO_TCP = IPPROTO_TCP;

// Flags
const int TINYCSOCKET_AI_PASSIVE = AI_PASSIVE;

// Backlog
const int TINYCSOCKET_BACKLOG_SOMAXCONN = SOMAXCONN;

int errno2retcode(int error_code)
{
    return TINYCSOCKET_ERROR_UNKNOWN;
}

int tinycsocket_init()
{
    // Not needed for posix
    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_free()
{
    // Not needed for posix
    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_socket(TinyCSocketCtx* socket_ctx, int domain, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    *socket_ctx = socket(domain, type, protocol);

    if (*socket_ctx != -1) // Same as TINYCSOCKET_NULLSOCKET
        return TINYCSOCKET_SUCCESS;
    else
        return errno2retcode(errno);
}

int tinycsocket_bind(TinyCSocketCtx socket_ctx,
                     const struct TinyCSocketAddress* address,
                     socklen_t address_length)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    if (bind(socket_ctx, (const struct sockaddr*)address, (int)address_length) == 0)
        return TINYCSOCKET_SUCCESS;
    else
        return errno2retcode(errno);
}

int tinycsocket_connect(TinyCSocketCtx socket_ctx,
                        const struct TinyCSocketAddress* address,
                        socklen_t address_length)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    if (connect(socket_ctx, (const struct sockaddr*)address, address_length) == 0)
        return TINYCSOCKET_SUCCESS;
    else
        return errno2retcode(errno);
}

int tinycsocket_listen(TinyCSocketCtx socket_ctx, int backlog)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    if (listen(socket_ctx, backlog) == 0)
        return TINYCSOCKET_SUCCESS;
    else
        return errno2retcode(errno);
}

int tinycsocket_accept(TinyCSocketCtx socket_ctx,
                       TinyCSocketCtx* child_socket_ctx,
                       struct TinyCSocketAddress* address,
                       socklen_t* address_length)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET || child_socket_ctx == NULL ||
        *child_socket_ctx != TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int new_child_socket = accept(socket_ctx, (struct sockaddr*)address, address_length);
    if (new_child_socket != -1)
    {
        *child_socket_ctx = new_child_socket;
        return TINYCSOCKET_SUCCESS;
    }
    else
    {
        return errno2retcode(errno);
    }
}

int tinycsocket_send(TinyCSocketCtx socket_ctx,
                     const uint8_t* buffer,
                     size_t buffer_length,
                     uint_fast32_t flags,
                     size_t* bytes_sent)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int status = send(socket_ctx, (const char*)buffer, (int)buffer_length, (int)flags);
    if (status != -1)
    {
        if (bytes_sent != NULL)
            *bytes_sent = status;
        return TINYCSOCKET_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;
        return errno2retcode(errno);
    }
}

int tinycsocket_sendto(TinyCSocketCtx socket_ctx,
                       const uint8_t* buffer,
                       size_t buffer_length,
                       uint_fast32_t flags,
                       const struct TinyCSocketAddress* destination_address,
                       size_t destination_address_length,
                       size_t* bytes_sent)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

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
        return TINYCSOCKET_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return errno2retcode(errno);
    }
}

int tinycsocket_recv(TinyCSocketCtx socket_ctx,
                     uint8_t* buffer,
                     size_t buffer_length,
                     uint_fast32_t flags,
                     size_t* bytes_recieved)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int status = recv(socket_ctx, (char*)buffer, (int)buffer_length, (int)flags);

    if (status > 0)
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = status;
        return TINYCSOCKET_SUCCESS;
    }
    else if (status == 0)
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = 0;
        return TINYCSOCKET_ERROR_NOT_CONNECTED; // TODO: think about this
    }
    else
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = 0;
        return errno2retcode(errno);
    }
}

int tinycsocket_recvfrom(TinyCSocketCtx socket_ctx,
                         uint8_t* buffer,
                         size_t buffer_length,
                         uint_fast32_t flags,
                         struct TinyCSocketAddress* source_address,
                         size_t* source_address_length,
                         size_t* bytes_recieved)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

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
        return TINYCSOCKET_SUCCESS;
    }
    else if (status == 0)
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = 0;
        return TINYCSOCKET_ERROR_NOT_CONNECTED; // TODO: think about this
    }
    else
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = 0;
        return errno2retcode(errno);
    }
}

int tinycsocket_setsockopt(TinyCSocketCtx socket_ctx,
                           int_fast32_t level,
                           int_fast32_t option_name,
                           const void* option_value,
                           socklen_t option_length)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    if (setsockopt(socket_ctx, (int)level, (int)option_name, (const char*)option_value, (int)option_length) == 0)
        return TINYCSOCKET_SUCCESS;
    else
        return errno2retcode(errno);
}

int tinycsocket_shutdown(TinyCSocketCtx socket_ctx, int how)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    // This translation makes it possible to use "binary OR" operations for how
    int posix_how = how - 1;

    if (shutdown(socket_ctx, posix_how) == 0)
        return TINYCSOCKET_SUCCESS;
    else
        return errno2retcode(errno);
}


int tinycsocket_closesocket(TinyCSocketCtx* socket_ctx)
{
    if (socket_ctx == NULL || *socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    if (close(*socket_ctx) == 0)
    {
        *socket_ctx = TINYCSOCKET_NULLSOCKET;
        return TINYCSOCKET_SUCCESS;
    }
    else
    {
        return errno2retcode(errno);
    }
}

int tinycsocket_getaddrinfo(const char* node,
                            const char* service,
                            const struct TinyCSocketAddressInfo* hints,
                            struct TinyCSocketAddressInfo** res)
{
    if ((node == NULL && service == NULL) || res == NULL)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int status = getaddrinfo(node, service, (const struct addrinfo*)hints, (struct addrinfo**)res);
    if (status == 0)
        return TINYCSOCKET_SUCCESS;
    else if (status == EAI_SYSTEM)
        return errno2retcode(errno);
    else
        return TINYCSOCKET_ERROR_UNKNOWN;
}

int tinycsocket_freeaddrinfo(struct TinyCSocketAddressInfo** addressinfo)
{
    if (addressinfo == NULL)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    freeaddrinfo((struct addrinfo*)*addressinfo);

    *addressinfo = NULL;
    return TINYCSOCKET_SUCCESS;
}

#endif
