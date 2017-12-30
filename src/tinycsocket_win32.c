#include "tinycsocket.h"

#ifdef TINYCSOCKET_USE_WIN32_IMPL

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

const tcs_socket TINYCSOCKET_NULLSOCKET = INVALID_SOCKET;

static int g_init_count = 0;

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

static inline int wsaerror2retcode(int wsa_error)
{
    switch (wsa_error)
    {
        case WSANOTINITIALISED:
            return TINYCSOCKET_ERROR_NOT_INITED;
        default:
            return TINYCSOCKET_ERROR_UNKNOWN;
    }
}

static inline int socketstatus2retcode(int status)
{
    if (status == 0)
    {
        return TINYCSOCKET_SUCCESS;
    }
    else if (status == SOCKET_ERROR)
    {
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }
    else
    {
        return TINYCSOCKET_ERROR_UNKNOWN;
    }
}

int tcs_init()
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

int tcs_free()
{
    g_init_count--;
    if (g_init_count <= 0)
    {
        WSACleanup();
    }
    return TINYCSOCKET_SUCCESS;
}

int tcs_new(tcs_socket* socket_ctx, int domain, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    UINT_PTR new_socket = socket(domain, type, protocol);

    if (new_socket != INVALID_SOCKET)
    {
        *socket_ctx = new_socket;
        return TINYCSOCKET_SUCCESS;
    }
    else
    {
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }
}

int tcs_bind(tcs_socket socket_ctx,
             const struct TinyCSocketAddress* address,
             socklen_t address_length)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int status = bind(socket_ctx, (const struct sockaddr*)address, (int)address_length);
    return socketstatus2retcode(status);
}

int tcs_connect(tcs_socket socket_ctx,
                const struct TinyCSocketAddress* address,
                socklen_t address_length)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int status = connect(socket_ctx, (const struct sockaddr*)address, address_length);
    return socketstatus2retcode(status);
}

int tcs_listen(tcs_socket socket_ctx, int backlog)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int status = listen(socket_ctx, (int)backlog);
    return socketstatus2retcode(status);
}

int tcs_accept(tcs_socket socket_ctx,
               tcs_socket* child_socket_ctx,
               struct TinyCSocketAddress* address,
               socklen_t* address_length)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET || child_socket_ctx == NULL ||
        *child_socket_ctx != TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int new_child_socket = accept(socket_ctx, (struct sockaddr*)address, address_length);
    if (new_child_socket != INVALID_SOCKET)
    {
        *child_socket_ctx = new_child_socket;
        return TINYCSOCKET_SUCCESS;
    }
    else
    {
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }
}

int tcs_send(tcs_socket socket_ctx,
             const uint8_t* buffer,
             size_t buffer_length,
             uint_fast32_t flags,
             size_t* bytes_sent)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int status = send(socket_ctx, (const char*)buffer, (int)buffer_length, (int)flags);
    if (status != SOCKET_ERROR)
    {
        if (bytes_sent != NULL)
            *bytes_sent = status;
        return TINYCSOCKET_SUCCESS;
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

    if (status != SOCKET_ERROR)
    {
        if (bytes_sent != NULL)
            *bytes_sent = status;
        return TINYCSOCKET_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return socketstatus2retcode(status);
    }
}

int tcs_recv(tcs_socket socket_ctx,
             uint8_t* buffer,
             size_t buffer_length,
             uint_fast32_t flags,
             size_t* bytes_recieved)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int status = recv(socket_ctx, (char*)buffer, (int)buffer_length, (int)flags);

    if (status != SOCKET_ERROR)
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = status;
        return TINYCSOCKET_SUCCESS;
    }
    else
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = 0;

        return socketstatus2retcode(status);
    }
}

int tcs_recvfrom(tcs_socket socket_ctx,
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

    if (status != SOCKET_ERROR)
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = status;
        return TINYCSOCKET_SUCCESS;
    }
    else
    {
        if (bytes_recieved != NULL)
            *bytes_recieved = 0;

        return socketstatus2retcode(status);
    }
}

int tcs_setsockopt(tcs_socket socket_ctx,
                   int_fast32_t level,
                   int_fast32_t option_name,
                   const void* option_value,
                   socklen_t option_length)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int status = setsockopt(
        socket_ctx, (int)level, (int)option_name, (const char*)option_value, (int)option_length);
    return socketstatus2retcode(status);
}

int tcs_shutdown(tcs_socket socket_ctx, int how)
{
    if (socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    // This translation makes it possible to use "binary OR" operations for how
    int win_how = how - 1;

    int status = shutdown(socket_ctx, win_how);
    return socketstatus2retcode(status);
}

int tcs_delete(tcs_socket* socket_ctx)
{
    if (socket_ctx == NULL || *socket_ctx == TINYCSOCKET_NULLSOCKET)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int status = closesocket(*socket_ctx);
    if (status != SOCKET_ERROR)
    {
        *socket_ctx = TINYCSOCKET_NULLSOCKET;
        return TINYCSOCKET_SUCCESS;
    }
    else
    {
        return socketstatus2retcode(status);
    }
}

int tcs_getaddrinfo(const char* node,
                    const char* service,
                    const struct tcs_addrinfo* hints,
                    struct tcs_addrinfo** res)
{
    if ((node == NULL && service == NULL) || res == NULL)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    int ret = getaddrinfo(node, service, (const struct addrinfo*)hints, (struct addrinfo**)res);

    if (ret != 0)
        return TINYCSOCKET_ERROR_ADDRESS_LOOKUP_FAILED;

    if (*res == NULL)
        return TINYCSOCKET_ERROR_UNKNOWN;

    return TINYCSOCKET_SUCCESS;
}

int tcs_freeaddrinfo(struct tcs_addrinfo** addressinfo)
{
    if (addressinfo == NULL)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    freeaddrinfo((PADDRINFOA)*addressinfo);
    *addressinfo = NULL;
    return TINYCSOCKET_SUCCESS;
}

#endif
