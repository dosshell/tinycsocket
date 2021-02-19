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

#include <tinycsocket.h>

#include <stdbool.h>
#include <stdio.h>

static int show_error(const char* error_text)
{
    fprintf(stderr, "%s", error_text);
    return -1;
}

int main(void)
{
    if (tcs_lib_init() != TCS_SUCCESS)
        return show_error("Could not init tinycsocket");

    TcsSocket socket = TCS_NULLSOCKET;
    if (tcs_create(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) != TCS_SUCCESS)
        return show_error("Could not create socket");

    struct TcsAddress address[32];
    size_t found_addresses = 0;

    if (tcs_get_addresses("localhost", "1212", TCS_AF_IP4, address, 32, &found_addresses) != TCS_SUCCESS)
        return show_error("Could not resolve listen address");

    bool is_bounded = false;
    for (size_t i = 0; i < found_addresses; ++i)
    {
        if (tcs_bind(socket, &address[i]) == TCS_SUCCESS)
        {
            is_bounded = true;
            break;
        }
        tcs_destroy(&socket);
    }

    if (!is_bounded)
        return show_error("Could not bind socket");

    struct TcsAddress remote_address = {0};
    uint8_t recv_buffer[1024];
    size_t bytes_received = 0;
    if (tcs_receive_from(socket, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, &remote_address, &bytes_received) !=
        TCS_SUCCESS)
        return show_error("Could not receive data");

    // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in receive
    recv_buffer[bytes_received] = '\0';
    printf("received: %s\n", recv_buffer);

    char msg[] = "I here you loud and clear\n";
    if (tcs_send_to(socket, (const uint8_t*)msg, sizeof(msg), 0, &remote_address, NULL) != TCS_SUCCESS)
        return show_error("Could not send message");

    if (tcs_destroy(&socket) != TCS_SUCCESS)
        return show_error("Could not close socket");

    if (tcs_lib_free() != TCS_SUCCESS)
        return show_error("Could not free tinycsocket");
}
