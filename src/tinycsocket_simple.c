#include "tinycsocket.h"

#include <stdbool.h>
#include <stdio.h> //sprintf

TcsReturnCode tcs_simple_create_and_connect(TcsSocket* socket_ctx,
                                            const char* hostname,
                                            const char* port,
                                            uint16_t family)
{
    if (socket_ctx == NULL)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress found_addresses[32] = {0};
    size_t no_of_found_addresses = 0;
    int sts = tcs_getaddrinfo(hostname, port, family, found_addresses, 32, &no_of_found_addresses);
    if (sts != TCS_SUCCESS)
        return sts;

    for (size_t i = 0; i < no_of_found_addresses; ++i)
    {
        sts = tcs_create(socket_ctx, found_addresses[i].family, TCS_SOCK_STREAM, 0);
        if (sts != TCS_SUCCESS)
        {
            continue;
        }
        if (tcs_connect(*socket_ctx, &found_addresses[i]) == TCS_SUCCESS)
        {
            return TCS_SUCCESS;
        }
        else
        {
            tcs_close(socket_ctx);
        }
    }

    return TCS_ERROR_CONNECTION_REFUSED;
}

int tcs_simple_create_and_bind(TcsSocket* socket_ctx, const char* hostname, const char* port, uint16_t family)
{
    struct TcsAddress found_addresses[32] = {0};
    size_t no_of_found_addresses = 0;
    int sts = tcs_getaddrinfo(hostname, port, family, found_addresses, 32, &no_of_found_addresses);
    if (sts != TCS_SUCCESS)
        return sts;

    bool is_bounded = false;
    for (size_t i = 0; i < no_of_found_addresses; ++i)
    {
        if (tcs_create(socket_ctx, found_addresses[i].family, TCS_SOCK_DGRAM, 0) != TCS_SUCCESS)
            continue;

        if (tcs_bind(*socket_ctx, &found_addresses[i]) != TCS_SUCCESS)
        {
            tcs_close(socket_ctx);
            continue;
        }

        is_bounded = true;
        break;
    }

    if (!is_bounded)
    {
        return TCS_ERROR_UNKNOWN;
    }

    return TCS_SUCCESS;
}

int tcs_simple_create_and_listen(TcsSocket* socket_ctx, const char* hostname, const char* port, uint16_t family)
{
    if (socket_ctx == NULL || *socket_ctx != TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    struct TcsAddress found_address = {0};

    int sts = 0;
    sts = tcs_getaddrinfo(hostname, port, family, &found_address, 1, NULL);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_create(socket_ctx, found_address.family, TCS_SOCK_STREAM, 0);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_bind(*socket_ctx, &found_address);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_listen(*socket_ctx, TCS_BACKLOG_SOMAXCONN);
    if (sts != TCS_SUCCESS)
        return sts;

    return TCS_SUCCESS;
}

int tcs_simple_recv_all(TcsSocket socket_ctx, uint8_t* buffer, size_t length)
{
    if (socket_ctx == TCS_NULLSOCKET)
        return TCS_ERROR_INVALID_ARGUMENT;

    size_t bytes_left = length;
    while (bytes_left > 0)
    {
        size_t bytes_received = 0;
        int sts = tcs_recv(socket_ctx, buffer + bytes_received, bytes_left, 0, &bytes_received);
        if (sts != TCS_SUCCESS)
            return sts;

        bytes_left -= bytes_received;
    }
    return TCS_SUCCESS;
}

int tcs_simple_send_all(TcsSocket socket_ctx, uint8_t* buffer, size_t length, uint32_t flags)
{
    size_t left = length;
    size_t sent = 0;

    while (left > 0)
    {
        int sts = tcs_send(socket_ctx, buffer, length, flags, &sent);
        if (sts != TCS_SUCCESS)
            return sts;

        left -= sent;
    }
    return TCS_SUCCESS;
}

int tcs_simple_recv_netstring(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_length, size_t* bytes_received)
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
        sts = tcs_simple_recv_all(socket_ctx, (uint8_t*)&t, 1);
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

    sts = tcs_simple_recv_all(socket_ctx, buffer, expected_length);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_simple_recv_all(socket_ctx, (uint8_t*)&t, 1);
    if (sts != TCS_SUCCESS)
        return sts;

    if (t != ',')
        return TCS_ERROR_ILL_FORMED_MESSAGE;

    if (bytes_received != NULL)
        *bytes_received = expected_length;

    return TCS_SUCCESS;
}

int tcs_simple_send_netstring(TcsSocket socket_ctx, uint8_t* buffer, size_t buffer_length)
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
    sts = tcs_simple_send_all(socket_ctx, (uint8_t*)netstring_header, (size_t)header_length, 0);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_simple_send_all(socket_ctx, buffer, buffer_length, 0);
    if (sts != TCS_SUCCESS)
        return sts;

    sts = tcs_simple_send_all(socket_ctx, (uint8_t*)",", 1, 0);
    if (sts != TCS_SUCCESS)
        return sts;

    return TCS_SUCCESS;
}
