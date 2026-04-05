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
#ifndef TINYCSOCKET_INTERNAL_H_
#include "tinycsocket_internal.h"
#endif

// This file should never call OS dependent code. Do not include OS files of OS specific ifdefs

#ifdef DO_WRAP
#include "dbg_wrap.h"
#endif

#include <stdbool.h>
#include <stdio.h>  //sprintf
#include <string.h> // memset

// ######## Library Management ########

// tcs_lib_init() is defined in OS specific files
// tcs_lib_free() is defined in OS specific files

// ######## Socket Creation ########

// tcs_socket() is defined in OS specific files

TcsResult tcs_socket_preset(TcsSocket* socket_ctx, TcsPreset socket_type)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsAddressFamily family = TCS_AF_ANY;
    int type = 0;
    int protocol = 0;

    switch (socket_type)
    {
        case TCS_PRESET_TCP_IP4:
            family = TCS_AF_IP4;
            type = TCS_SOCK_STREAM;
            protocol = TCS_PROTOCOL_IP_TCP;
            break;
        case TCS_PRESET_UDP_IP4:
            family = TCS_AF_IP4;
            type = TCS_SOCK_DGRAM;
            protocol = TCS_PROTOCOL_IP_UDP;
            break;
        case TCS_PRESET_RAW:
            family = TCS_AF_PACKET;
            type = TCS_SOCK_RAW;
            protocol = 0; // Block all traffic, unblock with tcs_bind_addr()
            break;
        case TCS_PRESET_PACKET:
            family = TCS_AF_PACKET;
            type = TCS_SOCK_DGRAM;
            protocol = 0; // Block all traffic, unblock with tcs_bind_addr()
            break;
        case TCS_PRESET_TCP_IP6:
            family = TCS_AF_IP6;
            type = TCS_SOCK_STREAM;
            protocol = TCS_PROTOCOL_IP_TCP;
            break;
        case TCS_PRESET_UDP_IP6:
            family = TCS_AF_IP6;
            type = TCS_SOCK_DGRAM;
            protocol = TCS_PROTOCOL_IP_UDP;
            break;
        default:
            return TCS_ERROR_NOT_IMPLEMENTED;
    }
    return tcs_socket(socket_ctx, family, type, protocol);
}

// tcs_close() is defined in OS specific files

// ######## High-level Socket Creation ########

TcsResult tcs_tcp_server(TcsSocket* socket_ctx, const struct TcsAddress* local_address)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsResult res = tcs_socket(socket_ctx, local_address->family, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP);
    if (res != TCS_SUCCESS)
        return res;
    res = tcs_bind(*socket_ctx, local_address);
    if (res != TCS_SUCCESS)
    {
        tcs_close(socket_ctx);
        return res;
    }
    res = tcs_listen(*socket_ctx, TCS_BACKLOG_MAX);
    if (res != TCS_SUCCESS)
    {
        tcs_close(socket_ctx);
        return res;
    }
    return TCS_SUCCESS;
}

TcsResult tcs_tcp_server_str(TcsSocket* socket_ctx, const char* local_address, uint16_t port)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress bind_address = TCS_ADDRESS_NONE;
    TcsResult res = tcs_address_parse(local_address, &bind_address);
    if (res != TCS_SUCCESS)
        return res;
    if (bind_address.family != TCS_AF_IP4 && bind_address.family != TCS_AF_IP6)
        return TCS_ERROR_INVALID_ARGUMENT;

    uint16_t* parsed_port = NULL;
    if (bind_address.family == TCS_AF_IP4)
    {
        parsed_port = &bind_address.data.ip4.port;
    }
    else if (bind_address.family == TCS_AF_IP6)
    {
        parsed_port = &bind_address.data.ip6.port;
    }
    else
    {
        return TCS_ERROR_UNKNOWN;
    }

    if (port == 0 && *parsed_port == 0)
        return TCS_ERROR_INVALID_ARGUMENT; // No port specified
    if (port != 0 && *parsed_port != 0)
        return TCS_ERROR_INVALID_ARGUMENT; // Port specified in both string and argument

    if (port != 0 && *parsed_port == 0)
        *parsed_port = port;

    return tcs_tcp_server(socket_ctx, &bind_address);
}

TcsResult tcs_tcp_client(TcsSocket* socket_ctx, const struct TcsAddress* remote_address, int timeout_ms)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (timeout_ms < 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (remote_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (remote_address->family != TCS_AF_IP4 && remote_address->family != TCS_AF_IP6)
        return TCS_ERROR_NOT_IMPLEMENTED;
    if (remote_address->family == TCS_AF_IP4 && remote_address->data.ip4.port == 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (remote_address->family == TCS_AF_IP6 && remote_address->data.ip6.port == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsResult res = tcs_socket(socket_ctx, remote_address->family, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP);
    if (res != TCS_SUCCESS)
        return res;

    struct TcsPool* timeout_pool = NULL;
    res = tcs_pool_create(&timeout_pool);
    if (res != TCS_SUCCESS)
    {
        tcs_close(socket_ctx);
        return res;
    }

    res = tcs_opt_nonblocking_set(*socket_ctx, true);
    if (res != TCS_SUCCESS)
    {
        tcs_pool_destroy(&timeout_pool);
        tcs_close(socket_ctx);
        return res;
    }
    res = tcs_connect(*socket_ctx, remote_address);
    if (res != TCS_IN_PROGRESS && res != TCS_SUCCESS)
    {
        tcs_pool_destroy(&timeout_pool);
        tcs_close(socket_ctx);
        return res;
    }

    if (res == TCS_IN_PROGRESS)
    {
        res = tcs_pool_add(timeout_pool, *socket_ctx, NULL, false, true, true);
        if (res != TCS_SUCCESS)
        {
            tcs_pool_destroy(&timeout_pool);
            tcs_close(socket_ctx);
            return res;
        }

        struct TcsPollEvent event = TCS_POOL_EVENT_EMPTY;
        size_t events_populated = 0;
        res = tcs_pool_poll(timeout_pool, &event, 1, &events_populated, timeout_ms);
        if (res == TCS_ERROR_TIMED_OUT)
        {
            tcs_pool_destroy(&timeout_pool);
            tcs_close(socket_ctx);
            return TCS_ERROR_TIMED_OUT;
        }
        if (res != TCS_SUCCESS || events_populated == 0)
        {
            tcs_pool_destroy(&timeout_pool);
            tcs_close(socket_ctx);
            return res != TCS_SUCCESS ? res : TCS_ERROR_UNKNOWN;
        }
        if (event.error != TCS_SUCCESS)
        {
            tcs_pool_destroy(&timeout_pool);
            tcs_close(socket_ctx);
            return TCS_ERROR_CONNECTION_REFUSED;
        }
    }

    tcs_pool_destroy(&timeout_pool);

    res = tcs_opt_nonblocking_set(*socket_ctx, false);
    if (res != TCS_SUCCESS)
    {
        tcs_close(socket_ctx);
        return res;
    }

    return TCS_SUCCESS;
}

TcsResult tcs_tcp_client_str(TcsSocket* socket_ctx, const char* remote_address, uint16_t port, int timeout_ms)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (remote_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress address = TCS_ADDRESS_NONE;
    TcsResult res = tcs_address_parse(remote_address, &address);
    if (res != TCS_SUCCESS)
        return res;
    if (address.family != TCS_AF_IP4 && address.family != TCS_AF_IP6)
        return TCS_ERROR_INVALID_ARGUMENT;

    uint16_t* parsed_port = NULL;
    if (address.family == TCS_AF_IP4)
    {
        parsed_port = &address.data.ip4.port;
    }
    else if (address.family == TCS_AF_IP6)
    {
        parsed_port = &address.data.ip6.port;
    }
    else
    {
        return TCS_ERROR_UNKNOWN;
    }

    if (port == 0 && *parsed_port == 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (port != 0 && *parsed_port != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (port != 0 && *parsed_port == 0)
        *parsed_port = port;

    return tcs_tcp_client(socket_ctx, &address, timeout_ms);
}

TcsResult tcs_udp_receiver(TcsSocket* socket_ctx, const struct TcsAddress* local_address)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsResult res = tcs_socket(socket_ctx, local_address->family, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP);
    if (res != TCS_SUCCESS)
        return res;
    res = tcs_bind(*socket_ctx, local_address);
    if (res != TCS_SUCCESS)
    {
        tcs_close(socket_ctx);
        return res;
    }
    return TCS_SUCCESS;
}

TcsResult tcs_udp_receiver_str(TcsSocket* socket_ctx, const char* local_address, uint16_t port)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress bind_address = TCS_ADDRESS_NONE;
    TcsResult res = tcs_address_parse(local_address, &bind_address);
    if (res != TCS_SUCCESS)
        return res;
    if (bind_address.family != TCS_AF_IP4 && bind_address.family != TCS_AF_IP6)
        return TCS_ERROR_INVALID_ARGUMENT;

    uint16_t* parsed_port = NULL;
    if (bind_address.family == TCS_AF_IP4)
    {
        parsed_port = &bind_address.data.ip4.port;
    }
    else if (bind_address.family == TCS_AF_IP6)
    {
        parsed_port = &bind_address.data.ip6.port;
    }
    else
    {
        return TCS_ERROR_UNKNOWN;
    }

    if (port == 0 && *parsed_port == 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (port != 0 && *parsed_port != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (port != 0 && *parsed_port == 0)
        *parsed_port = port;

    return tcs_udp_receiver(socket_ctx, &bind_address);
}

TcsResult tcs_udp_sender(TcsSocket* socket_ctx, const struct TcsAddress* remote_address)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (remote_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsResult res = tcs_socket(socket_ctx, remote_address->family, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP);
    if (res != TCS_SUCCESS)
        return res;
    res = tcs_connect(*socket_ctx, remote_address);
    if (res != TCS_SUCCESS)
    {
        tcs_close(socket_ctx);
        return res;
    }
    return TCS_SUCCESS;
}

TcsResult tcs_udp_sender_str(TcsSocket* socket_ctx, const char* remote_address, uint16_t port)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (remote_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress address = TCS_ADDRESS_NONE;
    TcsResult res = tcs_address_parse(remote_address, &address);
    if (res != TCS_SUCCESS)
        return res;
    if (address.family != TCS_AF_IP4 && address.family != TCS_AF_IP6)
        return TCS_ERROR_INVALID_ARGUMENT;

    uint16_t* parsed_port = NULL;
    if (address.family == TCS_AF_IP4)
    {
        parsed_port = &address.data.ip4.port;
    }
    else if (address.family == TCS_AF_IP6)
    {
        parsed_port = &address.data.ip6.port;
    }
    else
    {
        return TCS_ERROR_UNKNOWN;
    }

    if (port == 0 && *parsed_port == 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (port != 0 && *parsed_port != 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (port != 0 && *parsed_port == 0)
        *parsed_port = port;

    return tcs_udp_sender(socket_ctx, &address);
}

TcsResult tcs_udp_peer(TcsSocket* socket_ctx,
                       const struct TcsAddress* local_address,
                       const struct TcsAddress* remote_address)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (remote_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsResult res = tcs_socket(socket_ctx, local_address->family, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP);
    if (res != TCS_SUCCESS)
        return res;
    res = tcs_bind(*socket_ctx, local_address);
    if (res != TCS_SUCCESS)
    {
        tcs_close(socket_ctx);
        return res;
    }
    res = tcs_connect(*socket_ctx, remote_address);
    if (res != TCS_SUCCESS)
    {
        tcs_close(socket_ctx);
        return res;
    }
    return TCS_SUCCESS;
}

TcsResult tcs_udp_peer_str(TcsSocket* socket_ctx,
                           const char* local_address,
                           uint16_t local_port,
                           const char* remote_address,
                           uint16_t remote_port)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (local_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (remote_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress local_addr = TCS_ADDRESS_NONE;
    TcsResult res = tcs_address_parse(local_address, &local_addr);
    if (res != TCS_SUCCESS)
        return res;
    if (local_addr.family != TCS_AF_IP4 && local_addr.family != TCS_AF_IP6)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress remote_addr = TCS_ADDRESS_NONE;
    res = tcs_address_parse(remote_address, &remote_addr);
    if (res != TCS_SUCCESS)
        return res;
    if (remote_addr.family != TCS_AF_IP4 && remote_addr.family != TCS_AF_IP6)
        return TCS_ERROR_INVALID_ARGUMENT;

    uint16_t* local_parsed_port = NULL;
    if (local_addr.family == TCS_AF_IP4)
        local_parsed_port = &local_addr.data.ip4.port;
    else if (local_addr.family == TCS_AF_IP6)
        local_parsed_port = &local_addr.data.ip6.port;
    else
        return TCS_ERROR_UNKNOWN;

    if (local_port == 0 && *local_parsed_port == 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (local_port != 0 && *local_parsed_port != 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (local_port != 0 && *local_parsed_port == 0)
        *local_parsed_port = local_port;

    uint16_t* remote_parsed_port = NULL;
    if (remote_addr.family == TCS_AF_IP4)
        remote_parsed_port = &remote_addr.data.ip4.port;
    else if (remote_addr.family == TCS_AF_IP6)
        remote_parsed_port = &remote_addr.data.ip6.port;
    else
        return TCS_ERROR_UNKNOWN;

    if (remote_port == 0 && *remote_parsed_port == 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (remote_port != 0 && *remote_parsed_port != 0)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (remote_port != 0 && *remote_parsed_port == 0)
        *remote_parsed_port = remote_port;

    return tcs_udp_peer(socket_ctx, &local_addr, &remote_addr);
}

// ######## High-level Raw L2-Packet Sockets (Experimental) ########

TcsResult tcs_raw(TcsSocket* socket_ctx, const struct TcsAddress* bind_address)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (bind_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (bind_address->family != TCS_AF_PACKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsResult res = tcs_socket(socket_ctx, TCS_AF_PACKET, TCS_SOCK_RAW, bind_address->data.packet.protocol);
    if (res != TCS_SUCCESS)
        return res;
    res = tcs_bind(*socket_ctx, bind_address);
    if (res != TCS_SUCCESS)
    {
        tcs_close(socket_ctx);
        return res;
    }
    return TCS_SUCCESS;
}

TcsResult tcs_raw_str(TcsSocket* socket_ctx, const char* interface_name, uint16_t protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (interface_name == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsInterface interfaces[32];
    size_t count = 0;
    TcsResult res = tcs_interface_list(interfaces, 32, &count);
    if (res != TCS_SUCCESS)
        return res;

    for (size_t i = 0; i < count; ++i)
    {
        if (strcmp(interfaces[i].name, interface_name) == 0)
        {
            struct TcsAddress bind_address = TCS_ADDRESS_NONE;
            bind_address.family = TCS_AF_PACKET;
            bind_address.data.packet.interface_id = interfaces[i].id;
            bind_address.data.packet.protocol = protocol;
            return tcs_raw(socket_ctx, &bind_address);
        }
    }

    return TCS_ERROR_INVALID_ARGUMENT;
}

// ######## High-level L2-Packet DGRAM Sockets (Experimental) ########

TcsResult tcs_packet(TcsSocket* socket_ctx, const struct TcsAddress* bind_address)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (bind_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (bind_address->family != TCS_AF_PACKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsResult res = tcs_socket(socket_ctx, TCS_AF_PACKET, TCS_SOCK_DGRAM, bind_address->data.packet.protocol);
    if (res != TCS_SUCCESS)
        return res;
    res = tcs_bind(*socket_ctx, bind_address);
    if (res != TCS_SUCCESS)
    {
        tcs_close(socket_ctx);
        return res;
    }
    return TCS_SUCCESS;
}

TcsResult tcs_packet_str(TcsSocket* socket_ctx, const char* interface_name, uint16_t protocol)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (interface_name == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsInterface interfaces[32];
    size_t count = 0;
    TcsResult res = tcs_interface_list(interfaces, 32, &count);
    if (res != TCS_SUCCESS)
        return res;

    for (size_t i = 0; i < count; ++i)
    {
        if (strcmp(interfaces[i].name, interface_name) == 0)
        {
            struct TcsAddress bind_address = TCS_ADDRESS_NONE;
            bind_address.family = TCS_AF_PACKET;
            bind_address.data.packet.interface_id = interfaces[i].id;
            bind_address.data.packet.protocol = protocol;
            return tcs_packet(socket_ctx, &bind_address);
        }
    }

    return TCS_ERROR_INVALID_ARGUMENT;
}

// ######## Socket Operations ########

// tcs_bind() is defined in OS specific files
// tcs_connect() is defined in OS specific files

TcsResult tcs_connect_str(TcsSocket socket_ctx, const char* remote_address, uint16_t port)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (remote_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsAddressFamily socket_family = TCS_AF_ANY;
    TcsResult res = tcs_address_socket_family(socket_ctx, &socket_family);
    if (res != TCS_SUCCESS)
        return res;

    if (socket_family != TCS_AF_IP4 && socket_family != TCS_AF_IP6)
        return TCS_ERROR_NOT_IMPLEMENTED;

    struct TcsAddress found_addresses;
    size_t no_of_found_addresses = 0;

    res = tcs_address_resolve(remote_address, socket_family, &found_addresses, 1, &no_of_found_addresses);
    if (res != TCS_SUCCESS)
        return res;

    if (no_of_found_addresses == 0)
        return TCS_ERROR_ADDRESS_LOOKUP_FAILED;

    switch (socket_family)
    {
        case TCS_AF_IP4:
            if (found_addresses.data.ip4.port == 0)
                found_addresses.data.ip4.port = port;
            else if (port != 0)
                return TCS_ERROR_INVALID_ARGUMENT;
            break;
        case TCS_AF_IP6:
            if (found_addresses.data.ip6.port == 0)
                found_addresses.data.ip6.port = port;
            else if (port != 0)
                return TCS_ERROR_INVALID_ARGUMENT;
            break;
        case TCS_AF_ANY:
            break;
        default:
            return TCS_ERROR_INVALID_ARGUMENT;
    }
    return tcs_connect(socket_ctx, &found_addresses);
}

// tcs_listen() is defined in OS specific files
// tcs_accept() is defined in OS specific files
// tcs_shutdown() is defined in OS specific files

// ######## Data Transfer ########

// tcs_send() is defined in OS specific files
// tcs_send_to() is defined in OS specific files
// tcs_sendv() is defined in OS specific files

TcsResult tcs_send_netstring(TcsSocket socket_ctx, const uint8_t* buffer, size_t buffer_length)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (buffer == NULL || buffer_length == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

#if SIZE_MAX > 0xffffffffffffffffULL
    // buffer_length bigger than 64 bits? (size_t can be bigger on some systems)
    if (buffer_length > 0xffffffffffffffffULL)
        return TCS_ERROR_INVALID_ARGUMENT;
#endif

    int header_length = 0;
    char netstring_header[21];
    memset(netstring_header, 0, sizeof netstring_header);

    // %zu is not supported by all compilers, therefor we cast it to llu
    header_length = snprintf(netstring_header, 21, "%llu:", (unsigned long long)buffer_length);

    if (header_length < 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsResult sts = TCS_SUCCESS;
    sts = tcs_send(socket_ctx, (uint8_t*)netstring_header, (size_t)header_length, TCS_MSG_SENDALL, NULL);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_send(socket_ctx, buffer, buffer_length, TCS_MSG_SENDALL, NULL);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_send(socket_ctx, (const uint8_t*)",", 1, TCS_MSG_SENDALL, NULL);
    if (sts != TCS_SUCCESS)
        return sts;

    return TCS_SUCCESS;
}

// tcs_receive() is defined in OS specific files
// tcs_receive_from() is defined in OS specific files

TcsResult tcs_receive_line(TcsSocket socket_ctx,
                           uint8_t* buffer,
                           size_t buffer_length,
                           size_t* bytes_received,
                           uint8_t delimiter)
{
    if (socket_ctx == TCS_SOCKET_INVALID || buffer == NULL || buffer_length == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    /*
    *                    data in kernel buffer
    *   |12345yyyyyyyyyyyyyyyyyyyyyyyy..............|
    *
    *       buffer_length ----------------------------------.
    *       searched ----------------------.                |
    *                                      |                |
    *       bytes_peeked ------------------.                |
    *       bytes_read ---------------.    |                |
    *                                 v    v                v
    *       data in arg buffer
    *   |xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx12345................|
    */
    size_t bytes_read = 0;
    size_t bytes_peeked = 0;
    size_t bytes_searched = 0;
    while (bytes_read < buffer_length)
    {
        TcsResult sts = TCS_SUCCESS;
        size_t bytes_free_in_buffer = buffer_length - bytes_read;
        size_t current_peeked = 0;
        sts = tcs_receive(socket_ctx, buffer + bytes_read, bytes_free_in_buffer, TCS_MSG_PEEK, &current_peeked);
        if (sts != TCS_SUCCESS)
        {
            if (bytes_received != NULL)
                *bytes_received = bytes_read;
            return sts;
        }
        bytes_peeked += current_peeked;

        if (current_peeked == 0)
        {
            // Make sure we block so we do not fast loop previous PEEK.
            // Can not assume that peek with waitall is not crossplatform, needs to read
            size_t current_read = 0;
            sts = tcs_receive(socket_ctx, buffer + bytes_read, 1, TCS_MSG_WAITALL, &current_read);
            bytes_read += current_read;
            bytes_peeked += current_read;

            if (sts != TCS_SUCCESS)
            {
                if (bytes_received != NULL)
                    *bytes_received = bytes_read;
                return sts;
            }
        }

        bool found_delimiter = false;

        while (bytes_searched < bytes_peeked)
        {
            if (buffer[bytes_searched++] == delimiter)
            {
                found_delimiter = true;
                break;
            }
        }

        // byte_searched == bytes_peeked if no delimiter was found
        // after this block, bytes_read will also has the same value as they have
        if (bytes_searched > bytes_read)
        {
            size_t bytes = 0;
            size_t bytes_to_read_to_catch_up = bytes_searched - bytes_read;
            sts = tcs_receive(socket_ctx, buffer + bytes_read, bytes_to_read_to_catch_up, TCS_MSG_WAITALL, &bytes);
            bytes_read += bytes;
            if (sts != TCS_SUCCESS)
            {
                if (bytes_received != NULL)
                    *bytes_received = bytes_read;
                return sts;
            }
        }
        if (found_delimiter)
        {
            if (bytes_received != NULL)
                *bytes_received = bytes_read;
            return sts;
        }
    }
    if (bytes_received != NULL)
        *bytes_received = bytes_read;
    return TCS_AGAIN;
}

TcsResult tcs_receive_netstring(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_length, size_t* bytes_received)
{
    if (socket_ctx == TCS_SOCKET_INVALID || buffer == NULL || buffer_length == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    size_t expected_length = 0;
    int parsed = 0;
    TcsResult sts = TCS_SUCCESS;
    char t = '\0';
    const int max_header = 21;
    while (t != ':' && parsed < max_header)
    {
        sts = tcs_receive(socket_ctx, (uint8_t*)&t, 1, TCS_MSG_WAITALL, NULL);
        if (sts != TCS_SUCCESS)
            return sts;

        parsed += 1;

        bool is_num = t >= '0' && t <= '9';
        bool is_end = t == ':';
        if (!is_num && !is_end)
            return TCS_ERROR_ILL_FORMED_MESSAGE;

        if (is_end)
            break;

        size_t digit = (size_t)t - '0';
        if (expected_length > (SIZE_MAX - digit) / 10)
            return TCS_ERROR_ILL_FORMED_MESSAGE;
        expected_length = expected_length * 10 + digit;
    }

    if (parsed >= max_header)
        return TCS_ERROR_ILL_FORMED_MESSAGE;

    if (buffer_length < expected_length)
        return TCS_ERROR_MEMORY;

    sts = tcs_receive(socket_ctx, buffer, expected_length, TCS_MSG_WAITALL, NULL);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_receive(socket_ctx, (uint8_t*)&t, 1, TCS_MSG_WAITALL, NULL);
    if (sts != TCS_SUCCESS)
        return sts;

    if (t != ',')
        return TCS_ERROR_ILL_FORMED_MESSAGE;

    if (bytes_received != NULL)
        *bytes_received = expected_length;

    return TCS_SUCCESS;
}

// ######## Socket Pooling ########

// tcs_pool_create() is defined in OS specific files
// tcs_pool_destroy() is defined in OS specific files
// tcs_pool_add() is defined in OS specific files
// tcs_pool_remove() is defined in OS specific files
// tcs_pool_poll() is defined in OS specific files

// ######## Socket Options ########

// tcs_opt_set() is defined in OS specific files
// tcs_opt_get() is defined in OS specific files

TcsResult tcs_opt_broadcast_set(TcsSocket socket_ctx, bool do_allow_broadcast)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = do_allow_broadcast ? 1 : 0;
    return tcs_opt_set(socket_ctx, TCS_SOL_SOCKET, TCS_SO_BROADCAST, &b, sizeof(b));
}

TcsResult tcs_opt_type_get(TcsSocket socket_ctx, int* type)
{
    if (socket_ctx == TCS_SOCKET_INVALID || type == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int t = 0;
    size_t s = sizeof(t);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_TYPE, &t, &s);
    *type = t;
    return sts;
}

TcsResult tcs_opt_broadcast_get(TcsSocket socket_ctx, bool* is_broadcast_allowed)
{
    if (socket_ctx == TCS_SOCKET_INVALID || is_broadcast_allowed == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_BROADCAST, &b, &s);
    *is_broadcast_allowed = b;
    return sts;
}

TcsResult tcs_opt_keep_alive_set(TcsSocket socket_ctx, bool do_keep_alive)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = do_keep_alive ? 1 : 0;
    return tcs_opt_set(socket_ctx, TCS_SOL_SOCKET, TCS_SO_KEEPALIVE, &b, sizeof(b));
}

TcsResult tcs_opt_keep_alive_get(TcsSocket socket_ctx, bool* is_keep_alive_enabled)
{
    if (socket_ctx == TCS_SOCKET_INVALID || is_keep_alive_enabled == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_KEEPALIVE, &b, &s);
    *is_keep_alive_enabled = b;
    return sts;
}

// tcs_opt_reuse_address_set() is defined in platform-specific files
// tcs_opt_reuse_address_get() is defined in platform-specific files

TcsResult tcs_opt_send_buffer_size_set(TcsSocket socket_ctx, size_t send_buffer_size)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    unsigned int b = (unsigned int)send_buffer_size;
    return tcs_opt_set(socket_ctx, TCS_SOL_SOCKET, TCS_SO_SNDBUF, &b, sizeof(b));
}

TcsResult tcs_opt_send_buffer_size_get(TcsSocket socket_ctx, size_t* send_buffer_size)
{
    if (socket_ctx == TCS_SOCKET_INVALID || send_buffer_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    unsigned int b = 0;
    size_t s = sizeof(b);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_SNDBUF, &b, &s);
    *send_buffer_size = (size_t)b;
    return sts;
}

TcsResult tcs_opt_receive_buffer_size_set(TcsSocket socket_ctx, size_t receive_buffer_size)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    unsigned int b = (unsigned int)receive_buffer_size;
    return tcs_opt_set(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVBUF, &b, sizeof(b));
}

TcsResult tcs_opt_receive_buffer_size_get(TcsSocket socket_ctx, size_t* receive_buffer_size)
{
    if (socket_ctx == TCS_SOCKET_INVALID || receive_buffer_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    unsigned int b = 0;
    size_t s = sizeof(b);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVBUF, &b, &s);
    *receive_buffer_size = (size_t)b;
    return sts;
}

// tcs_opt_receive_timout_set() is defined in OS specific files
// tcs_opt_receive_timout_get() is defined in OS specific files
// tcs_opt_linger_set() is defined in OS specific files
// tcs_opt_linger_get() is defined in OS specific files

TcsResult tcs_opt_ip_no_delay_set(TcsSocket socket_ctx, bool use_no_delay)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = use_no_delay ? 1 : 0;
    return tcs_opt_set(socket_ctx, TCS_SOL_IP, TCS_SO_IP_NODELAY, &b, sizeof(b));
}

TcsResult tcs_opt_ip_no_delay_get(TcsSocket socket_ctx, bool* is_no_delay_used)
{
    if (socket_ctx == TCS_SOCKET_INVALID || is_no_delay_used == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_IP, TCS_SO_IP_NODELAY, &b, &s);
    *is_no_delay_used = b;
    return sts;
}

TcsResult tcs_opt_out_of_band_inline_set(TcsSocket socket_ctx, bool enable_oob)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = enable_oob ? 1 : 0;
    return tcs_opt_set(socket_ctx, TCS_SOL_SOCKET, TCS_SO_OOBINLINE, &b, sizeof(b));
}

TcsResult tcs_opt_out_of_band_inline_get(TcsSocket socket_ctx, bool* is_oob_enabled)
{
    if (socket_ctx == TCS_SOCKET_INVALID || is_oob_enabled == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_OOBINLINE, &b, &s);
    *is_oob_enabled = b;
    return sts;
}

TcsResult tcs_opt_priority_set(TcsSocket socket_ctx, int priority)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    return tcs_opt_set(socket_ctx, TCS_SOL_SOCKET, TCS_SO_PRIORITY, &priority, sizeof(priority));
}

TcsResult tcs_opt_priority_get(TcsSocket socket_ctx, int* priority)
{
    if (socket_ctx == TCS_SOCKET_INVALID || priority == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    size_t s = sizeof(*priority);
    return tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_PRIORITY, priority, &s);
}

// tcs_opt_nonblocking_set() is defined in OS specific files
// tcs_opt_nonblocking_get() is defined in OS specific files

// tcs_opt_membership_add() is defined in OS specific files
// tcs_opt_membership_add_to() is defined in OS specific files
// tcs_opt_membership_drop() is defined in OS specific files
// tcs_opt_membership_drop_from() is defined in OS specific files

// ######## Address and Interface Utilities ########

// tcs_interface_list() is defined in OS specific files
// tcs_address_resolve() is defined in OS specific files
// tcs_address_list() is defined in OS specific files
// tcs_address_socket_local() is defined in OS specific files
// tcs_address_socket_remote() is defined in OS specific files
// tcs_address_socket_family() is defined in OS specific files

TcsResult tcs_address_parse(const char str[], struct TcsAddress* out_address)
{
    if (out_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    if (str == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (str[0] == '\0')
    {
        *out_address = TCS_ADDRESS_NONE;
        return TCS_SUCCESS;
    }

    memset(out_address, 0, sizeof(struct TcsAddress));
    // Slow but easy parser
    int n_colons = 0;
    int n_dots = 0;
    int double_colons = 0;

    for (int i = 0; str[i] != '\0' && i < 70; ++i)
    {
        if (str[i] == '.')
        {
            n_dots++;
        }
        else if (str[i] == ':')
        {
            n_colons++;
            if (i > 0 && str[i - 1] == ':')
                double_colons++;
        }
    }
    bool is_ipv4 = n_dots == 3 && n_colons <= 1;
    bool is_mac = n_colons == 5 && double_colons == 0;
    bool is_ipv6 = !is_ipv4 && !is_mac && n_colons > 1 && double_colons <= 1;

    if (is_ipv4 + is_mac + is_ipv6 != 1)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (is_ipv4)
    {
        int b1;
        int b2;
        int b3;
        int b4;
        int p = 0;

        int parsed_args = sscanf(str, "%i.%i.%i.%i:%i", &b1, &b2, &b3, &b4, &p);
        if (parsed_args != 4 && parsed_args != 5)
            return TCS_ERROR_INVALID_ARGUMENT;

        if ((uint8_t)(b1 & 0xFF) != b1 || (uint8_t)(b2 & 0xFF) != b2 || (uint8_t)(b3 & 0xFF) != b3 ||
            (uint8_t)(b4 & 0xFF) != b4)
            return TCS_ERROR_INVALID_ARGUMENT;
        if (p < 0 || p > 65535)
            return TCS_ERROR_INVALID_ARGUMENT;

        out_address->family = TCS_AF_IP4;
        out_address->data.ip4.address = (uint32_t)b1 << 24 | (uint32_t)b2 << 16 | (uint32_t)b3 << 8 | (uint32_t)b4;
        out_address->data.ip4.port = (uint16_t)p;
    }
    else if (is_ipv6)
    {
        // Table-driven DFA transducer (RFC 4291 + RFC 3986 + RFC 4007 + RFC 5952)

        // Character classes (CC_OTHER = 0 so uninitialized LUT entries default to it)
        enum
        {
            CC_OTHER = 0,
            CC_DIGIT,
            CC_HEX,
            CC_COLON,
            CC_DOT,
            CC_PERCENT,
            CC_LBRACKET,
            CC_RBRACKET,
            CC_NUL,
            CC_COUNT
        };

        // clang-format off
        static const uint8_t cc_lut[256] = {
        /* 0x00     NUL                                                                                                                                                                                   */
                    CC_NUL,     0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,

        /* 0x10                                                                                                                                                                                           */
                    0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,

        /* 0x20     SPACE       !           "           #           $           %           &           '           (           )           *           +           ,           -           .           / */
                    0,          0,          0,          0,          0,          CC_PERCENT, 0,          0,          0,          0,          0,          0,          0,          0,          CC_DOT,     0,

        /* 0x30     0           1           2           3           4           5           6           7           8           9           :           ;           <           =           >           ? */
                    CC_DIGIT,   CC_DIGIT,   CC_DIGIT,   CC_DIGIT,   CC_DIGIT,   CC_DIGIT,   CC_DIGIT,   CC_DIGIT,   CC_DIGIT,   CC_DIGIT,   CC_COLON,   0,          0,          0,          0,          0,

        /* 0x40     @           A           B           C           D           E           F           G           H           I           J           K           L           M           N           O */
                    0,          CC_HEX,     CC_HEX,     CC_HEX,     CC_HEX,     CC_HEX,     CC_HEX,     0,          0,          0,          0,          0,          0,          0,          0,          0,

        /* 0x50     P           Q           R           S           T           U           V           W           X           Y           Z           [           \           ]           ^           _ */
                    0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          0,          CC_LBRACKET,0,          CC_RBRACKET,0,          0,

        /* 0x60     `           a           b           c           d           e           f                         */
                    0,          CC_HEX,     CC_HEX,     CC_HEX,     CC_HEX,     CC_HEX,     CC_HEX, /*  ... zeros ... */
        };
        // clang-format on

        // DFA states
        enum
        {
            S_START,
            S_BRACKET,
            S_HEX,
            S_HEX_DEC,
            S_COLON,
            S_DCOLON,
            S_DOT,
            S_IPV4,
            S_RBRACKET,
            S_PORT_COLON,
            S_PORT,
            S_PERCENT,
            S_SCOPE,
            S_ACCEPT,
            S_REJECT,
            S_COUNT
        };

        // Actions
        enum
        {
            A_NONE,
            A_OPEN_BRACKET,
            A_HEX_ACCUM,
            A_HEX_DEC_ACCUM,
            A_COMMIT,
            A_COMMIT_BRACKET,
            A_SET_GAP,
            A_CHECK_BRACKET,
            A_START_IPV4,
            A_IPV4_ACCUM,
            A_IPV4_DOT,
            A_IPV4_DONE,
            A_IPV4_DONE_BRACKET,
            A_PORT_ACCUM,
            A_SCOPE_ACCUM,
            A_SCOPE_BRACKET
        };

        // clang-format off
        static const struct
        {
            /* StateType  */ uint8_t next;
            /* ActionType */ uint8_t action;
        } dfa_table[S_COUNT][CC_COUNT] /* States x Characters */ = {
            /*                   CC_OTHER              CC_DIGIT                    CC_HEX                CC_COLON                 CC_DOT               CC_PERCENT               CC_LBRACKET                CC_RBRACKET                         CC_NUL       */
            /* S_START     */ {{S_REJECT,A_NONE}, {S_HEX_DEC,A_HEX_DEC_ACCUM}, {S_HEX,A_HEX_ACCUM},   {S_COLON,A_NONE},       {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_BRACKET,A_OPEN_BRACKET}, {S_REJECT,A_NONE},               {S_REJECT,A_NONE}},
            /* S_BRACKET   */ {{S_REJECT,A_NONE}, {S_HEX_DEC,A_HEX_DEC_ACCUM}, {S_HEX,A_HEX_ACCUM},   {S_COLON,A_NONE},       {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_REJECT,A_NONE},               {S_REJECT,A_NONE}},
            /* S_HEX       */ {{S_REJECT,A_NONE}, {S_HEX,A_HEX_ACCUM},         {S_HEX,A_HEX_ACCUM},   {S_COLON,A_COMMIT},     {S_REJECT,A_NONE},     {S_PERCENT,A_COMMIT},   {S_REJECT,A_NONE},          {S_RBRACKET,A_COMMIT_BRACKET},   {S_ACCEPT,A_COMMIT}},
            /* S_HEX_DEC   */ {{S_REJECT,A_NONE}, {S_HEX_DEC,A_HEX_DEC_ACCUM}, {S_HEX,A_HEX_ACCUM},   {S_COLON,A_COMMIT},     {S_DOT,A_START_IPV4},  {S_PERCENT,A_COMMIT},   {S_REJECT,A_NONE},          {S_RBRACKET,A_COMMIT_BRACKET},   {S_ACCEPT,A_COMMIT}},
            /* S_COLON     */ {{S_REJECT,A_NONE}, {S_HEX_DEC,A_HEX_DEC_ACCUM}, {S_HEX,A_HEX_ACCUM},   {S_DCOLON,A_SET_GAP},   {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_REJECT,A_NONE},               {S_REJECT,A_NONE}},
            /* S_DCOLON    */ {{S_REJECT,A_NONE}, {S_HEX_DEC,A_HEX_DEC_ACCUM}, {S_HEX,A_HEX_ACCUM},   {S_REJECT,A_NONE},      {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_RBRACKET,A_CHECK_BRACKET},    {S_ACCEPT,A_NONE}},
            /* S_DOT       */ {{S_REJECT,A_NONE}, {S_IPV4,A_IPV4_ACCUM},       {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_REJECT,A_NONE},               {S_REJECT,A_NONE}},
            /* S_IPV4      */ {{S_REJECT,A_NONE}, {S_IPV4,A_IPV4_ACCUM},       {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_DOT,A_IPV4_DOT},    {S_PERCENT,A_IPV4_DONE},{S_REJECT,A_NONE},          {S_RBRACKET,A_IPV4_DONE_BRACKET},{S_ACCEPT,A_IPV4_DONE}},
            /* S_RBRACKET  */ {{S_REJECT,A_NONE}, {S_REJECT,A_NONE},           {S_REJECT,A_NONE},     {S_PORT_COLON,A_NONE},  {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_REJECT,A_NONE},               {S_ACCEPT,A_NONE}},
            /* S_PORT_COLON*/ {{S_REJECT,A_NONE}, {S_PORT,A_PORT_ACCUM},       {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_REJECT,A_NONE},               {S_REJECT,A_NONE}},
            /* S_PORT      */ {{S_REJECT,A_NONE}, {S_PORT,A_PORT_ACCUM},       {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_REJECT,A_NONE},               {S_ACCEPT,A_NONE}},
            /* S_PERCENT   */ {{S_REJECT,A_NONE}, {S_SCOPE,A_SCOPE_ACCUM},     {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_REJECT,A_NONE},               {S_REJECT,A_NONE}},
            /* S_SCOPE     */ {{S_REJECT,A_NONE}, {S_SCOPE,A_SCOPE_ACCUM},     {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_RBRACKET,A_SCOPE_BRACKET},    {S_ACCEPT,A_NONE}},
            /* S_ACCEPT    */ {{S_REJECT,A_NONE}, {S_REJECT,A_NONE},           {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_REJECT,A_NONE},               {S_REJECT,A_NONE}},
            /* S_REJECT    */ {{S_REJECT,A_NONE}, {S_REJECT,A_NONE},           {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},     {S_REJECT,A_NONE},      {S_REJECT,A_NONE},          {S_REJECT,A_NONE},               {S_REJECT,A_NONE}},
        };
        // clang-format on

        struct
        {
            uint16_t groups[8];
            int group_count;
            int gap_pos;
            int32_t hex_val;
            int32_t dec_val;
            int digits;
            int32_t port_val;
            int64_t scope_id;
            int32_t ipv4_octets[4];
            int ipv4_index;
            uint8_t state;
            bool in_bracket;
        } ctx = {{0}, 0, -1, 0, 0, 0, 0, 0, {0}, 0, S_START, false};

        const char* c = str;

        while (ctx.state != S_ACCEPT && ctx.state != S_REJECT)
        {
            int cc = cc_lut[(unsigned char)*c];

            int next = dfa_table[ctx.state][cc].next;
            int action = dfa_table[ctx.state][cc].action;

            // Execute action
            switch (action)
            {
                case A_NONE:
                    break;
                case A_OPEN_BRACKET:
                    ctx.in_bracket = true;
                    break;
                case A_HEX_ACCUM: {
                    int32_t d = (*c >= '0' && *c <= '9')   ? (*c - '0')
                                : (*c >= 'a' && *c <= 'f') ? (*c - 'a' + 10)
                                                           : (*c - 'A' + 10);
                    ctx.hex_val = (ctx.hex_val << 4) | d;
                    ctx.digits++;
                    if (ctx.digits > 4)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    break;
                }
                case A_HEX_DEC_ACCUM: {
                    int32_t d = *c - '0';
                    ctx.hex_val = (ctx.hex_val << 4) | d;
                    ctx.dec_val = ctx.dec_val * 10 + d;
                    ctx.digits++;
                    if (ctx.digits > 4)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    break;
                }
                case A_COMMIT:
                    if (ctx.group_count >= 8)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    ctx.groups[ctx.group_count++] = (uint16_t)ctx.hex_val;
                    ctx.hex_val = 0;
                    ctx.dec_val = 0;
                    ctx.digits = 0;
                    break;
                case A_COMMIT_BRACKET:
                    if (!ctx.in_bracket)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    if (ctx.group_count >= 8)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    ctx.groups[ctx.group_count++] = (uint16_t)ctx.hex_val;
                    ctx.hex_val = 0;
                    ctx.dec_val = 0;
                    ctx.digits = 0;
                    ctx.in_bracket = false;
                    break;
                case A_SET_GAP:
                    if (ctx.gap_pos >= 0)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    ctx.gap_pos = ctx.group_count;
                    break;
                case A_CHECK_BRACKET:
                    if (!ctx.in_bracket)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    ctx.in_bracket = false;
                    break;
                case A_START_IPV4:
                    if (ctx.dec_val > 255)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    ctx.ipv4_octets[0] = ctx.dec_val;
                    ctx.ipv4_index = 1;
                    ctx.dec_val = 0;
                    ctx.hex_val = 0;
                    ctx.digits = 0;
                    break;
                case A_IPV4_ACCUM:
                    ctx.dec_val = ctx.dec_val * 10 + (*c - '0');
                    if (ctx.dec_val > 255)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    break;
                case A_IPV4_DOT:
                    if (ctx.ipv4_index >= 4)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    ctx.ipv4_octets[ctx.ipv4_index++] = ctx.dec_val;
                    ctx.dec_val = 0;
                    break;
                case A_IPV4_DONE:
                case A_IPV4_DONE_BRACKET:
                    if (action == A_IPV4_DONE_BRACKET)
                    {
                        if (!ctx.in_bracket)
                            return TCS_ERROR_INVALID_ARGUMENT;
                        ctx.in_bracket = false;
                    }
                    if (ctx.ipv4_index != 3)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    ctx.ipv4_octets[3] = ctx.dec_val;
                    if (ctx.group_count + 2 > 8)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    ctx.groups[ctx.group_count++] = (uint16_t)(ctx.ipv4_octets[0] << 8 | ctx.ipv4_octets[1]);
                    ctx.groups[ctx.group_count++] = (uint16_t)(ctx.ipv4_octets[2] << 8 | ctx.ipv4_octets[3]);
                    break;
                case A_PORT_ACCUM:
                    ctx.port_val = ctx.port_val * 10 + (*c - '0');
                    if (ctx.port_val > UINT16_MAX)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    break;
                case A_SCOPE_ACCUM:
                    ctx.scope_id = ctx.scope_id * 10 + (*c - '0');
                    if (ctx.scope_id > UINT32_MAX)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    break;
                case A_SCOPE_BRACKET:
                    if (!ctx.in_bracket)
                        return TCS_ERROR_INVALID_ARGUMENT;
                    ctx.in_bracket = false;
                    break;
            }

            ctx.state = (uint8_t)next;
            if (*c != '\0')
                c++;
        }

        if (ctx.state == S_REJECT)
            return TCS_ERROR_INVALID_ARGUMENT;

        // Unclosed bracket
        if (ctx.in_bracket)
            return TCS_ERROR_INVALID_ARGUMENT;

        // Expand :: gap into zero groups
        if (ctx.gap_pos >= 0)
        {
            int32_t gap_size = 8 - ctx.group_count;
            if (gap_size < 1)
                return TCS_ERROR_INVALID_ARGUMENT;
            int32_t after_gap = ctx.group_count - ctx.gap_pos;
            for (int32_t i = after_gap - 1; i >= 0; i--)
                ctx.groups[ctx.gap_pos + gap_size + i] = ctx.groups[ctx.gap_pos + i];
            for (int32_t i = 0; i < gap_size; i++)
                ctx.groups[ctx.gap_pos + i] = 0;
        }
        else if (ctx.group_count != 8)
        {
            return TCS_ERROR_INVALID_ARGUMENT;
        }

        out_address->family = TCS_AF_IP6;
        for (int i = 0; i < 8; i++)
        {
            out_address->data.ip6.address.bytes[i * 2] = (uint8_t)(ctx.groups[i] >> 8);
            out_address->data.ip6.address.bytes[i * 2 + 1] = (uint8_t)(ctx.groups[i] & 0xFF);
        }
        out_address->data.ip6.port = (uint16_t)ctx.port_val;
        out_address->data.ip6.scope_id = (TcsInterfaceId)ctx.scope_id;
    }
    else if (is_mac)
    {
        unsigned int b1;
        unsigned int b2;
        unsigned int b3;
        unsigned int b4;
        unsigned int b5;
        unsigned int b6;

        int parsed_args = sscanf(str, "%x:%x:%x:%x:%x:%x", &b1, &b2, &b3, &b4, &b5, &b6);
        if (parsed_args != 6)
            return TCS_ERROR_INVALID_ARGUMENT;

        if ((uint8_t)(b1 & 0xFF) != b1 || (uint8_t)(b2 & 0xFF) != b2 || (uint8_t)(b3 & 0xFF) != b3 ||
            (uint8_t)(b4 & 0xFF) != b4 || (uint8_t)(b5 & 0xFF) != b5 || (uint8_t)(b6 & 0xFF) != b6)
            return TCS_ERROR_INVALID_ARGUMENT;

        out_address->family = TCS_AF_PACKET;
        out_address->data.packet.mac[0] = (uint8_t)b1;
        out_address->data.packet.mac[1] = (uint8_t)b2;
        out_address->data.packet.mac[2] = (uint8_t)b3;
        out_address->data.packet.mac[3] = (uint8_t)b4;
        out_address->data.packet.mac[4] = (uint8_t)b5;
        out_address->data.packet.mac[5] = (uint8_t)b6;
        out_address->data.packet.interface_id = 0; // Must be set before use
        out_address->data.packet.protocol = 0;     // Must be set before use

        // TODO: Check if we have a local interface with this mac address
    }
    else
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }

    return TCS_SUCCESS;
}

TcsResult tcs_address_to_str(const struct TcsAddress* address, char str[70])
{
    if (address == NULL || str == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    memset(str, 0, 70);
    if (address->family == TCS_AF_IP4)
    {
        uint32_t d = address->data.ip4.address;
        uint16_t p = address->data.ip4.port;
        uint8_t b1 = (uint8_t)(d & 0xFF);
        uint8_t b2 = (uint8_t)((d >> 8) & 0xFF);
        uint8_t b3 = (uint8_t)((d >> 16) & 0xFF);
        uint8_t b4 = (uint8_t)((d >> 24) & 0xFF);
        if (p == 0)
            snprintf(str, 70, "%i.%i.%i.%i", b4, b3, b2, b1);
        else
            snprintf(str, 70, "%i.%i.%i.%i:%i", b4, b3, b2, b1, p);
    }
    else if (address->family == TCS_AF_IP6)
    {
        uint16_t groups[8];
        for (int i = 0; i < 8; i++)
            groups[i] = (uint16_t)((unsigned int)address->data.ip6.address.bytes[i * 2] << 8 |
                                   address->data.ip6.address.bytes[i * 2 + 1]);

        // Find longest run of consecutive zero groups for :: compression (RFC 5952)
        int best_start = -1;
        int best_len = 0;
        int cur_start = -1;
        int cur_len = 0;
        for (int i = 0; i < 8; i++)
        {
            if (groups[i] == 0)
            {
                if (cur_start < 0)
                    cur_start = i;
                cur_len++;
                if (cur_len > best_len)
                {
                    best_start = cur_start;
                    best_len = cur_len;
                }
            }
            else
            {
                cur_start = -1;
                cur_len = 0;
            }
        }
        if (best_len < 2)
            best_start = -1;

        char addr_str[46];
        int pos = 0;
        for (int i = 0; i < 8; i++)
        {
            if (i == best_start)
            {
                pos += snprintf(addr_str + pos, sizeof addr_str - (size_t)pos, "::");
                i += best_len - 1;
                continue;
            }
            if (i > 0 && (best_start < 0 || i != best_start + best_len))
                addr_str[pos++] = ':';
            pos += snprintf(addr_str + pos, sizeof addr_str - (size_t)pos, "%x", (unsigned int)groups[i]);
        }
        addr_str[pos] = '\0';

        uint16_t p = address->data.ip6.port;
        TcsInterfaceId sc = address->data.ip6.scope_id;
        if (p != 0 && sc != 0)
            snprintf(str, 70, "[%s%%%u]:%u", addr_str, (unsigned int)sc, (unsigned int)p);
        else if (p != 0)
            snprintf(str, 70, "[%s]:%u", addr_str, (unsigned int)p);
        else if (sc != 0)
            snprintf(str, 70, "%s%%%u", addr_str, (unsigned int)sc);
        else
            snprintf(str, 70, "%s", addr_str);
    }
    else if (address->family == TCS_AF_PACKET)
    {
        snprintf(str,
                 70,
                 "%02X:%02X:%02X:%02X:%02X:%02X",
                 address->data.packet.mac[0],
                 address->data.packet.mac[1],
                 address->data.packet.mac[2],
                 address->data.packet.mac[3],
                 address->data.packet.mac[4],
                 address->data.packet.mac[5]);
    }
    else
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }

    return TCS_SUCCESS;
}

bool tcs_address_is_equal(const struct TcsAddress* l, const struct TcsAddress* r)
{
    if (l == r) // pointer equality also covers NULL == NULL
        return true;
    if (l == NULL || r == NULL)
        return false;
    if (l->family != r->family)
        return false;

    switch (l->family)
    {
        case TCS_AF_ANY:
            return true; // We consider any address equal to any address
        case TCS_AF_IP4:
            return l->data.ip4.address == r->data.ip4.address && l->data.ip4.port == r->data.ip4.port;
        case TCS_AF_IP6:
            return memcmp(l->data.ip6.address.bytes, r->data.ip6.address.bytes, 16) == 0 &&
                   l->data.ip6.port == r->data.ip6.port;
        case TCS_AF_PACKET:
            return memcmp(l->data.packet.mac, r->data.packet.mac, 6) == 0 &&
                   l->data.packet.protocol == r->data.packet.protocol &&
                   l->data.packet.interface_id == r->data.packet.interface_id;
        default:
            return false;
    }
}

bool tcs_address_is_any(const struct TcsAddress* addr)
{
    if (addr == NULL)
        return false;
    switch (addr->family)
    {
        case TCS_AF_IP4:
            return addr->data.ip4.address == TCS_ADDRESS_ANY_IP4;
        case TCS_AF_IP6: {
            static const uint8_t any6[16] = {0};
            return memcmp(addr->data.ip6.address.bytes, any6, 16) == 0;
        }
        default:
            return false;
    }
}

bool tcs_address_is_local(const struct TcsAddress* addr)
{
    if (addr == NULL)
        return false;
    switch (addr->family)
    {
        case TCS_AF_IP4:
            return (addr->data.ip4.address >> 16) == 0xA9FE; // 169.254.0.0/16
        case TCS_AF_IP6:
            return addr->data.ip6.address.bytes[0] == 0xFE &&
                   (addr->data.ip6.address.bytes[1] & 0xC0) == 0x80; // fe80::/10
        default:
            return false;
    }
}

bool tcs_address_is_loopback(const struct TcsAddress* addr)
{
    if (addr == NULL)
        return false;
    switch (addr->family)
    {
        case TCS_AF_IP4:
            return addr->data.ip4.address == TCS_ADDRESS_LOOPBACK_IP4;
        case TCS_AF_IP6:
            return addr->data.ip6.address.bytes[0] == 0 && addr->data.ip6.address.bytes[1] == 0 &&
                   addr->data.ip6.address.bytes[2] == 0 && addr->data.ip6.address.bytes[3] == 0 &&
                   addr->data.ip6.address.bytes[4] == 0 && addr->data.ip6.address.bytes[5] == 0 &&
                   addr->data.ip6.address.bytes[6] == 0 && addr->data.ip6.address.bytes[7] == 0 &&
                   addr->data.ip6.address.bytes[8] == 0 && addr->data.ip6.address.bytes[9] == 0 &&
                   addr->data.ip6.address.bytes[10] == 0 && addr->data.ip6.address.bytes[11] == 0 &&
                   addr->data.ip6.address.bytes[12] == 0 && addr->data.ip6.address.bytes[13] == 0 &&
                   addr->data.ip6.address.bytes[14] == 0 && addr->data.ip6.address.bytes[15] == 1;
        case TCS_AF_PACKET: {
            static const uint8_t zero_mac[6] = {0};
            return memcmp(addr->data.packet.mac, zero_mac, 6) == 0;
        }
        default:
            return false;
    }
}

bool tcs_address_is_multicast(const struct TcsAddress* addr)
{
    if (addr == NULL)
        return false;
    switch (addr->family)
    {
        case TCS_AF_IP4:
            return (addr->data.ip4.address >> 24) >= 224 && (addr->data.ip4.address >> 24) <= 239;
        case TCS_AF_IP6:
            return addr->data.ip6.address.bytes[0] == 0xFF;
        case TCS_AF_PACKET:
            return (addr->data.packet.mac[0] & 0x01) != 0;
        default:
            return false;
    }
}

bool tcs_address_is_broadcast(const struct TcsAddress* addr)
{
    if (addr == NULL)
        return false;
    switch (addr->family)
    {
        case TCS_AF_IP4:
            return addr->data.ip4.address == TCS_ADDRESS_BROADCAST_IP4;
        case TCS_AF_PACKET:
            return addr->data.packet.mac[0] == 0xFF && addr->data.packet.mac[1] == 0xFF &&
                   addr->data.packet.mac[2] == 0xFF && addr->data.packet.mac[3] == 0xFF &&
                   addr->data.packet.mac[4] == 0xFF && addr->data.packet.mac[5] == 0xFF;
        default:
            return false;
    }
}
