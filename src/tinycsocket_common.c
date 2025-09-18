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
        case TCS_PRESET_PACKET:
            family = TCS_AF_PACKET;
            type = TCS_SOCK_RAW;
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
            break;
    }
    return tcs_socket(socket_ctx, family, type, protocol);
}

// tcs_close() is defined in OS specific files

// ######## High-level Socket Creation ########

TcsResult tcs_tcp_server_str(TcsSocket* socket_ctx, const char* local_address, uint16_t port)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_tcp_server(TcsSocket* socket_ctx, const struct TcsAddress* local_address)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_tcp_client_str(TcsSocket* socket_ctx, const char* remote_address, uint16_t port, int timeout_ms)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_tcp_client(TcsSocket* socket_ctx, const struct TcsAddress* remote_address, int timeout_ms)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_udp_receiver_str(TcsSocket* socket_ctx, const char* local_address, uint16_t port)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_udp_receiver(TcsSocket* socket_ctx, const struct TcsAddress* local_address)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_udp_sender_str(TcsSocket* socket_ctx, const char* remote_address, uint16_t port)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_udp_sender(TcsSocket* socket_ctx, const struct TcsAddress* remote_address)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_udp_peer_str(TcsSocket* socket_ctx,
                           const char* local_address,
                           uint16_t local_port,
                           const char* remote_address,
                           uint16_t remote_port)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_udp_peer(TcsSocket* socket_ctx,
                       const struct TcsAddress* local_address,
                       const struct TcsAddress* remote_address)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

// ######## High-level Raw L2-Packet Sockets (Experimental) ########

TcsResult tcs_packet_sender_str(TcsSocket* socket_ctx,
                                const char* interface_name,
                                const uint8_t destination_mac[6],
                                uint16_t protocol)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_packet_sender(TcsSocket* socket_ctx, const struct TcsAddress* remote_address)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_packet_peer_str(TcsSocket* socket_ctx,
                              const char* interface_name,
                              const uint8_t destination_mac[6],
                              uint16_t protocol)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_packet_peer(TcsSocket* socket_ctx,
                          const struct TcsAddress* local_address,
                          const struct TcsAddress* remote_address)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_packet_capture_iface(TcsSocket* socket_ctx, const struct TcsInterface* iface)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_packet_capture_ifname(TcsSocket* socket_ctx, const char* interface_name)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
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

    struct TcsAddress found_addresses[32];
    memset(found_addresses, 0, sizeof found_addresses);
    size_t no_of_found_addresses = 0;
    TcsResult sts = tcs_address_resolve(remote_address, TCS_AF_IP4, found_addresses, 32, &no_of_found_addresses);
    if (sts != TCS_SUCCESS)
        return sts;

    for (size_t i = 0; i < no_of_found_addresses; ++i)
    {
        found_addresses[i].data.ip4.port = port;
        if (tcs_connect(socket_ctx, &found_addresses[i]) == TCS_SUCCESS)
            return TCS_SUCCESS;
    }

    return TCS_ERROR_CONNECTION_REFUSED;
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
    if (socket_ctx == TCS_SOCKET_INVALID || buffer == NULL || buffer_length <= 0)
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
    if (socket_ctx == TCS_SOCKET_INVALID || buffer == NULL || buffer_length <= 0)
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

        expected_length += (size_t)t - '0';
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

TcsResult tcs_opt_reuse_address_set(TcsSocket socket_ctx, bool do_allow_reuse_address)
{
    if (socket_ctx == TCS_SOCKET_INVALID)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = do_allow_reuse_address ? 1 : 0;
    return tcs_opt_set(socket_ctx, TCS_SOL_SOCKET, TCS_SO_REUSEADDR, &b, sizeof(b));
}

TcsResult tcs_opt_reuse_address_get(TcsSocket socket_ctx, bool* is_reuse_address_allowed)
{
    if (socket_ctx == TCS_SOCKET_INVALID || is_reuse_address_allowed == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsResult sts = tcs_opt_get(socket_ctx, TCS_SOL_SOCKET, TCS_SO_REUSEADDR, &b, &s);
    *is_reuse_address_allowed = b;
    return sts;
}

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

TcsResult tcs_opt_nonblocking_set(TcsSocket socket_ctx, bool do_non_blocking)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

TcsResult tcs_opt_nonblocking_get(TcsSocket socket_ctx, bool* is_non_blocking)
{
    return TCS_ERROR_NOT_IMPLEMENTED;
}

// tcs_opt_membership_add() is defined in OS specific files
// tcs_opt_membership_add_to() is defined in OS specific files
// tcs_opt_membership_drop() is defined in OS specific files
// tcs_opt_membership_drop_from() is defined in OS specific files

// ######## Address and Interface Utilities ########

// tcs_interface_list() is defined in OS specific files
// tcs_address_resolve() is defined in OS specific files
// tcs_address_resolve_timeout() is defined in OS specific files
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

    for (int i = 0; i < 21; ++i) // max ipv4 string length with port colon
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

    // AVTP Multicast address format:
    // 91:E0:F0:00:FE:00 - 91:E0:F0:00:FE:FF

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
        return TCS_ERROR_NOT_IMPLEMENTED;
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
    if (str == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    memset(str, 0, 40);
    if (address->family == TCS_AF_IP4)
    {
        uint32_t d = address->data.ip4.address;
        uint16_t p = address->data.ip4.port;
        uint8_t b1 = (uint8_t)(d & 0xFF);
        uint8_t b2 = (uint8_t)((d >> 8) & 0xFF);
        uint8_t b3 = (uint8_t)((d >> 16) & 0xFF);
        uint8_t b4 = (uint8_t)((d >> 24) & 0xFF);
        if (p == 0)
            sprintf(str, "%i.%i.%i.%i", b4, b3, b2, b1);
        else
            sprintf(str, "%i.%i.%i.%i:%i", b4, b3, b2, b1, p);
    }
    else if (address->family == TCS_AF_IP6)
    {
        return TCS_ERROR_NOT_IMPLEMENTED;
    }
    else if (address->family == TCS_AF_PACKET)
    {
        sprintf(str,
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
            return memcmp(l->data.ip6.address, r->data.ip6.address, 16) == 0 && l->data.ip6.port == r->data.ip6.port;
        case TCS_AF_PACKET:
            return memcmp(l->data.packet.mac, r->data.packet.mac, 6) == 0 &&
                   l->data.packet.protocol == r->data.packet.protocol &&
                   l->data.packet.interface_id == r->data.packet.interface_id;
        default:
            return false;
    }
    return false;
}

bool tcs_address_is_any(const struct TcsAddress* addr)
{
    return false; // Not implemented
}

bool tcs_address_is_local(const struct TcsAddress* addr)
{
    return false; // Not implemented
}
bool tcs_address_is_loopback(const struct TcsAddress* addr)
{
    return false; // Not implemented
}
bool tcs_address_is_multicast(const struct TcsAddress* addr)
{
    return false; // Not implemented
}
bool tcs_address_is_broadcast(const struct TcsAddress* addr)
{
    return false; // Not implemented
}
