#include "tinycsocket.h"

#ifdef TINYCSOCKET_USE_POSIX_IMPL

#include <arpa/inet.h> //inet_addr
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

#include <stdbool.h>

static const int NO_SOCKET = -1;
static const int FAILURE = -1;

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

int tinycsocket_create_socket(TinyCSocketCtx* socket_ctx)
{
    if (socket_ctx == NULL)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    socket_ctx->_socket = NO_SOCKET;

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_destroy_socket(TinyCSocketCtx* socket_ctx)
{
    if (socket_ctx == NULL)
        return TINYCSOCKET_SUCCESS;

    close(socket_ctx->_socket);
    socket_ctx->_socket = NO_SOCKET;

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_connect(TinyCSocketCtx* socket_ctx, const char* address, const char* port)
{
    if (socket_ctx == NULL || socket_ctx->_socket != NO_SOCKET)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }
    u_short port_number = atoi(port);
    if (port_number <= 0)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    struct addrinfo *serverinfo = NULL, *ptr = NULL, hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(address, port, &hints, &serverinfo) != 0)
    {
        return TINYCSOCKET_ERROR_ADDRESS_LOOKUP_FAILED;
    }

    // Try to connect
    bool did_connect = false;
    for (ptr = serverinfo; ptr != NULL; ptr = ptr->ai_next)
    {
        socket_ctx->_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket_ctx->_socket == NO_SOCKET)
        {
            continue;
        }

        if (connect(socket_ctx->_socket, ptr->ai_addr, ptr->ai_addrlen) == FAILURE)
        {
            close(socket_ctx->_socket);
            continue;
        }

        did_connect = true;
    }

    if (!did_connect)
    {
        return TINYCSOCKET_ERROR_CONNECTION_REFUSED;
    }
    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_send_data(TinyCSocketCtx* socket_ctx, const void* data, const size_t bytes)
{
    if (socket_ctx->_socket == NO_SOCKET)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    if (send(socket_ctx->_socket, data, bytes, 0) == FAILURE)
    {
        int send_error_code = errno;
        switch (send_error_code)
        {
            case ENOTCONN:
                return TINYCSOCKET_ERROR_NOT_CONNECTED;
            default:
                return TINYCSOCKET_ERROR_UNKNOWN;
        }
    }
    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_recieve_data(TinyCSocketCtx* socket_ctx,
                             void* buffer,
                             const size_t buffer_byte_size,
                             size_t* bytes_recieved)
{
    if (socket_ctx == NULL || buffer == NULL || buffer_byte_size == 0)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    int recv_status_code = recv(socket_ctx->_socket, buffer, buffer_byte_size, 0);
    if (recv_status_code == -1)
    {
        return TINYCSOCKET_ERROR_UNKNOWN;
    }
    if (bytes_recieved != NULL)
    {
        *bytes_recieved = recv_status_code;
    }
    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_bind(TinyCSocketCtx* socket_ctx, const char* address, const char* port)
{
    if (socket_ctx == NULL || socket_ctx->_socket != NO_SOCKET)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    struct addrinfo* result = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int addressInfo = getaddrinfo(NULL, port, &hints, &result);
    if (addressInfo != 0)
    {
        return TINYCSOCKET_ERROR_KERNEL;
    }

    bool did_connect = false;
    socket_ctx->_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_ctx->_socket == NO_SOCKET)
    {
        return TINYCSOCKET_ERROR_KERNEL;
    }

    if (bind(socket_ctx->_socket, result->ai_addr, (int)result->ai_addrlen) != FAILURE)
    {
        did_connect = true;
    }

    freeaddrinfo(result);

    if (!did_connect)
    {
        return TINYCSOCKET_ERROR_UNKNOWN;
    }

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_listen(TinyCSocketCtx* socket_ctx)
{
    if (socket_ctx == NULL || socket_ctx->_socket == NO_SOCKET)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    if (listen(socket_ctx->_socket, SOMAXCONN) == FAILURE)
    {
        return TINYCSOCKET_ERROR_UNKNOWN;
    }

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_accept(TinyCSocketCtx* listen_socket_ctx, TinyCSocketCtx* bind_socket_ctx)
{
    bind_socket_ctx->_socket = accept(listen_socket_ctx->_socket, NULL, NULL);

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_close_socket(TinyCSocketCtx* socket_ctx)
{
    if (socket_ctx == NULL)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }
    if (socket_ctx->_socket == NO_SOCKET)
    {
        return TINYCSOCKET_SUCCESS;
    }

    if (close(socket_ctx->_socket) == FAILURE)
    {
        socket_ctx->_socket = NO_SOCKET;
        return TINYCSOCKET_ERROR_UNKNOWN;
    }
    return TINYCSOCKET_SUCCESS;
}

#endif
