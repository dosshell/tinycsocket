﻿/*
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
#include <stdbool.h>

int show_error(const char* error_text)
{
    fprintf(stderr, "%s", error_text);
    return -1;
}

int main(int argc, const char* argv[])
{
    if (tcs_lib_init() != TINYCSOCKET_SUCCESS)
        return show_error("Could not init tinycsockets");
    
    tcs_socket socket = TINYCSOCKET_NULLSOCKET;

    struct tcs_addrinfo* remote_info = NULL;

    struct tcs_addrinfo hints = { 0 };
    hints.ai_family = TINYCSOCKET_AF_INET;
    hints.ai_socktype = TINYCSOCKET_SOCK_DGRAM;

    tcs_getaddrinfo("localhost", "1212", &hints, &remote_info);

    bool didConnect = false;
    for (struct tcs_addrinfo* address_iterator = remote_info; address_iterator != NULL; address_iterator = address_iterator->ai_next)
    {
        if (tcs_create(&socket, address_iterator->ai_family, address_iterator->ai_socktype, address_iterator->ai_protocol) != TINYCSOCKET_SUCCESS)
            continue;

        if (tcs_connect(socket, address_iterator->ai_addr, address_iterator->ai_addrlen) != TINYCSOCKET_SUCCESS)
            continue;

        didConnect = true;
        break;
    }

    if (!didConnect)
        return show_error("Could not connect to server");

    char msg[] = "hello world\n";
    if (tcs_send(socket, msg, sizeof(msg), 0, NULL) != TINYCSOCKET_SUCCESS)
        return show_error("Could not send message");

    uint8_t recv_buffer[1024];
    size_t bytes_recieved = 0;
    if (tcs_recv(socket, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, &bytes_recieved) != TINYCSOCKET_SUCCESS)
        return show_error("Could not recieve data");

    // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in recieve
    recv_buffer[bytes_recieved] = '\0';
    printf("recieved: %s\n", recv_buffer);

    if (tcs_close(&socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not close socket");

    if (tcs_lib_free() != TINYCSOCKET_SUCCESS)
        return show_error("Could not free tinycsockets");
}
