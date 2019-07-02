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
#include <stdlib.h>
#include <string.h>

int show_error(const char* error_text);

int show_error(const char* error_text)
{
    fprintf(stderr, "%s", error_text);
    return -1;
}

int main(void)
{
    if (tcs_lib_init() != TCS_SUCCESS)
        return show_error("Could not init tinycsocket");

    tcs_socket client_socket = TCS_NULLSOCKET;

    if (tcs_create(&client_socket, TCS_AF_INET, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) != TCS_SUCCESS)
        return show_error("Could not create a socket");

    struct tcs_addrinfo* address_info = NULL;
    if (tcs_getaddrinfo("localhost", "1212", NULL, &address_info) != TCS_SUCCESS)
        return show_error("Could not resolve host");

    bool is_connected = false;
    for (struct tcs_addrinfo* address_iterator = address_info; address_iterator != NULL;
         address_iterator = address_iterator->ai_next)
    {
        if (tcs_connect(client_socket, address_iterator->ai_addr, address_iterator->ai_addrlen) == TCS_SUCCESS)
        {
            is_connected = true;
            break;
        }
    }

    tcs_freeaddrinfo(&address_info);

    if (!is_connected)
        return show_error("Could not connect to server");

    char msg[] = "hello world\n";
    tcs_send(client_socket, (const uint8_t*)msg, sizeof(msg), 0, NULL);

    uint8_t recv_buffer[1024];
    size_t bytes_recieved = 0;
    if (tcs_recv(client_socket, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, &bytes_recieved) != TCS_SUCCESS)
        return show_error("Could not recieve data");

    // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in recieve
    recv_buffer[bytes_recieved] = '\0';
    printf("recieved: %s\n", recv_buffer);

    if (tcs_shutdown(client_socket, TCS_SD_BOTH) != TCS_SUCCESS)
        return show_error("Could not shutdown socket");

    if (tcs_close(&client_socket) != TCS_SUCCESS)
        return show_error("Could not close the socket");

    if (tcs_lib_free() != TCS_SUCCESS)
        return show_error("Could not free tinycsocket");
}
