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

#if !defined(NTDDI_VERSION) && !defined(_WIN32_WINNT) && !defined(WINVER)
#ifdef _WIN64
#define NTDDI_VERSION 0x05020000
#define _WIN32_WINNT 0x0502
#define WINVER 0x0502
#else
#define NTDDI_VERSION 0x05010300
#define _WIN32_WINNT 0x0501
#define WINVER 0x0501
#endif
#endif
#define WIN32_LEAN_AND_MEAN

#include "tinydatastructures.h"

// before windows.h
#include <winsock2.h> // sockets

#include <windows.h>

// after windows.h
#include <iphlpapi.h> // GetAdaptersAddresses
#include <ws2tcpip.h> // getaddrinfo

#include <stdlib.h> // Malloc for GetAdaptersAddresses

#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#endif

// Forwards declaration due to winver dispatch
// We will dispatch at lib_init() which OS functions to call depending on OS support

TDS_ULIST_IMPL(SOCKET, soc)

// Needs to be compatible with fd_set, hopefully this works
struct tcs_fd_set
{
    u_int fd_count;
    SOCKET fd_array[1]; // dynamic memory hack that is compatible with Win32 API fd_set
};

struct tcs_usr_data_vector
{
    SOCKET* key;
    void** value;
    size_t capacity_bytes;
    size_t count;
};

struct TcsPool
{
    UList_soc read_sockets;
    UList_soc write_sockets;
    UList_soc error_sockets;
    struct tcs_usr_data_vector user_data;
};

static const size_t TCS_POOL_CAPACITY_STEP = 1024;

const TcsSocket TCS_NULLSOCKET = INVALID_SOCKET;
const int TCS_INF = -1;

// Addresses
const uint32_t TCS_ADDRESS_ANY_IP4 = INADDR_ANY;
const uint32_t TCS_ADDRESS_LOOPBACK_IP4 = INADDR_LOOPBACK;
const uint32_t TCS_ADDRESS_BROADCAST_IP4 = INADDR_BROADCAST;
const uint32_t TCS_ADDRESS_NONE_IP4 = INADDR_NONE;

// Type
const int TCS_SOCK_STREAM = SOCK_STREAM;
const int TCS_SOCK_DGRAM = SOCK_DGRAM;

// Protocol
const int TCS_IPPROTO_TCP = IPPROTO_TCP;
const int TCS_IPPROTO_UDP = IPPROTO_UDP;

// Flags
const uint32_t TCS_AI_PASSIVE = AI_PASSIVE;

// Recv flags
const uint32_t TCS_MSG_PEEK = MSG_PEEK;
const uint32_t TCS_MSG_OOB = MSG_OOB;
const uint32_t TCS_MSG_WAITALL = 0x8; // Binary compatible when it does not exist

// Send flags
const uint32_t TCS_MSG_SENDALL = 0x80000000;

// Backlog
const int TCS_BACKLOG_SOMAXCONN = SOMAXCONN;

// Option levels
const int TCS_SOL_SOCKET = SOL_SOCKET;
const int TCS_SOL_IP = IPPROTO_IP;

// Socket options
const int TCS_SO_BROADCAST = SO_BROADCAST;
const int TCS_SO_KEEPALIVE = SO_KEEPALIVE;
const int TCS_SO_LINGER = SO_LINGER;
const int TCS_SO_REUSEADDR = SO_REUSEADDR;
const int TCS_SO_RCVBUF = SO_RCVBUF;
const int TCS_SO_RCVTIMEO = SO_RCVTIMEO;
const int TCS_SO_SNDBUF = SO_SNDBUF;
const int TCS_SO_OOBINLINE = SO_OOBINLINE;

// IP options
const int TCS_SO_IP_NODELAY = TCP_NODELAY;
const int TCS_SO_IP_MEMBERSHIP_ADD = IP_ADD_MEMBERSHIP;
const int TCS_SO_IP_MEMBERSHIP_DROP = IP_DROP_MEMBERSHIP;
const int TCS_SO_IP_MULTICAST_LOOP = IP_MULTICAST_LOOP;

int g_init_count = 0;

static TcsReturnCode wsaerror2retcode(int wsa_error)
{
    switch (wsa_error)
    {
        case WSANOTINITIALISED:
            return TCS_ERROR_NOT_INITED;
        case WSAEWOULDBLOCK:
            return TCS_ERROR_WOULD_BLOCK;
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

static TcsReturnCode family2native(const TcsAddressFamily family, short* native_family)
{
    static short lut[TCS_AF_LENGTH] = {AF_UNSPEC, AF_INET, AF_INET6};
    if (native_family == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (family >= TCS_AF_LENGTH || family < 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    *native_family = lut[family];
    return TCS_SUCCESS;
}

static TcsReturnCode sockaddr2native(const struct TcsAddress* in_addr, PSOCKADDR out_addr, int* out_addrlen)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->family == TCS_AF_IP4)
    {
        PSOCKADDR_IN addr = (PSOCKADDR_IN)out_addr;
        addr->sin_family = (ADDRESS_FAMILY)AF_INET;
        addr->sin_port = htons((USHORT)in_addr->data.af_inet.port);
        addr->sin_addr.S_un.S_addr = htonl((ULONG)in_addr->data.af_inet.address);

        if (out_addrlen != NULL)
            *out_addrlen = sizeof(SOCKADDR_IN);

        return TCS_SUCCESS;
    }
    else if (in_addr->family == TCS_AF_IP6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (in_addr->family == TCS_AF_ANY)
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
        out_addr->family = TCS_AF_IP4;
        out_addr->data.af_inet.port = ntohs((uint16_t)addr->sin_port);
        out_addr->data.af_inet.address = ntohl((uint32_t)addr->sin_addr.S_un.S_addr);
    }
    else if (in_addr->sa_family == AF_INET6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (in_addr->sa_family == AF_UNSPEC)
    {
        return TCS_ERROR_INVALID_ARGUMENT;
    }
    else
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }

    return TCS_SUCCESS;
}

TcsReturnCode tcs_lib_init()
{
    if (g_init_count <= 0)
    {
        WSADATA wsa_data;
        int wsa_startup_status_code = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (wsa_startup_status_code != 0)
            return TCS_ERROR_KERNEL;
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

TcsReturnCode tcs_create_ext(TcsSocket* socket_ctx, TcsAddressFamily family, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    short native_family = AF_UNSPEC;
    TcsReturnCode sts = family2native(family, &native_family);
    if (sts != TCS_SUCCESS)
        return sts;

    TcsSocket new_socket = socket(native_family, type, protocol);

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

TcsReturnCode tcs_bind_address(TcsSocket socket_ctx, const struct TcsAddress* address)
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

TcsReturnCode tcs_connect_address(TcsSocket socket_ctx, const struct TcsAddress* address)
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

    // Send all
    if (flags & TCS_MSG_SENDALL)
    {
        uint32_t new_flags = flags & ~TCS_MSG_SENDALL; // For recursive call
        size_t left = buffer_size;
        size_t sent = 0;

        while (left > 0)
        {
            int sts = tcs_send(socket_ctx, buffer, buffer_size, new_flags, &sent);
            if (sts != TCS_SUCCESS)
                return sts;
            left -= sent;
        }
        return TCS_SUCCESS;
    }
    else // Send
    {
        int send_status = send(socket_ctx, (const char*)buffer, (int)buffer_size, (int)flags);
        if (send_status != SOCKET_ERROR)
        {
            if (bytes_sent != NULL)
                *bytes_sent = (size_t)send_status;
            return TCS_SUCCESS;
        }
        else
        {
            if (bytes_sent != NULL)
                *bytes_sent = 0;

            return socketstatus2retcode(send_status);
        }
    }
}

TcsReturnCode tcs_send_to(TcsSocket socket_ctx,
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
            *bytes_sent = (size_t)sendto_status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;

        return socketstatus2retcode(sendto_status);
    }
}

TcsReturnCode tcs_receive(TcsSocket socket_ctx,
                          uint8_t* buffer,
                          size_t buffer_size,
                          uint32_t flags,
                          size_t* bytes_received)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

#if WINVER <= 0x501
    if (flags & TCS_MSG_WAITALL)
    {
        uint32_t new_flags = flags & ~TCS_MSG_WAITALL; // Unmask for recursive call
        size_t received_so_far = 0;
        while (received_so_far < buffer_size)
        {
            size_t received_now = 0;
            uint8_t* cursor = buffer + received_so_far;
            size_t left = buffer_size - received_so_far;
            int sts = tcs_receive(socket_ctx, cursor, left, new_flags, &received_now);
            if (sts != TCS_SUCCESS)
                return sts;
            received_so_far += received_now;
        }
        if (bytes_received != NULL)
            *bytes_received = 0;
        return TCS_SUCCESS;
    }
#endif

    int recv_status = recv(socket_ctx, (char*)buffer, (int)buffer_size, (int)flags);

    if (recv_status == 0)
    {
        return TCS_ERROR_SOCKET_CLOSED;
    }
    else if (recv_status != SOCKET_ERROR)
    {
        if (bytes_received != NULL)
            *bytes_received = (size_t)recv_status;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_received != NULL)
            *bytes_received = 0;

        return socketstatus2retcode(recv_status);
    }
}

TcsReturnCode tcs_receive_from(TcsSocket socket_ctx,
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
            *bytes_received = (size_t)recvfrom_status;

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

TcsReturnCode tcs_pool_create(struct TcsPool** pool)
{
    if (pool == NULL || *pool != NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    *pool = malloc(sizeof(struct TcsPool));
    if (*pool == NULL)
        return TCS_ERROR_MEMORY;
    memset(*pool, 0, sizeof(struct TcsPool)); // Just to be safe
    int sts_read_array = ulist_soc_create(&(*pool)->read_sockets, 64);
    int sts_write_array = ulist_soc_create(&(*pool)->write_sockets, 64);
    int sts_error_array = ulist_soc_create(&(*pool)->error_sockets, 64);

    // TODO(markusl): Create a common vector structure
    (*pool)->user_data.key = malloc(sizeof(SOCKET) * TCS_POOL_CAPACITY_STEP);
    (*pool)->user_data.value = malloc(sizeof(void*) * TCS_POOL_CAPACITY_STEP);
    (*pool)->user_data.count = 0;
    (*pool)->user_data.capacity_bytes = TCS_POOL_CAPACITY_STEP;
    TcsReturnCode sts_usrdata = TCS_SUCCESS;
    if ((*pool)->user_data.key == NULL || (*pool)->user_data.value == NULL)
        sts_usrdata = TCS_ERROR_MEMORY;

    if (sts_usrdata != TCS_SUCCESS || sts_read_array != 0 || sts_write_array != TCS_SUCCESS ||
        sts_error_array != TCS_SUCCESS)
    {
        tcs_pool_destory(pool);
        return TCS_ERROR_MEMORY;
    }
    return TCS_SUCCESS;
}

TcsReturnCode tcs_pool_destory(struct TcsPool** pool)
{
    if (pool == NULL || *pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // Free away!
    ulist_soc_destroy(&(*pool)->read_sockets);
    ulist_soc_destroy(&(*pool)->write_sockets);
    ulist_soc_destroy(&(*pool)->error_sockets);

    free((*pool)->user_data.key);
    free((*pool)->user_data.value);
    free(*pool);
    *pool = NULL;

    return TCS_SUCCESS;
}

TcsReturnCode tcs_pool_add(struct TcsPool* pool,
                           TcsSocket socket,
                           void* user_data,
                           bool poll_can_read,
                           bool poll_can_write,
                           bool poll_error)
{
    if (pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (!poll_can_read && !poll_can_write && !poll_error)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (pool->user_data.count == pool->user_data.capacity_bytes)
    {
        size_t new_capacity = pool->user_data.capacity_bytes + TCS_POOL_CAPACITY_STEP;
        SOCKET* new_key = realloc(pool->user_data.key, sizeof(SOCKET) * new_capacity);
        if (new_key == NULL)
            return TCS_ERROR_MEMORY;
        pool->user_data.key = new_key;

        void** new_value = realloc(pool->user_data.value, sizeof(SOCKET) * new_capacity);
        if (new_value == NULL)
        {
            new_key = realloc(pool->user_data.key, sizeof(SOCKET) * pool->user_data.capacity_bytes);
            if (new_key != NULL)
                pool->user_data.key = new_key;
            return TCS_ERROR_MEMORY;
        }
        pool->user_data.value = new_value;
        pool->user_data.capacity_bytes = new_capacity;
    }
    pool->user_data.key[pool->user_data.count] = socket;
    pool->user_data.value[pool->user_data.count] = user_data;
    pool->user_data.count++;

    if (poll_can_read)
    {
        int sts = ulist_soc_add_one(&pool->read_sockets, socket);
        if (sts != 0)
        {
            return TCS_ERROR_MEMORY;
        }
    }
    if (poll_can_write)
    {
        int sts = ulist_soc_add_one(&pool->write_sockets, socket);
        if (sts != 0)
        {
            ulist_soc_pop(&pool->read_sockets, NULL);
            return TCS_ERROR_MEMORY;
        }
    }
    if (poll_error)
    {
        int sts = ulist_soc_add_one(&pool->error_sockets, socket);
        if (sts != 0)
        {
            ulist_soc_pop(&pool->read_sockets, NULL);
            ulist_soc_pop(&pool->write_sockets, NULL);
            return TCS_ERROR_MEMORY;
        }
    }
    return TCS_SUCCESS;
}

TcsReturnCode tcs_pool_remove(struct TcsPool* pool, TcsSocket socket)
{
    for (size_t i = 0; i < pool->read_sockets.count; ++i)
    {
        if (pool->read_sockets.data[i] == socket)
        {
            ulist_soc_remove_one(&pool->read_sockets, i);
            break;
        }
    }
    for (size_t i = 0; i < pool->write_sockets.count; ++i)
    {
        if (pool->write_sockets.data[i] == socket)
        {
            ulist_soc_remove_one(&pool->write_sockets, i);
            break;
        }
    }
    for (size_t i = 0; i < pool->error_sockets.count; ++i)
    {
        if (pool->error_sockets.data[i] == socket)
        {
            ulist_soc_remove_one(&pool->error_sockets, i);
            break;
        }
    }
    for (size_t i = 0; i < pool->user_data.count; ++i)
    {
        if (pool->user_data.key[i] == socket)
        {
            pool->user_data.count--;
            pool->user_data.key[i] = pool->user_data.key[pool->user_data.count];
            pool->user_data.value[i] = pool->user_data.value[pool->user_data.count];
        }
    }

    return TCS_SUCCESS;
}

TcsReturnCode tcs_pool_poll(struct TcsPool* pool,
                            struct TcsPollEvent* events,
                            size_t events_capacity,
                            size_t* events_populated,
                            int64_t timeout_in_ms)
{
    if (pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (timeout_in_ms != TCS_INF && (timeout_in_ms > 0xffffffff || timeout_in_ms < 0))
        return TCS_ERROR_INVALID_ARGUMENT;

    // Copy fds
    //
    // Allocate so that we can access fd_array out of nominel bounds
    // We need this hack to be able to use dynamic memory for select
    const size_t data_offset = offsetof(struct tcs_fd_set, fd_array);

    struct tcs_fd_set* rfds_cpy = malloc(data_offset + sizeof(SOCKET) * pool->read_sockets.count);
    struct tcs_fd_set* wfds_cpy = malloc(data_offset + sizeof(SOCKET) * pool->write_sockets.count);
    struct tcs_fd_set* efds_cpy = malloc(data_offset + sizeof(SOCKET) * pool->error_sockets.count);
    if (rfds_cpy == NULL || wfds_cpy == NULL || efds_cpy == NULL)
    {
        free(rfds_cpy);
        free(wfds_cpy);
        free(efds_cpy);

        return TCS_ERROR_MEMORY;
    }
    rfds_cpy->fd_count = (u_int)pool->read_sockets.count;
    wfds_cpy->fd_count = (u_int)pool->write_sockets.count;
    efds_cpy->fd_count = (u_int)pool->error_sockets.count;

    memcpy(rfds_cpy->fd_array, pool->read_sockets.data, sizeof(SOCKET) * pool->read_sockets.count);
    memcpy(wfds_cpy->fd_array, pool->write_sockets.data, sizeof(SOCKET) * pool->write_sockets.count);
    memcpy(efds_cpy->fd_array, pool->error_sockets.data, sizeof(SOCKET) * pool->error_sockets.count);

    // Run select
    struct timeval* t_ptr = NULL;
    struct timeval t = {0};
    if (timeout_in_ms != TCS_INF)
    {
        t.tv_sec = (long)(timeout_in_ms / 1000);
        t.tv_usec = (long)(timeout_in_ms % 1000) * 1000;
        t_ptr = &t;
    }
    int no = select(IGNORE, (fd_set*)rfds_cpy, (fd_set*)wfds_cpy, (fd_set*)efds_cpy, t_ptr);

    size_t events_added = 0;

    if (no > 0)
    {
        memset(events, 0, sizeof(struct TcsPollEvent) * min((size_t)no, events_capacity));
        for (u_int n = 0; n < rfds_cpy->fd_count && events_added < events_capacity; ++n)
        {
            events[events_added].socket = rfds_cpy->fd_array[n];
            events[events_added].can_read = true;
            for (size_t i = 0; i < pool->user_data.count; ++i)
            {
                if (events[events_added].socket == pool->user_data.key[i])
                {
                    events[events_added].user_data = pool->user_data.value[i];
                    pool->user_data.key[pool->user_data.count] = pool->user_data.key[i];
                    pool->user_data.value[pool->user_data.count] = pool->user_data.value[i];
                    pool->user_data.count--;
                    break;
                }
            }
            events_added++;
        }
        for (u_int n = 0; n < wfds_cpy->fd_count && events_added < events_capacity; ++n)
        {
            // Check already added events
            size_t new_n = events_added;
            for (size_t m = 0; m < events_added; ++m)
            {
                if (events[m].socket == wfds_cpy->fd_array[n])
                {
                    new_n = m;
                    break;
                }
            }
            events[new_n].can_write = true;
            // Check for new events
            if (events_added == new_n)
            {
                events[new_n].socket = wfds_cpy->fd_array[n];

                for (size_t i = 0; i < pool->user_data.count; ++i)
                {
                    if (events[new_n].socket == pool->user_data.key[i])
                    {
                        events[new_n].user_data = pool->user_data.value[i];
                        pool->user_data.key[pool->user_data.count] = pool->user_data.key[i];
                        pool->user_data.value[pool->user_data.count] = pool->user_data.value[i];
                        pool->user_data.count--;
                        break;
                    }
                }
                events_added++;
            }
        }
        for (u_int n = 0; n < efds_cpy->fd_count && events_added < events_capacity; ++n)
        {
            // Check already added events
            size_t new_n = events_added;
            for (size_t m = 0; m < events_added; ++m)
            {
                if (events[m].socket == efds_cpy->fd_array[n])
                {
                    new_n = m;
                    break;
                }
            }
            // Check for new events
            events[new_n].error = TCS_ERROR_NOT_IMPLEMENTED; //TODO(markusl): This does not feel right
            if (events_added == new_n)
            {
                events[new_n].socket = efds_cpy->fd_array[n];

                for (size_t i = 0; i < pool->user_data.count; ++i)
                {
                    if (events[new_n].socket == pool->user_data.key[i])
                    {
                        events[new_n].user_data = pool->user_data.value[i];
                        pool->user_data.key[pool->user_data.count] = pool->user_data.key[i];
                        pool->user_data.value[pool->user_data.count] = pool->user_data.value[i];
                        pool->user_data.count--;
                        break;
                    }
                }

                events_added++;
            }
        }
        bool is_minimum = pool->user_data.capacity_bytes == TCS_POOL_CAPACITY_STEP;
        bool should_shrink =
            pool->user_data.capacity_bytes >= pool->user_data.count + 2 * TCS_POOL_CAPACITY_STEP; // hysteresis
        if (!is_minimum && should_shrink)
        {
            size_t new_capacity = pool->user_data.capacity_bytes - 2 * TCS_POOL_CAPACITY_STEP;
            TcsSocket* new_key = realloc(pool->user_data.key, new_capacity);
            if (new_key == NULL)
                return TCS_ERROR_MEMORY; // Should not happen since we are shrinking

            pool->user_data.key = new_key;

            void** new_value = realloc(pool->user_data.value, new_capacity);
            if (new_value == NULL)
            {
                new_key = realloc(pool->user_data.key, pool->user_data.capacity_bytes);
                if (new_key != NULL) // Should not happen since we are shrinking
                    pool->user_data.key = new_key;
                return TCS_ERROR_MEMORY;
            }

            pool->user_data.capacity_bytes = new_capacity;
            pool->user_data.value = new_value;
            pool->user_data.key = new_key;
        }
    }

    if (events_populated != NULL)
        *events_populated = events_added;

    // Clean up
    free(rfds_cpy);
    free(wfds_cpy);
    free(efds_cpy);

    if (no == 0)
    {
        return TCS_ERROR_TIMED_OUT;
    }
    if (no == SOCKET_ERROR)
    {
        int error_code = WSAGetLastError();
        return wsaerror2retcode(error_code);
    }

    return TCS_SUCCESS;
}

TcsReturnCode tcs_set_option(TcsSocket socket_ctx,
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

TcsReturnCode tcs_get_option(TcsSocket socket_ctx,
                             int32_t level,
                             int32_t option_name,
                             void* option_value,
                             size_t* option_size)
{
    if (socket_ctx == TCS_NULLSOCKET || option_value == NULL || option_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    int sockopt_status = getsockopt(socket_ctx, (int)level, (int)option_name, (char*)option_value, (int*)option_size);
    return socketstatus2retcode(sockopt_status);
}

TcsReturnCode tcs_shutdown(TcsSocket socket_ctx, TcsSocketDirection direction)
{
    const int LUT[] = {SD_RECEIVE, SD_SEND, SD_BOTH};

    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    const int how = LUT[direction];
    int shutdown_status = shutdown(socket_ctx, how);
    return socketstatus2retcode(shutdown_status);
}

TcsReturnCode tcs_destroy(TcsSocket* socket_ctx)
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

TcsReturnCode tcs_resolve_hostname(const char* hostname,
                                   TcsAddressFamily address_family,
                                   struct TcsAddress found_addresses[],
                                   size_t found_addresses_length,
                                   size_t* no_of_found_addresses)
{
    if (hostname == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (found_addresses == NULL && no_of_found_addresses == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (no_of_found_addresses != NULL)
        *no_of_found_addresses = 0;

    ADDRINFOA native_hints = {0};
    TcsReturnCode sts = family2native(address_family, (short*)&native_hints.ai_family);
    if (sts != TCS_SUCCESS)
        return sts;

    PADDRINFOA native_addrinfo_list = NULL;
    int getaddrinfo_status = getaddrinfo(hostname, NULL, &native_hints, &native_addrinfo_list);
    if (getaddrinfo_status != 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    if (native_addrinfo_list == NULL)
        return TCS_ERROR_UNKNOWN;

    size_t i = 0;
    if (found_addresses == NULL)
    {
        for (PADDRINFOA iter = native_addrinfo_list; iter != NULL; iter = iter->ai_next)
            i++;
    }
    else
    {
        for (PADDRINFOA iter = native_addrinfo_list; iter != NULL && i < found_addresses_length; iter = iter->ai_next)
        {
            TcsReturnCode address_convert_status = native2sockaddr(iter->ai_addr, &found_addresses[i]);
            if (address_convert_status != TCS_SUCCESS)
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

TcsReturnCode tcs_local_interfaces(struct TcsInterface found_interfaces[],
                                   size_t found_interfaces_length,
                                   size_t* no_of_found_interfaces)
{
    if (found_interfaces == NULL && no_of_found_interfaces == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (found_interfaces == NULL && found_interfaces_length != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (no_of_found_interfaces != NULL)
        *no_of_found_interfaces = 0;

    const int MAX_TRIES = 3;
    ULONG adapeters_buffer_size = 15000;
    PIP_ADAPTER_ADDRESSES adapters = NULL;
    ULONG adapter_sts = ERROR_NO_DATA;
    for (int i = 0; i < MAX_TRIES; ++i)
    {
        adapters = malloc(adapeters_buffer_size);
        adapter_sts = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapters, &adapeters_buffer_size);
        if (adapter_sts == ERROR_BUFFER_OVERFLOW)
        {
            free(adapters);
            adapters = NULL;
        }
        else
        {
            break;
        }
    }
    if (adapter_sts != NO_ERROR)
    {
        if (adapters != NULL)
            free(adapters);
        return TCS_ERROR_UNKNOWN;
    }

    size_t i = 0;
    for (PIP_ADAPTER_ADDRESSES iter = adapters;
         iter != NULL && (found_interfaces == NULL || i < found_interfaces_length);
         iter = iter->Next)
    {
        if (iter->OperStatus != IfOperStatusUp)
            continue;
        for (PIP_ADAPTER_UNICAST_ADDRESS address_iter = iter->FirstUnicastAddress; address_iter != NULL;
             address_iter = address_iter->Next)
        {
            struct TcsAddress t;
            if (native2sockaddr(address_iter->Address.lpSockaddr, &t) != TCS_SUCCESS)
                continue;
            if (found_interfaces != NULL)
            {
                found_interfaces[i].address = t;
                memset(found_interfaces[i].name, '\0', 32);
                WideCharToMultiByte(CP_UTF8, 0, iter->FriendlyName, -1, found_interfaces[i].name, 31, NULL, NULL);
            }
            ++i;
        }
    }
    if (adapters != NULL)
        free(adapters);
    if (no_of_found_interfaces != NULL)
        *no_of_found_interfaces = i;

    return TCS_SUCCESS;
}

TcsReturnCode tcs_set_linger(TcsSocket socket_ctx, bool do_linger, int timeout_seconds)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {0};
    l.l_onoff = (u_short)do_linger;
    l.l_linger = (u_short)timeout_seconds;
    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, sizeof(l));
}

TcsReturnCode tcs_get_linger(TcsSocket socket_ctx, bool* do_linger, int* timeout_seconds)
{
    if (socket_ctx == TCS_NULLSOCKET || (do_linger == NULL && timeout_seconds == NULL))
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {0};
    size_t l_size = sizeof(l);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, &l_size);
    if (sts == TCS_SUCCESS)
    {
        if (do_linger)
            *do_linger = l.l_onoff;
        if (timeout_seconds)
            *timeout_seconds = l.l_linger;
    }

    return sts;
}

TcsReturnCode tcs_set_receive_timeout(TcsSocket socket_ctx, int timeout_ms)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));
}

TcsReturnCode tcs_get_receive_timeout(TcsSocket socket_ctx, int* timeout_ms)
{
    if (socket_ctx == TCS_NULLSOCKET || timeout_ms == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    DWORD t = 0;
    size_t t_size = sizeof(t);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &t, &t_size);

    if (sts == TCS_SUCCESS)
    {
        *timeout_ms = (int)t;
    }
    return sts;
}

TcsReturnCode tcs_set_ip_multicast_add(TcsSocket socket_ctx,
                                       const struct TcsAddress* local_address,
                                       const struct TcsAddress* multicast_address)
{
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // TODO(markusl): Add ipv6 support
    if (multicast_address->family != TCS_AF_IP4)
        return TCS_ERROR_NOT_IMPLEMENTED;

    struct ip_mreq imr = {0};
    imr.imr_multiaddr.s_addr = htonl(multicast_address->data.af_inet.address);
    if (local_address != NULL)
        imr.imr_interface.s_addr = htonl(local_address->data.af_inet.address);

    return tcs_set_option(socket_ctx, TCS_SOL_IP, TCS_SO_IP_MEMBERSHIP_ADD, &imr, sizeof(imr));
}

TcsReturnCode tcs_set_ip_multicast_drop(TcsSocket socket_ctx,
                                        const struct TcsAddress* local_address,
                                        const struct TcsAddress* multicast_address)
{
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // TODO(markusl): Add ipv6 support
    if (multicast_address->family != TCS_AF_IP4)
        return TCS_ERROR_NOT_IMPLEMENTED;

    struct ip_mreq imr = {0};
    imr.imr_multiaddr.s_addr = htonl(multicast_address->data.af_inet.address);
    if (local_address != NULL)
        imr.imr_interface.s_addr = htonl(local_address->data.af_inet.address);

    return tcs_set_option(socket_ctx, TCS_SOL_IP, TCS_SO_IP_MEMBERSHIP_DROP, &imr, sizeof(imr));
}
#endif
