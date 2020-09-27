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

    // Setup UDP receiver
    CHECK(tcs_lib_init() == TCS_SUCCESS);
    TcsSocket recv_soc = TCS_NULLSOCKET;

    TcsAddressInfo address_info[32];

    CHECK(tcs_create(&recv_soc, TCS_AF_INET, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    TcsAddressInfo hints = {};
    hints.family = TCS_AF_INET;
    hints.socktype = TCS_SOCK_DGRAM;
    hints.protocol = TCS_IPPROTO_UDP;
    size_t found_addresses;
    size_t address_info_used = 0;

    CHECK(tcs_getaddrinfo("localhost", "1212", &hints, address_info, 32, &found_addresses) == TCS_SUCCESS);

    bool didBind = false;

    for (size_t i = 0; i < found_addresses; ++i)
    {
        if (tcs_bind(recv_soc, &address_info[i].address) != TCS_SUCCESS)
            continue;

        address_info_used = i;
        didBind = true;
        break;
    }

    CHECK(didBind);

    uint8_t recv_buffer[1024] = {0};
    size_t bytes_received = 0;

    std::thread recv_thread([&]() {
        CHECK(tcs_recvfrom(recv_soc, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, NULL, &bytes_received) ==
              TCS_SUCCESS);
        recv_buffer[bytes_received] = '\0';
    });
    std::this_thread::yield();

    // Setup UDP sender
    TcsSocket send_socket = TCS_NULLSOCKET;
    CHECK(tcs_create(&send_socket, TCS_AF_INET, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    // Send message
    uint8_t msg[] = "hello world\n";
    size_t sent;
    CHECK(tcs_sendto(send_socket, msg, sizeof(msg), 0, &address_info[address_info_used].address, &sent) == TCS_SUCCESS);
    CHECK(sent > 0);
    // Join threads and check if we did send the message correctly
    recv_thread.join();
    CHECK(strcmp(reinterpret_cast<const char*>(recv_buffer), reinterpret_cast<const char*>(msg)) == 0);
    CHECK(tcs_close(&recv_soc) == TCS_SUCCESS);
    CHECK(tcs_close(&send_socket) == TCS_SUCCESS);
    CHECK(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple TCP Test")
{
    CHECK(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket listen_socket = TCS_NULLSOCKET;
    TcsSocket accept_socket = TCS_NULLSOCKET;
    TcsSocket client_socket = TCS_NULLSOCKET;

    CHECK(tcs_simple_create_and_listen(&listen_socket, "localhost", "1212", TCS_AF_INET) == TCS_SUCCESS);
    CHECK(tcs_simple_create_and_connect(&client_socket, "localhost", "1212", TCS_AF_INET) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    uint8_t recv_buffer[8] = {0};
    uint8_t* send_buffer = (uint8_t*)"12345678";

    CHECK(tcs_simple_send_all(client_socket, send_buffer, 8, 0) == TCS_SUCCESS);
    CHECK(tcs_simple_recv_all(accept_socket, recv_buffer, 8) == TCS_SUCCESS);

    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);

    CHECK(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple TCP Netstring Test")
{
    CHECK(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket listen_socket = TCS_NULLSOCKET;
    TcsSocket accept_socket = TCS_NULLSOCKET;
    TcsSocket client_socket = TCS_NULLSOCKET;

    CHECK(tcs_simple_create_and_listen(&listen_socket, "localhost", "1212", TCS_AF_INET) == TCS_SUCCESS);
    CHECK(tcs_simple_create_and_connect(&client_socket, "localhost", "1212", TCS_AF_INET) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    uint8_t recv_buffer[16] = {0};
    uint8_t* send_buffer = (uint8_t*)"12345678";

    CHECK(tcs_simple_send_netstring(client_socket, send_buffer, 8) == TCS_SUCCESS);
    CHECK(tcs_simple_recv_netstring(accept_socket, recv_buffer, 16, NULL) == TCS_SUCCESS);

    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);

    CHECK(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Address information count")
{
    CHECK(tcs_lib_init() == TCS_SUCCESS);

    // struct TcsAddressInfo address_info_list[32];
    size_t no_of_found_addresses = 0;
    tcs_getaddrinfo("localhost", NULL, NULL, NULL, 0, &no_of_found_addresses);
    CHECK(no_of_found_addresses > 0);

    CHECK(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Example from README")
{
    CHECK(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket client_socket = TCS_NULLSOCKET;
    CHECK(tcs_simple_create_and_connect(&client_socket, "example.com", "80", TCS_AF_UNSPEC) == TCS_SUCCESS);

    uint8_t send_buffer[] = "GET / HTTP/1.1\nHost: example.com\n\n";
    CHECK(tcs_simple_send_all(client_socket, send_buffer, sizeof(send_buffer), 0) == TCS_SUCCESS);

    uint8_t recv_buffer[8192] = {0};
    size_t bytes_received = 0;
    CHECK(tcs_recv(client_socket, recv_buffer, 8192, 0, &bytes_received) == TCS_SUCCESS);
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);

    CHECK(tcs_lib_free() == TCS_SUCCESS);
}
