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

#include <stdio.h>
#include <stdlib.h>

static int show_error(const char* error_text)
{
    fprintf(stderr, "%s", error_text);
    return -1;
}

int main(void)
{
    if (tcs_lib_init() != TCS_SUCCESS)
        return show_error("Could not init tinycsocket");

    TcsSocket listen_socket = TCS_NULLSOCKET;
    TcsSocket child_socket = TCS_NULLSOCKET;

    struct TcsAddress listen_address;
    if (tcs_get_addresses(NULL, "1212", TCS_AF_IP4, &listen_address, 1, NULL) != TCS_SUCCESS)
        return show_error("Could not resolve listen address");

    if (tcs_create(&listen_socket, listen_address.family, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) != TCS_SUCCESS)
        return show_error("Could not create a listen socket");

    if (tcs_bind(listen_socket, &listen_address) != TCS_SUCCESS)
        return show_error("Could not bind to listen address");

    if (tcs_listen(listen_socket, TCS_BACKLOG_SOMAXCONN) != TCS_SUCCESS)
        return show_error("Could not listen");

    if (tcs_accept(listen_socket, &child_socket, NULL) != TCS_SUCCESS)
        return show_error("Could not accept socket");

    if (tcs_destroy(&listen_socket) != TCS_SUCCESS)
        return show_error("Could not close listen socket");

    uint8_t recv_buffer[1024];
    size_t bytes_received = 0;
    if (tcs_receive(child_socket, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, &bytes_received) != TCS_SUCCESS)
        return show_error("Could not receive data from client");

    recv_buffer[bytes_received] = '\0';
    printf("received: %s\n", recv_buffer);

    char msg[] = "I here you loud and clear\n";
    if (tcs_send(child_socket, (const uint8_t*)msg, sizeof(msg), 0, NULL) != TCS_SUCCESS)
        return show_error("Could not send reply message");

    if (tcs_shutdown(child_socket, TCS_SD_BOTH) != TCS_SUCCESS)
        return show_error("Could not shutdown socket");

    if (tcs_destroy(&child_socket) != TCS_SUCCESS)
        return show_error("Could not close socket");

    if (tcs_lib_free() != TCS_SUCCESS)
        return show_error("Could not free tinycsocket");
}
