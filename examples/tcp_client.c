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

#define TINYCSOCKET_IMPLEMENTATION
#include <tinycsocket.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int show_error(const char* error_text)
{
    fprintf(stderr, "%s", error_text);
    return -1;
}

int main(void)
{
    if (tcs_lib_init() != TCS_SUCCESS)
        return show_error("Could not init tinycsocket");

    TcsSocket client_socket = TCS_NULLSOCKET;
    if (tcs_create(&client_socket, TCS_TYPE_TCP_IP4) != TCS_SUCCESS)
        return show_error("Could not create a socket");

    if (tcs_connect(client_socket, "lcoalhost", 1212) != TCS_SUCCESS)
        return show_error("Could not connect to server");

    char msg[] = "hello world\n";
    if (tcs_send(client_socket, (const uint8_t*)msg, sizeof(msg), TCS_MSG_SENDALL, NULL) != TCS_SUCCESS)
        return show_error("Could not send message");

    uint8_t recv_buffer[1024];
    size_t recv_size = sizeof(recv_buffer) - sizeof('\0');
    size_t bytes_received = 0;
    if (tcs_receive(client_socket, recv_buffer, recv_size, TCS_NO_FLAGS, &bytes_received) != TCS_SUCCESS)
        return show_error("Could not receive data");

    // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in receive
    recv_buffer[bytes_received] = '\0';
    printf("received: %s\n", recv_buffer);

    if (tcs_shutdown(client_socket, TCS_SD_BOTH) != TCS_SUCCESS)
        return show_error("Could not shutdown socket");

    if (tcs_destroy(&client_socket) != TCS_SUCCESS)
        return show_error("Could not close the socket");

    if (tcs_lib_free() != TCS_SUCCESS)
        return show_error("Could not free tinycsocket");
}
