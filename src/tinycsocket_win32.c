#include "implementation_detector.h"
#ifdef USE_WIN32_IMPL

#include "tinycsocket.h"

// Close to POSIX but we may want to handle some things different

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>

typedef struct TinyCSocketCtxInternal
{
    SOCKET socket;
} TinyCSocketCtxInternal;

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

int tinycsocket_create_socket(TinyCSocketCtx** socket_ctx)
{
    // Must be a pointer to a null value, sent as a pointer argument
    if (socket_ctx == NULL || *socket_ctx != NULL)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    // Allocate socket data
    TinyCSocketCtxInternal** internal_socket_ctx_ptr = (TinyCSocketCtxInternal**)socket_ctx;
    *internal_socket_ctx_ptr = (TinyCSocketCtxInternal*)malloc(sizeof(TinyCSocketCtxInternal));
    if (*internal_socket_ctx_ptr == NULL)
    {
        return TINYCSOCKET_ERROR_MEMORY;
    }
    TinyCSocketCtxInternal* internal_ctx = (*internal_socket_ctx_ptr);

    // Init data
    tinycsocket_init();

    internal_ctx->socket = INVALID_SOCKET;

    // Do not create the win-socket now, wait for it in the listen or connect part

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_destroy_socket(TinyCSocketCtx** socket_ctx)
{
    if (socket_ctx == NULL)
        return TINYCSOCKET_SUCCESS;

    TinyCSocketCtxInternal* internal_ctx = (TinyCSocketCtxInternal*)(*socket_ctx);

    closesocket(internal_ctx->socket);

    free(*socket_ctx);
    *socket_ctx = NULL;
    tinycsocket_free();

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_connect(TinyCSocketCtx* socket_ctx, const char* address, const char* port)
{
    TinyCSocketCtxInternal* internal_ctx = socket_ctx;
    if (internal_ctx == NULL ||
        internal_ctx->socket != INVALID_SOCKET) // We create the win-socket here
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
        internal_ctx->socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (internal_ctx->socket == INVALID_SOCKET)
        {
            continue;
        }

        if (connect(internal_ctx->socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(internal_ctx->socket);
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
    TinyCSocketCtxInternal* internal_ctx = socket_ctx;
    if (internal_ctx->socket == INVALID_SOCKET)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    if (send(internal_ctx->socket, data, bytes, 0) == SOCKET_ERROR)
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
    if (socket_ctx == NULL || buffer == NULL || buffer_byte_size == 0)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    TinyCSocketCtxInternal* internal_ctx = socket_ctx;
    int recvResult = recv(internal_ctx->socket, buffer, buffer_byte_size, 0);
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
    TinyCSocketCtxInternal* internal_ctx = socket_ctx;

    if (internal_ctx == NULL || internal_ctx->socket != INVALID_SOCKET)
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
    internal_ctx->socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (internal_ctx->socket == INVALID_SOCKET)
    {
        return TINYCSOCKET_ERROR_KERNEL;
    }

    if (bind(internal_ctx->socket, result->ai_addr, (int)result->ai_addrlen) != SOCKET_ERROR)
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
    TinyCSocketCtxInternal* internal_ctx = socket_ctx;

    if (internal_ctx == NULL || internal_ctx->socket == INVALID_SOCKET)
    {
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
    }

    if (listen(internal_ctx->socket, SOMAXCONN) == SOCKET_ERROR)
    {
        return TINYCSOCKET_ERROR_UNKNOWN;
    }

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_accept(TinyCSocketCtx* listen_socket_ctx, TinyCSocketCtx* bind_socket_ctx)
{
    TinyCSocketCtxInternal* internal_listen_socket_ctx = listen_socket_ctx;
    TinyCSocketCtxInternal* internal_bind_socket_ctx = bind_socket_ctx;

    internal_bind_socket_ctx->socket = accept(internal_listen_socket_ctx->socket, NULL, NULL);

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_close_socket(TinyCSocketCtx* socket_ctx)
{
    TinyCSocketCtxInternal* internal_ctx = socket_ctx;
    if (internal_ctx == NULL || internal_ctx->socket == INVALID_SOCKET)
    {
        return TINYCSOCKET_SUCCESS;
    }

    if (closesocket(internal_ctx->socket) != S_OK)
    {
        internal_ctx->socket = INVALID_SOCKET;
        return TINYCSOCKET_ERROR_UNKNOWN;
    }
    return TINYCSOCKET_SUCCESS;
}

#endif
