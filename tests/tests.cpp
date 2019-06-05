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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <tinycsocket.h>
#include <cstring>
#include <thread>

TEST_CASE("Init Test")
{
    CHECK(tcs_lib_init() == TCS_SUCCESS);
    CHECK(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("UDP Test")
{
    CHECK(tcs_lib_init() == TCS_SUCCESS);

    struct tcs_addrinfo* address_info = NULL;

    struct tcs_addrinfo hints = {0};
    hints.ai_family = TCS_AF_INET;
    hints.ai_socktype = TCS_SOCK_DGRAM;

    tcs_getaddrinfo("localhost", "1212", &hints, &address_info);

    // Setup UDP receiver
    CHECK(tcs_lib_init() == TCS_SUCCESS);
    tcs_socket recv_soc = TCS_NULLSOCKET;

    bool didBind = false;
    for (struct tcs_addrinfo* address_iterator = address_info; address_iterator != NULL;
         address_iterator = address_iterator->ai_next)
    {
        if (tcs_create(
                &recv_soc, address_iterator->ai_family, address_iterator->ai_socktype, address_iterator->ai_protocol) !=
            TCS_SUCCESS)
            continue;

        if (tcs_bind(recv_soc, address_iterator->ai_addr, address_iterator->ai_addrlen) != TCS_SUCCESS)
            continue;

        didBind = true;
        break;
    }

    CHECK(didBind);

    struct tcs_sockaddr remote_address = {0};
    size_t remote_address_size = sizeof(remote_address);
    uint8_t recv_buffer[1024] = {0};
    size_t bytes_recieved = 0;

    std::thread recv_thread([&]() {
        CHECK(tcs_recvfrom(recv_soc,
                           recv_buffer,
                           sizeof(recv_buffer) - sizeof('\0'),
                           0,
                           &remote_address,
                           &remote_address_size,
                           &bytes_recieved) == TCS_SUCCESS);
        recv_buffer[bytes_recieved] = '\0';
    });

    // Setup UDP sender
    tcs_socket socket = TCS_NULLSOCKET;

    bool didConnect = false;
    for (struct tcs_addrinfo* address_iterator = address_info; address_iterator != NULL;
         address_iterator = address_iterator->ai_next)
    {
        if (tcs_create(
                &socket, address_iterator->ai_family, address_iterator->ai_socktype, address_iterator->ai_protocol) !=
            TCS_SUCCESS)
            continue;

        if (tcs_connect(socket, address_iterator->ai_addr, address_iterator->ai_addrlen) != TCS_SUCCESS)
            continue;

        didConnect = true;
        break;
    }

    CHECK(didConnect);

    // Send message
    uint8_t msg[] = "hello world\n";
    CHECK(tcs_send(socket, msg, sizeof(msg), 0, NULL) == TCS_SUCCESS);

    // Join threads and check if we did send the message correctly
    recv_thread.join();
    CHECK(strcmp(reinterpret_cast<const char*>(recv_buffer), reinterpret_cast<const char*>(msg)) == 0);
    CHECK(tcs_freeaddrinfo(&address_info) == TCS_SUCCESS);
    CHECK(tcs_close(&recv_soc) == TCS_SUCCESS);
    CHECK(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple TCP Test")
{
    CHECK(tcs_lib_init() == TCS_SUCCESS);

    tcs_socket listen_socket = TCS_NULLSOCKET;
    tcs_socket accept_socket = TCS_NULLSOCKET;
    tcs_socket client_socket = TCS_NULLSOCKET;

    CHECK(tcs_simple_create_and_listen(&listen_socket, "localhost", "1212", TCS_AF_INET) == TCS_SUCCESS);

    CHECK(tcs_create(&client_socket, TCS_AF_INET, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_simple_connect(client_socket, "localhost", "1212") == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    uint8_t recv_buffer[8] = {0};
    uint8_t* send_buffer = (uint8_t*)"12345678";

    CHECK(tcs_simple_send_all(client_socket, send_buffer, 8, 0) == TCS_SUCCESS);
    CHECK(tcs_simple_recv_all(accept_socket, recv_buffer, 8) == TCS_SUCCESS);

    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);

    tcs_lib_free();
}

TEST_CASE("Simple TCP Netstring Test")
{
    CHECK(tcs_lib_init() == TCS_SUCCESS);

    tcs_socket listen_socket = TCS_NULLSOCKET;
    tcs_socket accept_socket = TCS_NULLSOCKET;
    tcs_socket client_socket = TCS_NULLSOCKET;

    CHECK(tcs_simple_create_and_listen(&listen_socket, "localhost", "1212", TCS_AF_INET) == TCS_SUCCESS);

    CHECK(tcs_create(&client_socket, TCS_AF_INET, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_simple_connect(client_socket, "localhost", "1212") == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    uint8_t recv_buffer[16] = {0};
    uint8_t* send_buffer = (uint8_t*)"12345678";

    CHECK(tcs_simple_send_netstring(client_socket, send_buffer, 8) == TCS_SUCCESS);
    CHECK(tcs_simple_recv_netstring(accept_socket, recv_buffer, 16, NULL) == TCS_SUCCESS);

    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);

    tcs_lib_free();
}
