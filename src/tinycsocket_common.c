#include "tinycsocket.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

int tinycsocket_bind_and_listen(TinyCSocketCtx* socket_ctx, const char* address, const char* port)
{
    int bind_status = tinycsocket_bind(socket_ctx, address, port);
    if (bind_status != TINYCSOCKET_SUCCESS)
        return bind_status;

    int listen_status = tinycsocket_listen(socket_ctx);
    if (listen_status != TINYCSOCKET_SUCCESS)
        return listen_status;

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_send_netstring(TinyCSocketCtx* socket_ctx, const char* msg, const size_t bytes)
{
    if (socket_ctx == NULL || msg == NULL)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    char msg_length_as_char[21]; // 999 999 Peta bytes max size, for now
    sprintf(msg_length_as_char, "%lu", (unsigned long)bytes); // bad compiler support for %zu

    const uint8_t DIVIDER_CHAR = 0x3a;
    const uint8_t END_CHAR = 0x2c;

    int send_status = TINYCSOCKET_SUCCESS;

    send_status = tinycsocket_send_data(socket_ctx, msg_length_as_char, strlen(msg_length_as_char));
    if (send_status != TINYCSOCKET_SUCCESS)
        return send_status;

    send_status = tinycsocket_send_data(socket_ctx, &DIVIDER_CHAR, 1);
    if (send_status != TINYCSOCKET_SUCCESS)
        return send_status;

    send_status = tinycsocket_send_data(socket_ctx, msg, bytes);
    if (send_status != TINYCSOCKET_SUCCESS)
        return send_status;

    send_status = tinycsocket_send_data(socket_ctx, &END_CHAR, 1);
    if (send_status != TINYCSOCKET_SUCCESS)
        return send_status;

    return TINYCSOCKET_SUCCESS;
}

int tinycsocket_recieve_netstring(TinyCSocketCtx* socket_ctx,
                                  char* recieved_msg,
                                  const size_t buffer_byte_size,
                                  size_t* bytes_recieved)
{
    if (socket_ctx == NULL || recieved_msg == NULL || buffer_byte_size <= 0)
        return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

    size_t msg_length = 0;

    const uint8_t DIVIDER_CHAR = 0x3a;
    const uint8_t END_CHAR = 0x2c;

    char t = '\0';

    size_t mult = 1;
    const int max_prefix_length = 21;
    int i = 0;
    for (i = 0; i < max_prefix_length; ++i)
    {
        int recv_status = tinycsocket_recieve_data(socket_ctx, &t, 1, NULL);
        if (recv_status != TINYCSOCKET_SUCCESS)
            return recv_status;

        if (t == DIVIDER_CHAR)
            break;

        if (!isdigit(t))
            return TINYCSOCKET_ERROR_ILL_FORMED_MESSAGE;

        int number = t - '0';
        msg_length += number * mult;
        mult *= 10;
    }
    if (i == max_prefix_length)
        return TINYCSOCKET_ERROR_ILL_FORMED_MESSAGE;

    if (msg_length > buffer_byte_size)
        return TINYCSOCKET_ERROR_MEMORY;

    int recv_status =
        tinycsocket_recieve_data(socket_ctx, recieved_msg, msg_length, bytes_recieved);
    if (recv_status != TINYCSOCKET_SUCCESS)
        return recv_status;

    char end_char = '\0';
    recv_status = tinycsocket_recieve_data(socket_ctx, &end_char, 1, NULL);
    if (recv_status != TINYCSOCKET_SUCCESS)
        return recv_status;

    if (end_char != END_CHAR)
        return TINYCSOCKET_ERROR_ILL_FORMED_MESSAGE;

    return TINYCSOCKET_SUCCESS;
}
