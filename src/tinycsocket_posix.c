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

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef TINYCSOCKET_INTERNAL_H_
#include "tinycsocket_internal.h"
#endif
#ifdef TINYCSOCKET_USE_POSIX_IMPL

#ifdef DO_WRAP
#include "dbg_wrap.h"
#endif

#include <errno.h>
#include <ifaddrs.h>     // getifaddr()
#include <limits.h>      // IOV_MAX
#include <net/if.h>      // Flags for ifaddrs (?)
#include <netdb.h>       // Protocols and custom return codes
#include <netinet/in.h>  // IPPROTO_XXP
#include <netinet/tcp.h> // TCP_NODELAY
#include <poll.h>        // poll()
#include <stdlib.h>      // malloc()/free()
#include <string.h>      // strcpy, memset
#include <sys/ioctl.h>   // Flags for ifaddrs
#include <sys/socket.h>  // pretty much everything
#include <sys/types.h>   // POSIX.1 compatibility
#include <unistd.h>      // close()

const TcsSocket TCS_NULLSOCKET = -1;
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
const uint32_t TCS_MSG_WAITALL = MSG_WAITALL;

// Send flags
const uint32_t TCS_MSG_SENDALL = 0x80000000;

// Backlog
const int TCS_BACKLOG_SOMAXCONN = SOMAXCONN;

// Option levels
const int TCS_SOL_SOCKET = SOL_SOCKET;
const int TCS_SOL_IP = IPPROTO_IP; // Same as SOL_IP but crossplatform (BSD)

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

// Default flags
const int TCS_DEFAULT_SEND_FLAGS = MSG_NOSIGNAL;
const int TCS_DEFAULT_RECV_FLAGS = 0;

static TcsReturnCode family2native(const TcsAddressFamily family, sa_family_t* native_family)
{
    static uint16_t lut[TCS_AF_LENGTH] = {AF_UNSPEC, AF_INET, AF_INET6};
    if (native_family == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (family >= TCS_AF_LENGTH || family < 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    *native_family = lut[family];
    return TCS_SUCCESS;
}

static TcsReturnCode errno2retcode(int error_code)
{
    switch (error_code)
    {
        case ECONNREFUSED:
            return TCS_ERROR_CONNECTION_REFUSED;
        case EAGAIN:
            return TCS_ERROR_TIMED_OUT;
        case EINVAL:
            return TCS_ERROR_INVALID_ARGUMENT;
        case ENOMEM:
            return TCS_ERROR_MEMORY;
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

    if (in_addr->family == TCS_AF_IP4)
    {
        // TODO: Fix UB aliasing
        struct sockaddr_in* addr = (struct sockaddr_in*)out_addr;
        addr->sin_family = (sa_family_t)AF_INET;
        addr->sin_port = (in_port_t)htons(in_addr->data.af_inet.port);
        addr->sin_addr.s_addr = (in_addr_t)htonl(in_addr->data.af_inet.address);

        if (out_addrlen != NULL)
            *out_addrlen = sizeof(struct sockaddr_in);

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

static TcsReturnCode native2sockaddr(const struct sockaddr* in_addr, struct TcsAddress* out_addr)
{
    if (in_addr == NULL || out_addr == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (in_addr->sa_family == AF_INET)
    {
        // False positive alignment warning, the creator of the sockaddr is responsible for the alignment.
        struct sockaddr_in* addr = (struct sockaddr_in*)in_addr;
        out_addr->family = TCS_AF_IP4;
        out_addr->data.af_inet.port = ntohs((uint16_t)addr->sin_port);
        out_addr->data.af_inet.address = ntohl((uint32_t)addr->sin_addr.s_addr);
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
    // Not needed for posix
    return TCS_SUCCESS;
}

TcsReturnCode tcs_lib_free()
{
    // Not needed for posix
    return TCS_SUCCESS;
}

TcsReturnCode tcs_create_ext(TcsSocket* socket_ctx, TcsAddressFamily family, int type, int protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;
    sa_family_t native_family;
    TcsReturnCode sts = family2native(family, &native_family);
    if (sts != TCS_SUCCESS)
        return sts;
    *socket_ctx = socket(native_family, type, protocol);

    if (*socket_ctx != -1) // Same as TCS_NULLSOCKET
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_bind_address(TcsSocket socket_ctx, const struct TcsAddress* address)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    socklen_t addrlen = 0;
    TcsReturnCode convert_address_status = sockaddr2native(address, (struct sockaddr*)&native_sockaddr, &addrlen);
    if (convert_address_status != TCS_SUCCESS)
        return convert_address_status;
    if (bind(socket_ctx, (struct sockaddr*)&native_sockaddr, addrlen) != -1)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_connect_address(TcsSocket socket_ctx, const struct TcsAddress* address)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
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

    struct sockaddr_storage native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    socklen_t address_length = sizeof native_sockaddr;

    *child_socket_ctx = accept(socket_ctx, (struct sockaddr*)&native_sockaddr, &address_length);
    if (*child_socket_ctx != -1)
    {
        if (address != NULL)
        {
            TcsReturnCode convert_address_status = native2sockaddr((struct sockaddr*)&native_sockaddr, address);
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

    if (bytes_sent != NULL)
        *bytes_sent = 0;

    if (buffer == NULL || buffer_size == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    // Send all
    if (flags & TCS_MSG_SENDALL)
    {
        uint32_t new_flags = flags & ~TCS_MSG_SENDALL; // For recursive call
        size_t left = buffer_size;
        const uint8_t* iterator = buffer;

        while (left > 0)
        {
            size_t sent = 0;
            TcsReturnCode sts = tcs_send(socket_ctx, iterator, left, new_flags, &sent);
            if (bytes_sent != NULL)
                *bytes_sent += sent;
            if (sts != TCS_SUCCESS)
                return sts;
            left -= sent;
            iterator += sent;
        }
        return TCS_SUCCESS;
    }
    else // Send
    {
        ssize_t send_status = send(socket_ctx, (const char*)buffer, buffer_size, TCS_DEFAULT_SEND_FLAGS | (int)flags);
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

    if (flags & TCS_MSG_SENDALL)
        return TCS_ERROR_NOT_IMPLEMENTED;

    struct sockaddr_storage native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    socklen_t address_length = 0;
    TcsReturnCode convert_addr_status =
        sockaddr2native(destination_address, (struct sockaddr*)&native_sockaddr, &address_length);
    if (convert_addr_status != TCS_SUCCESS)
        return convert_addr_status;

    ssize_t sendto_status = sendto(socket_ctx,
                                   (const char*)buffer,
                                   buffer_size,
                                   TCS_DEFAULT_SEND_FLAGS | (int)flags,
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

TcsReturnCode tcs_sendv(TcsSocket socket_ctx,
                        const struct TcsBuffer* buffers,
                        size_t buffer_count,
                        uint32_t flags,
                        size_t* bytes_sent)
{
    if (socket_ctx == TCS_NULLSOCKET || buffers == NULL || buffer_count == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (flags & TCS_MSG_SENDALL)
        return TCS_ERROR_NOT_IMPLEMENTED;

    // We are going to use undefined behavior here, since we are going to use the struct iovec as a TcsBuffer.
    // We check that the size and offset is the same and hope for the best.
    // Some plattforms (Glibc) do not follow posix. We have mixed size_t and int types for msg_iovlen.
    // We cast it to a narrower unsigned type to avoid warnings for later assignment.
    // If you read this and doesn't like it, you can use probably use the compiler extension typeof() instead
    // to cast it to correct type without warnigns. We can not, since we want to support more compilers.

    if (sizeof(struct TcsBuffer) != sizeof(struct iovec))
        return TCS_ERROR_NOT_IMPLEMENTED;

    if (offsetof(struct TcsBuffer, length) != offsetof(struct iovec, iov_len))
        return TCS_ERROR_NOT_IMPLEMENTED;

#if USHRT_MAX < INT_MAX
    if (buffer_count > USHRT_MAX)
        return TCS_ERROR_INVALID_ARGUMENT;
    unsigned short narrow_casted_iovlen = (unsigned short)buffer_count;
#else
    if (buffer_count > UCHAR_MAX)
        return TCS_ERROR_INVALID_ARGUMENT;
    unsigned char narrow_casted_iovlen = (unsigned char)buffer_count;
#endif

    struct msghdr msg;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = (struct iovec*)buffers; // UB: aliasing optimization here.
    msg.msg_iovlen = narrow_casted_iovlen;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    ssize_t ret = 0;
    ret = sendmsg(socket_ctx, &msg, TCS_DEFAULT_SEND_FLAGS | (int)flags);

    if (ret >= 0)
    {
        if (bytes_sent != NULL)
            *bytes_sent = (size_t)ret;
        return TCS_SUCCESS;
    }
    else
    {
        if (bytes_sent != NULL)
            *bytes_sent = 0;
        return errno2retcode(errno);
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

    ssize_t recv_status = recv(socket_ctx, (char*)buffer, buffer_size, TCS_DEFAULT_RECV_FLAGS | (int)flags);

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

TcsReturnCode tcs_receive_from(TcsSocket socket_ctx,
                               uint8_t* buffer,
                               size_t buffer_size,
                               uint32_t flags,
                               struct TcsAddress* source_address,
                               size_t* bytes_received)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage native_sockaddr;
    memset(&native_sockaddr, 0, sizeof native_sockaddr);
    socklen_t addrlen = sizeof native_sockaddr;

    ssize_t recvfrom_status = recvfrom(socket_ctx,
                                       (char*)buffer,
                                       buffer_size,
                                       TCS_DEFAULT_RECV_FLAGS | (int)flags,
                                       (struct sockaddr*)&native_sockaddr,
                                       (socklen_t*)&addrlen);

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

struct __tcs_fd_poll_vector
{
    size_t capacity;
    size_t count;
    struct pollfd* data;
    void** user_data;
};

struct TcsPool
{
    union __backend
    {
        struct __poll
        {
            struct __tcs_fd_poll_vector vector;
        } poll;
    } backend;
};

static const size_t TCS_POOL_CAPACITY_STEP = 1024;

TcsReturnCode tcs_pool_create(struct TcsPool** pool)
{
    if (pool == NULL || *pool != NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    *pool = (struct TcsPool*)malloc(sizeof(struct TcsPool));
    if (*pool == NULL)
        return TCS_ERROR_MEMORY;
    memset(*pool, 0, sizeof(struct TcsPool));

    (*pool)->backend.poll.vector.data = (struct pollfd*)malloc(TCS_POOL_CAPACITY_STEP * sizeof(struct pollfd));
    if ((*pool)->backend.poll.vector.data == NULL)
    {
        tcs_pool_destory(pool);
        return TCS_ERROR_MEMORY;
    }
    memset((*pool)->backend.poll.vector.data, 0, TCS_POOL_CAPACITY_STEP * sizeof(struct pollfd));

    (*pool)->backend.poll.vector.user_data = (void**)malloc(TCS_POOL_CAPACITY_STEP * sizeof(void*));
    if ((*pool)->backend.poll.vector.user_data == NULL)
    {
        tcs_pool_destory(pool);
        return TCS_ERROR_MEMORY;
    }

    (*pool)->backend.poll.vector.capacity = TCS_POOL_CAPACITY_STEP;

    return TCS_SUCCESS;
}

TcsReturnCode tcs_pool_destory(struct TcsPool** pool)
{
    if (pool == NULL || *pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    free((*pool)->backend.poll.vector.data);
    free((*pool)->backend.poll.vector.user_data);
    free(*pool);
    *pool = NULL;

    return TCS_SUCCESS;
}

TcsReturnCode tcs_pool_add(struct TcsPool* pool,
                           TcsSocket socket_ctx,
                           void* user_data,
                           bool poll_can_read,
                           bool poll_can_write,
                           bool poll_error)
{
    if (pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;
    struct __tcs_fd_poll_vector* vec = &pool->backend.poll.vector;

    // Should not happen, something is corrupted
    if (vec->data == NULL)
        return TCS_ERROR_UNKNOWN;

    if (vec->count >= vec->capacity)
    {
        void* new_data = realloc(vec->data, vec->capacity + TCS_POOL_CAPACITY_STEP);
        if (new_data == NULL)
        {
            return TCS_ERROR_MEMORY;
        }
        void* new_user_data = realloc(vec->user_data, vec->capacity + TCS_POOL_CAPACITY_STEP);
        if (new_user_data == NULL)
        {
            free(new_data);
            return TCS_ERROR_MEMORY;
        }

        vec->data = (struct pollfd*)new_data;
        vec->user_data = (void**)new_user_data;
        vec->capacity += TCS_POOL_CAPACITY_STEP;
    }

    // todo(markusl): Add more events that is input and output events
    short ev = 0;
    if (poll_can_read)
        ev |= POLLIN;
    if (poll_can_write)
        ev |= POLLOUT;
    if (poll_error)
        ev |= POLLERR;

    vec->data[vec->count].fd = socket_ctx;
    vec->data[vec->count].revents = 0;
    vec->data[vec->count].events = ev;
    vec->user_data[vec->count] = user_data;
    vec->count++;

    return TCS_SUCCESS;
}

static TcsReturnCode __tcs_pool_remove_index(struct TcsPool* pool, size_t index)
{
#ifndef NDEBUG
    if (pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
#endif
    struct __tcs_fd_poll_vector* vec = &pool->backend.poll.vector;

    if (index >= vec->count)
        return TCS_ERROR_INVALID_ARGUMENT;

    vec->count--;
    vec->data[index] = vec->data[vec->count];
    vec->user_data[index] = vec->user_data[vec->count];

    bool is_minimum = vec->capacity == TCS_POOL_CAPACITY_STEP;
    bool should_shrink = vec->capacity >= vec->count + 2 * TCS_POOL_CAPACITY_STEP; // hysteresis
    if (!is_minimum && should_shrink)
    {
        // free only one step (two steps can be minimum because of hysteresis)
        void* new_data = realloc(vec->data, vec->capacity - TCS_POOL_CAPACITY_STEP);
        if (new_data == NULL)
            return TCS_ERROR_MEMORY; // Should not happen since we are shrinking

        void* new_user_data = realloc(vec->user_data, vec->capacity - TCS_POOL_CAPACITY_STEP);
        if (new_user_data == NULL)
        {
            free(new_data);
            return TCS_ERROR_MEMORY;
        }

        vec->capacity -= TCS_POOL_CAPACITY_STEP;
        vec->data = (struct pollfd*)new_data;
        vec->user_data = (void**)new_user_data;
    }

    return TCS_SUCCESS;
}

TcsReturnCode tcs_pool_remove(struct TcsPool* pool, TcsSocket socket_ctx)
{
    if (pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct __tcs_fd_poll_vector* vec = &pool->backend.poll.vector;

    // Should not happen, something is corrupted
    if (vec->data == NULL)
        return TCS_ERROR_UNKNOWN;

    bool found = false;
    for (size_t i = 0; i < vec->count; ++i)
    {
        if (socket_ctx == vec->data[i].fd)
        {
            TcsReturnCode sts = __tcs_pool_remove_index(pool, i);
            if (sts != TCS_SUCCESS)
                return sts;
            found = true;
            break;
        }
    }
    if (!found)
        return TCS_ERROR_INVALID_ARGUMENT;

    return TCS_SUCCESS;
}

TcsReturnCode tcs_pool_poll(struct TcsPool* pool,
                            struct TcsPollEvent* events,
                            size_t events_count,
                            size_t* events_populated,
                            int64_t timeout_in_ms)
{
    if (pool == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // We do not support more more elements or time than signed int32 supports.
    // todo(markusl): Add support for int64 timeout
    if (events_count > 0x7FFFFFFF || timeout_in_ms > 0x7FFFFFFF)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct __tcs_fd_poll_vector* vec = &pool->backend.poll.vector;

    int ret = poll(vec->data, vec->count, (int)timeout_in_ms);
    if (ret == 0)
    {
        return TCS_ERROR_TIMED_OUT;
    }
    else if (ret < 0)
    {
        return errno2retcode(errno);
    }
    else if ((size_t)ret > vec->count)
    {
        return TCS_ERROR_UNKNOWN; // Corruption?
    }

    int fill_max = ret > (int)events_count ? (int)events_count : ret; // min(ret, events_count)
    int filled = 0;
    size_t n = 0;
    while (filled < fill_max)
    {
#ifndef NDEBUG
        if (n >= vec->count)
            return TCS_ERROR_UNKNOWN;
#endif
        if (vec->data[n].revents != 0)
        {
            events[filled].socket = vec->data[n].fd;
            events[filled].user_data = vec->user_data[n];
            events[filled].can_read = vec->data[n].revents & POLLIN;
            events[filled].can_write = vec->data[n].revents & POLLOUT;
            events[filled].error = (TcsReturnCode)(vec->data[n].revents & POLLERR);
            filled++;
            TcsReturnCode sts = __tcs_pool_remove_index(pool, n);
            if (sts != TCS_SUCCESS)
                return sts;
            // n will now be contain a new element after remove index, therefor do not increase n
        }
        else
        {
            n++;
        }
    }
    if (events_populated != NULL)
        *events_populated = (size_t)filled; // guaranteed > 0

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

    if (setsockopt(socket_ctx, (int)level, (int)option_name, (const char*)option_value, (socklen_t)option_size) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_get_option(TcsSocket socket_ctx,
                             int32_t level,
                             int32_t option_name,
                             void* option_value,
                             size_t* option_size)
{
    if (socket_ctx == TCS_NULLSOCKET || option_value == NULL || option_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (getsockopt(socket_ctx, (int)level, (int)option_name, (void*)option_value, (socklen_t*)option_size) == 0)
    {
        // Linux sets the buffer size to the doubled because of internal use and returns the full doubled size including internal part
#ifdef __linux__
        if (option_name == TCS_SO_RCVBUF || option_name == TCS_SO_SNDBUF)
        {
            *(unsigned int*)option_value /= 2;
        }
#endif
        return TCS_SUCCESS;
    }
    else
    {
        return errno2retcode(errno);
    }
}

TcsReturnCode tcs_shutdown(TcsSocket socket_ctx, TcsSocketDirection direction)
{
    const int LUT[] = {SHUT_RD, SHUT_WR, SHUT_RDWR};

    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    const int how = LUT[direction];
    if (shutdown(socket_ctx, how) == 0)
        return TCS_SUCCESS;
    else
        return errno2retcode(errno);
}

TcsReturnCode tcs_destroy(TcsSocket* socket_ctx)
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

    struct addrinfo native_hints;
    memset(&native_hints, 0, sizeof native_hints);
    TcsReturnCode family_convert_status = family2native(address_family, (sa_family_t*)&native_hints.ai_family);
    if (family_convert_status != TCS_SUCCESS)
        return family_convert_status;

    struct addrinfo* native_addrinfo_list = NULL;
    int sts = getaddrinfo(hostname, NULL, &native_hints, &native_addrinfo_list);
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

// This is not part of posix, we need to be platform specific here
// TODO: This is probably not compatible with include only
#if TCS_HAVE_IFADDRS // acquired from CMake
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

    struct ifaddrs* interfaces;
    getifaddrs(&interfaces);
    size_t i = 0;

    for (struct ifaddrs* iter = interfaces; iter != NULL && (found_interfaces == NULL || i < found_interfaces_length);
         iter = iter->ifa_next)
    {
        if (iter->ifa_flags & IFF_UP)
        {
            struct TcsAddress t;
            if (native2sockaddr(iter->ifa_addr, &t) != TCS_SUCCESS)
                continue;
            if (found_interfaces != NULL)
            {
                found_interfaces[i].address = t;
                strncpy(found_interfaces[i].name, iter->ifa_name, 31);
                found_interfaces[i].name[31] = '\0';
            }
            i++;
        }
    }
    freeifaddrs(interfaces);
    if (no_of_found_interfaces != NULL)
        *no_of_found_interfaces = i;
    return TCS_SUCCESS;
}
#else
// SunOS before 2010, HP and AIX does not support getifaddrs
// ioctl implementation,
// https://stackoverflow.com/questions/4139405/how-can-i-get-to-know-the-ip-address-for-interfaces-in-c/4139811#4139811
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

    int fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return TCS_ERROR_UNKNOWN;

    struct ifconf adapters;
    char buf[16384];
    adapters.ifc_len = sizeof(buf);
    adapters.ifc_buf = buf;
    if (ioctl(fd, SIOCGIFCONF, &adapters) != 0) // todo(markus): add tries as for Windows
    {
        close(fd);
        return TCS_ERROR_UNKNOWN;
    }

    struct ifreq* ifr_iterator = adapters.ifc_req;
    int offset = 0;
    int len;
    size_t i = 0;
    while (offset < adapters.ifc_len)
    {
        // todo(markusl): we need to investigate which OS uses which system, scheduled to after CI with multiple OS
#ifndef __linux__
        len = IFNAMSIZ + ifr_iterator->ifr_addr.sa_len;
#else
        len = sizeof(struct ifreq);
#endif
        if (found_interfaces != NULL)
        {
            native2sockaddr((struct sockaddr*)&ifr_iterator->ifr_addr, &found_interfaces[i].address);
            strncpy(found_interfaces[i].name, ifr_iterator->ifr_name, 31);
            found_interfaces[i].name[31] = '\0';
        }
        ++i;
        ifr_iterator = (struct ifreq*)((uint8_t*)ifr_iterator + len);
        offset += len;
    }
    if (no_of_found_interfaces != NULL)
        *no_of_found_interfaces = i;
    close(fd);
    return TCS_SUCCESS;
}
#endif

TcsReturnCode tcs_set_linger(TcsSocket socket_ctx, bool do_linger, int timeout_seconds)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {(u_short)do_linger, (u_short)timeout_seconds};
    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_LINGER, &l, sizeof(l));
}

TcsReturnCode tcs_get_linger(TcsSocket socket_ctx, bool* do_linger, int* timeout_seconds)
{
    if (socket_ctx == TCS_NULLSOCKET || (do_linger == NULL && timeout_seconds == NULL))
        return TCS_ERROR_INVALID_ARGUMENT;

    struct linger l = {0, 0};
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

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &tv, sizeof(tv));
}

TcsReturnCode tcs_get_receive_timeout(TcsSocket socket_ctx, int* timeout_ms)
{
    if (socket_ctx == TCS_NULLSOCKET || timeout_ms == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct timeval tv = {0, 0};
    size_t tv_size = sizeof(tv);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVTIMEO, &tv, &tv_size);

    if (sts == TCS_SUCCESS)
    {
        int c = 0;
        c += (int)tv.tv_sec * 1000;
        c += (int)tv.tv_usec / 1000;
        *timeout_ms = c;
    }
    return sts;
}

TcsReturnCode tcs_set_ip_multicast_add(TcsSocket socket_ctx,
                                       const struct TcsAddress* local_address,
                                       const struct TcsAddress* multicast_address)
{
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    // Override here to remove error

    struct sockaddr_storage address_native_multicast;
    memset(&address_native_multicast, 0, sizeof address_native_multicast);
    socklen_t address_native_multicast_size = 0;
    TcsReturnCode mutlicast_to_native_sts =
        sockaddr2native(multicast_address, (struct sockaddr*)&address_native_multicast, &address_native_multicast_size);
    if (mutlicast_to_native_sts != TCS_SUCCESS)
        return mutlicast_to_native_sts;

    struct sockaddr_storage address_native_local;
    memset(&address_native_local, 0, sizeof address_native_local);
    socklen_t address_native_local_size = 0;
    if (local_address != NULL)
    {
        TcsReturnCode mutlicast_to_local_sts =
            sockaddr2native(local_address, (struct sockaddr*)&address_native_local, &address_native_local_size);
        if (mutlicast_to_local_sts)
            return mutlicast_to_local_sts;
    }

    if (multicast_address->family == TCS_AF_IP4)
    {
        struct sockaddr_in* address_native_multicast_p = (struct sockaddr_in*)&address_native_multicast;
        struct sockaddr_in* address_native_local_p = (struct sockaddr_in*)&address_native_local;

        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof mreq);

        mreq.imr_interface.s_addr = address_native_local_p->sin_addr.s_addr;
        mreq.imr_multiaddr.s_addr = address_native_multicast_p->sin_addr.s_addr;

        return tcs_set_option(socket_ctx, TCS_SOL_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    }
    else
    {
        // TODO(markusl): Add ipv6 support
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
}

TcsReturnCode tcs_set_ip_multicast_drop(TcsSocket socket_ctx,
                                        const struct TcsAddress* local_address,
                                        const struct TcsAddress* multicast_address)
{
    if (multicast_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct sockaddr_storage address_native_multicast;
    memset(&address_native_multicast, 0, sizeof address_native_multicast);
    socklen_t address_native_multicast_size = 0;
    TcsReturnCode mutlicast_to_native_sts =
        sockaddr2native(multicast_address, (struct sockaddr*)&address_native_multicast, &address_native_multicast_size);
    if (mutlicast_to_native_sts != TCS_SUCCESS)
        return mutlicast_to_native_sts;

    struct sockaddr_storage address_native_local;
    memset(&address_native_local, 0, sizeof address_native_local);
    socklen_t address_native_local_size = 0;
    if (local_address != NULL)
    {
        TcsReturnCode mutlicast_to_local_sts =
            sockaddr2native(local_address, (struct sockaddr*)&address_native_local, &address_native_local_size);
        if (mutlicast_to_local_sts)
            return mutlicast_to_local_sts;
    }

    if (multicast_address->family == TCS_AF_IP4)
    {
        struct sockaddr_in* address_native_multicast_p = (struct sockaddr_in*)&address_native_multicast;
        struct sockaddr_in* address_native_local_p = (struct sockaddr_in*)&address_native_local;

        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof mreq);

        mreq.imr_interface.s_addr = address_native_local_p->sin_addr.s_addr;
        mreq.imr_multiaddr.s_addr = address_native_multicast_p->sin_addr.s_addr;

        return tcs_set_option(socket_ctx, TCS_SOL_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    }
    else
    {
        // TODO(markusl): Add ipv6 support
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
}

#endif
