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
#include <iostream>
#include <thread>

namespace
{
std::ostream& operator<<(std::ostream& o, const TcsAddress& address)
{
    if (address.family == TCS_AF_IP4)
    {
        uint32_t ipv4 = address.data.af_inet.address;
        int b1 = ipv4 & 0xFF;
        int b2 = (ipv4 >> 8) & 0xFF;
        int b3 = (ipv4 >> 16) & 0xFF;
        int b4 = (ipv4 >> 24) & 0xFF;
        return o << b1 << "." << b2 << "." << b3 << "." << b4;
    }
    else
    {
        return o << "?";
    }
}
} // namespace

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

    CHECK(tcs_create(&recv_soc, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    TcsAddress found_addresses[32];
    size_t no_of_found_addresses;
    size_t address_info_used = 0;

    CHECK(tcs_get_addresses("localhost", "1212", TCS_AF_IP4, found_addresses, 32, &no_of_found_addresses) ==
          TCS_SUCCESS);

    bool didBind = false;

    for (size_t i = 0; i < no_of_found_addresses; ++i)
    {
        if (tcs_bind(recv_soc, &found_addresses[i]) != TCS_SUCCESS)
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
    CHECK(tcs_create(&send_socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    // Send message
    uint8_t msg[] = "hello world\n";
    size_t sent;
    CHECK(tcs_sendto(send_socket, msg, sizeof(msg), 0, &found_addresses[address_info_used], &sent) == TCS_SUCCESS);
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

    CHECK(tcs_simple_create_and_listen(&listen_socket, "localhost", "1212", TCS_AF_IP4) == TCS_SUCCESS);
    CHECK(tcs_simple_create_and_connect(&client_socket, "localhost", "1212", TCS_AF_IP4) == TCS_SUCCESS);

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

    CHECK(tcs_simple_create_and_listen(&listen_socket, "localhost", "1212", TCS_AF_IP4) == TCS_SUCCESS);
    CHECK(tcs_simple_create_and_connect(&client_socket, "localhost", "1212", TCS_AF_IP4) == TCS_SUCCESS);

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
    // Given
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);
    size_t no_of_found_addresses = 0;

    // When
    CHECK(tcs_get_addresses("localhost", NULL, TCS_AF_IP4, NULL, 0, &no_of_found_addresses) == TCS_SUCCESS);

    // Then
    CHECK(no_of_found_addresses > 0);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Get number of local addresses")
{
    // Given
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);
    size_t peek_no_of_found_addresses = 0;
    size_t no_of_found_addresses = 0;
    struct TcsInterface interfaces[128];

    // When
    CHECK(tcs_get_interfaces(NULL, 0, &peek_no_of_found_addresses) == TCS_SUCCESS);
    CHECK(tcs_get_interfaces(interfaces, 128, &no_of_found_addresses) == TCS_SUCCESS);

    // Then
    CHECK(no_of_found_addresses > 0);
    CHECK(peek_no_of_found_addresses == no_of_found_addresses); // Fails if computer has more than 128 addresses

    // Clean up
    CHECK(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Get loopback address")
{
    // Given
    size_t no_of_found_addresses = 0;
    struct TcsInterface interfaces[32];
    bool found_loopback = false;
    uint32_t LOOPBACK_ADDRESS = tcs_util_ipv4_args(127, 0, 0, 1);
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // When
    CHECK(tcs_get_interfaces(interfaces, 32, &no_of_found_addresses) == TCS_SUCCESS);
    // find IPv4 loopback
    for (size_t i = 0; i < no_of_found_addresses; ++i)
    {
        if (interfaces[i].address.family == TCS_AF_IP4 &&
            interfaces[i].address.data.af_inet.address == LOOPBACK_ADDRESS)
        {
            found_loopback = true;
        }
    }

    // Inspect
    for (size_t i = 0; i < no_of_found_addresses; ++i)
    {
        std::cout << interfaces[i].name << " => " << interfaces[i].address << std::endl;
    }

    // Then
    CHECK(no_of_found_addresses > 0);
    CHECK(found_loopback);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS); // We are in C++, we should use defer
}

TEST_CASE("Example from README")
{
    CHECK(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket client_socket = TCS_NULLSOCKET;
    CHECK(tcs_simple_create_and_connect(&client_socket, "example.com", "80", TCS_AF_ANY) == TCS_SUCCESS);

    uint8_t send_buffer[] = "GET / HTTP/1.1\nHost: example.com\n\n";
    CHECK(tcs_simple_send_all(client_socket, send_buffer, sizeof(send_buffer), 0) == TCS_SUCCESS);

    uint8_t recv_buffer[8192] = {0};
    size_t bytes_received = 0;
    CHECK(tcs_recv(client_socket, recv_buffer, 8192, 0, &bytes_received) == TCS_SUCCESS);
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);

    CHECK(tcs_lib_free() == TCS_SUCCESS);
}
