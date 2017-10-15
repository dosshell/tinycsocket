#include "tinycsocket.h"

#ifdef TINYCSOCKET_USE_WIN32_IMPL

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <limits.h>
#include <stdlib.h>

static int g_init_count = 0;

int tinycsocket_init()
{
    if (g_init_count <= 0)
    {
        WSADATA wsa_data;
        int wsa_startup_status_code = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (wsa_startup_status_code != 0)
        {
            return TINYCSOCKET_ERROR_KERNEL;
        }
    }
    ++g_init_count;
    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_free()
{
    g_init_count--;
    if (g_init_count <= 0)
    {
        WSACleanup();
    }
    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_create_socket(TinyCSocketCtx* socket_ctx)
{
    // Must be a pointer to a null value, sent as a pointer argument
    if (socket_ctx == NULL)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    // Init data
    tinycsocket_init();

    // Do not create the win-socket now, wait for it in the listen or connect part
    socket_ctx->_socket = INVALID_SOCKET;

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_destroy_socket(TinyCSocketCtx* socket_ctx)
{
    if (socket_ctx == NULL)
        return TINYCSOCKET_SUCCESS;

    int close_status = tinycsocket_close_socket(socket_ctx);
    if (close_status != TINYCSOCKET_SUCCESS)
    {
        return close_status;
    }

    int free_status = tinycsocket_free();
    if (free_status != TINYCSOCKET_SUCCESS)
    {
        return free_status;
    }

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_connect(TinyCSocketCtx* socket_ctx, const char* address, const char* port)
{
    if (socket_ctx == NULL || socket_ctx->_socket != INVALID_SOCKET)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = NULL;
    if (getaddrinfo(address, port, &hints, &result) != 0)
    {
        return TINYCSOCKET_ERROR_ADDRESS_LOOKUP_FAILED;
    }

    // Try to connect
    BOOL did_connect = FALSE;
    for (struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        socket_ctx->_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket_ctx->_socket == INVALID_SOCKET)
        {
            continue;
        }

        if (connect(socket_ctx->_socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(socket_ctx->_socket);
            continue;
        }

        did_connect = TRUE;
    }

    freeaddrinfo(result);

    if (!did_connect)
    {
        return TINYCSOCKET_ERROR_CONNECTION_REFUSED;
    }

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_send_data(TinyCSocketCtx* socket_ctx, const void* data, const size_t bytes)
{
    if (socket_ctx->_socket == INVALID_SOCKET || bytes > INT_MAX)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    if (send(socket_ctx->_socket, data, (int)bytes, 0) == SOCKET_ERROR)
    {
        int send_error_code = WSAGetLastError();
        switch (send_error_code)
        {
            case WSANOTINITIALISED:
                return TINYCSOCKET_ERROR_NOT_INITED;
            case WSAETIMEDOUT:
                return TINYCSOCKET_ERROR_TIMED_OUT;
            default:
                return TINYCSOCKET_ERROR_UNKNOWN;
        }
    }
    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_recieve_data(TinyCSocketCtx* socket_ctx,
                             void* buffer,
                             const size_t buffer_byte_size,
                             int* outBytesRecieved)
{
    if (socket_ctx == NULL || buffer == NULL || buffer_byte_size == 0 || buffer_byte_size > INT_MAX)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    int recvResult = recv(socket_ctx->_socket, buffer, (int)buffer_byte_size, 0);
    if (recvResult < 0)
    {
        return TINYCSOCKET_ERROR_UNKNOWN;
    }
    if (outBytesRecieved != NULL)
    {
        *outBytesRecieved = recvResult;
    }
    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_bind(TinyCSocketCtx* socket_ctx, const char* address, const char* port)
{
    if (socket_ctx == NULL || socket_ctx->_socket != INVALID_SOCKET)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    struct addrinfo* result = NULL;
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int address_info = getaddrinfo(NULL, port, &hints, &result);
    if (address_info != 0)
    {
        return TINYCSOCKET_ERROR_KERNEL;
    }

    BOOL did_connect = FALSE;
    socket_ctx->_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_ctx->_socket == INVALID_SOCKET)
    {
        return TINYCSOCKET_ERROR_KERNEL;
    }

    if (bind(socket_ctx->_socket, result->ai_addr, (int)result->ai_addrlen) != SOCKET_ERROR)
    {
        did_connect = TRUE;
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
    if (socket_ctx == NULL || socket_ctx->_socket == INVALID_SOCKET)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    if (listen(socket_ctx->_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        return TINYCSOCKET_ERROR_UNKNOWN;
    }

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_accept(TinyCSocketCtx* listen_socket_ctx, TinyCSocketCtx* bind_socket_ctx)
{
    if (listen_socket_ctx == NULL || bind_socket_ctx == NULL)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }
    bind_socket_ctx->_socket = accept(listen_socket_ctx->_socket, NULL, NULL);

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_close_socket(TinyCSocketCtx* socket_ctx)
{
    if (socket_ctx == NULL || socket_ctx->_socket == INVALID_SOCKET)
    {
        return TINYCSOCKET_SUCCESS;
    }

    if (closesocket(socket_ctx->_socket) != S_OK)
    {
        socket_ctx->_socket = INVALID_SOCKET;
        return TINYCSOCKET_ERROR_UNKNOWN;
    }
    return TINYCSOCKET_SUCCESS;
}

#endif
