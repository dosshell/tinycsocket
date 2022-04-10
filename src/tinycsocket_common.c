#include "tinycsocket.h"

// This file should never call OS dependent code. Do not include OS files of OS specific ifdefs

#ifdef DO_WRAP
#include "dbg_wrap.h"
#endif

#include <stdbool.h>
#include <stdio.h>  //sprintf
#include <string.h> // memset

TcsReturnCode tcs_util_ipv4_args(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint32_t* out_address)
{
    if (out_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    *out_address = (uint32_t)a << 24 | (uint32_t)b << 16 | (uint32_t)c << 8 | d;
    return TCS_SUCCESS;
}

TcsReturnCode tcs_util_string_to_address(const char str[], struct TcsAddress* parsed_address)
{
    if (parsed_address == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    memset(parsed_address, 0, sizeof(struct TcsAddress));
    // Slow but easy parser
    int n_colons = 0;
    int n_dots = 0;

    for (int i = 0; i < 21; ++i) // max ipv4 string length with port colon
    {
        if (str[i] == '.')
            n_dots++;
        else if (str[i] == ':')
            n_colons++;
    }
    bool is_ipv4 = n_dots > 0;
    //bool is_ipv6 = n_colons > 1;

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

        parsed_address->family = TCS_AF_IP4;
        if (tcs_util_ipv4_args(
                (uint8_t)b1, (uint8_t)b2, (uint8_t)b3, (uint8_t)b4, &parsed_address->data.af_inet.address) !=
            TCS_SUCCESS)
            return TCS_ERROR_UNKNOWN;
        parsed_address->data.af_inet.port = (uint16_t)p;
    }
    else
        return TCS_ERROR_NOT_IMPLEMENTED;

    return TCS_SUCCESS;
}

TcsReturnCode tcs_util_address_to_string(const struct TcsAddress* address, char str[40])
{
    if (str == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    memset(str, 0, 40);
    if (address->family == TCS_AF_IP4)
    {
        uint32_t d = address->data.af_inet.address;
        uint16_t p = address->data.af_inet.port;
        uint8_t b1 = (uint8_t)(d & 0xFF);
        uint8_t b2 = (uint8_t)((d >> 8) & 0xFF);
        uint8_t b3 = (uint8_t)((d >> 16) & 0xFF);
        uint8_t b4 = (uint8_t)((d >> 24) & 0xFF);
        if (p == 0)
            sprintf(str, "%i.%i.%i.%i", b4, b3, b2, b1);
        else
            sprintf(str, "%i.%i.%i.%i:%i", b4, b3, b2, b1, p);
    }
    else
        return TCS_ERROR_NOT_IMPLEMENTED;

    return TCS_SUCCESS;
}

TcsReturnCode tcs_create(TcsSocket* socket_ctx, TcsType socket_type)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsAddressFamily family = TCS_AF_ANY;
    int type = 0;
    int protocol = 0;

    switch (socket_type)
    {
        case TCS_TYPE_TCP_IP4:
            family = TCS_AF_IP4;
            type = TCS_SOCK_STREAM;
            protocol = TCS_IPPROTO_TCP;
            break;
        case TCS_TYPE_UDP_IP4:
            family = TCS_AF_IP4;
            type = TCS_SOCK_DGRAM;
            protocol = TCS_IPPROTO_UDP;
            break;
        case TCS_TYPE_TCP_IP6:
        case TCS_TYPE_UDP_IP6:
        default:
            return TCS_ERROR_NOT_IMPLEMENTED;
            break;
    }
    return tcs_create_ext(socket_ctx, family, type, protocol);
}

TcsReturnCode tcs_connect(TcsSocket socket_ctx, const char* hostname, uint16_t port)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress found_addresses[32] = {0};
    size_t no_of_found_addresses = 0;
    TcsAddressFamily family = TCS_AF_IP4;
    int sts = tcs_resolve_hostname(hostname, family, found_addresses, 32, &no_of_found_addresses);
    if (sts != TCS_SUCCESS)
        return sts;

    for (size_t i = 0; i < no_of_found_addresses; ++i)
    {
        found_addresses[i].data.af_inet.port = port;
        if (tcs_connect_address(socket_ctx, &found_addresses[i]) == TCS_SUCCESS)
            return TCS_SUCCESS;
    }

    return TCS_ERROR_CONNECTION_REFUSED;
}

TcsReturnCode tcs_bind(TcsSocket socket_ctx, uint16_t port)
{
    if (socket_ctx == TCS_NULLSOCKET || port == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress local_address = {0};
    local_address.family = TCS_AF_IP4;
    local_address.data.af_inet.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.af_inet.port = port;

    return tcs_bind_address(socket_ctx, &local_address);
}

TcsReturnCode tcs_listen_to(TcsSocket socket_ctx, uint16_t local_port)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    TcsReturnCode sts = tcs_bind(socket_ctx, local_port);
    if (sts != TCS_SUCCESS)
        return sts;

    return tcs_listen(socket_ctx, TCS_BACKLOG_SOMAXCONN);
}

TcsReturnCode tcs_receive_netstring(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_length, size_t* bytes_received)
{
    if (socket_ctx == TCS_NULLSOCKET || buffer == NULL || buffer_length <= 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    size_t expected_length = 0;
    int parsed = 0;
    int sts = 0;
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

TcsReturnCode tcs_send_netstring(TcsSocket socket_ctx, const uint8_t* buffer, size_t buffer_length)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    if (buffer == NULL || buffer_length == 0)
        return TCS_ERROR_INVALID_ARGUMENT;

#if SIZE_MAX > 0xffffffffffffffffULL
    // buffer_length bigger than 64 bits? (size_t can be bigger on some systems)
    if (buffer_length > 0xffffffffffffffffULL)
        return TCS_ERROR_INVALID_ARGUMENT;
#endif

    int header_length = 0;
    char netstring_header[21] = {0};

    // %zu is not supported by all compilers, therefor we cast it to llu
    header_length = snprintf(netstring_header, 21, "%llu:", (unsigned long long)buffer_length);

    if (header_length < 0)
        return TCS_ERROR_INVALID_ARGUMENT;

    int sts = 0;
    sts = tcs_send(socket_ctx, (uint8_t*)netstring_header, (size_t)header_length, TCS_MSG_SENDALL, NULL);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_send(socket_ctx, buffer, buffer_length, TCS_MSG_SENDALL, NULL);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_send(socket_ctx, (uint8_t*)",", 1, TCS_MSG_SENDALL, NULL);
    if (sts != TCS_SUCCESS)
        return sts;

    return TCS_SUCCESS;
}

TcsReturnCode tcs_set_broadcast(TcsSocket socket_ctx, bool do_allow_broadcast)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = do_allow_broadcast ? 1 : 0;
    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_BROADCAST, &b, sizeof(b));
}

TcsReturnCode tcs_get_broadcast(TcsSocket socket_ctx, bool* is_broadcast_allowed)
{
    if (socket_ctx == TCS_NULLSOCKET || is_broadcast_allowed == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_BROADCAST, &b, &s);
    *is_broadcast_allowed = b;
    return sts;
}

TcsReturnCode tcs_set_keep_alive(TcsSocket socket_ctx, bool do_keep_alive)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = do_keep_alive ? 1 : 0;
    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_KEEPALIVE, &b, sizeof(b));
}

TcsReturnCode tcs_get_keep_alive(TcsSocket socket_ctx, bool* is_keep_alive_enabled)
{
    if (socket_ctx == TCS_NULLSOCKET || is_keep_alive_enabled == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_KEEPALIVE, &b, &s);
    *is_keep_alive_enabled = b;
    return sts;
}

TcsReturnCode tcs_set_reuse_address(TcsSocket socket_ctx, bool do_allow_reuse_address)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = do_allow_reuse_address ? 1 : 0;
    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_REUSEADDR, &b, sizeof(b));
}

TcsReturnCode tcs_get_reuse_address(TcsSocket socket_ctx, bool* is_reuse_address_allowed)
{
    if (socket_ctx == TCS_NULLSOCKET || is_reuse_address_allowed == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_REUSEADDR, &b, &s);
    *is_reuse_address_allowed = b;
    return sts;
}

TcsReturnCode tcs_set_send_buffer_size(TcsSocket socket_ctx, size_t send_buffer_size)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    unsigned int b = (unsigned int)send_buffer_size;
    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_SNDBUF, &b, sizeof(b));
}

TcsReturnCode tcs_get_send_buffer_size(TcsSocket socket_ctx, size_t* send_buffer_size)
{
    if (socket_ctx == TCS_NULLSOCKET || send_buffer_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    unsigned int b = 0;
    size_t s = sizeof(b);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_SNDBUF, &b, &s);
    *send_buffer_size = (size_t)b;
    return sts;
}

TcsReturnCode tcs_set_receive_buffer_size(TcsSocket socket_ctx, size_t receive_buffer_size)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    unsigned int b = (unsigned int)receive_buffer_size;
    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVBUF, &b, sizeof(b));
}

TcsReturnCode tcs_get_receive_buffer_size(TcsSocket socket_ctx, size_t* receive_buffer_size)
{
    if (socket_ctx == TCS_NULLSOCKET || receive_buffer_size == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    unsigned int b = 0;
    size_t s = sizeof(b);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_RCVBUF, &b, &s);
    *receive_buffer_size = (size_t)b;
    return sts;
}

TcsReturnCode tcs_set_ip_no_delay(TcsSocket socket_ctx, bool use_no_delay)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = use_no_delay ? 1 : 0;
    return tcs_set_option(socket_ctx, TCS_SOL_IP, TCS_SO_IP_NODELAY, &b, sizeof(b));
}

TcsReturnCode tcs_get_ip_no_delay(TcsSocket socket_ctx, bool* is_no_delay_used)
{
    if (socket_ctx == TCS_NULLSOCKET || is_no_delay_used == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_IP, TCS_SO_IP_NODELAY, &b, &s);
    *is_no_delay_used = b;
    return sts;
}

TcsReturnCode tcs_set_out_of_band_inline(TcsSocket socket_ctx, bool enable_oob)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    int b = enable_oob ? 1 : 0;
    return tcs_set_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_OOBINLINE, &b, sizeof(b));
}

TcsReturnCode tcs_get_out_of_band_inline(TcsSocket socket_ctx, bool* is_oob_enabled)
{
    if (socket_ctx == TCS_NULLSOCKET || is_oob_enabled == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;
    int b = 0;
    size_t s = sizeof(b);
    TcsReturnCode sts = tcs_get_option(socket_ctx, TCS_SOL_SOCKET, TCS_SO_OOBINLINE, &b, &s);
    *is_oob_enabled = b;
    return sts;
}
