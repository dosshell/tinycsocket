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

namespace
{
std::ostream& operator<<(std::ostream& o, const TcsAddress& address)
{
    char c[49];
    TcsReturnCode sts = tcs_util_address_to_string(&address, c);
    if (sts == TCS_SUCCESS)
        return o << c;
    else
        return o << "?";
}
} // namespace

TEST_CASE("Example from README")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket client_socket = TCS_NULLSOCKET;
    CHECK(tcs_simple_create_and_connect(&client_socket, "example.com", "80", TCS_AF_ANY) == TCS_SUCCESS);

    uint8_t send_buffer[] = "GET / HTTP/1.1\nHost: example.com\n\n";
    CHECK(tcs_simple_send_all(client_socket, send_buffer, sizeof(send_buffer), 0) == TCS_SUCCESS);

    uint8_t recv_buffer[8192] = {0};
    size_t bytes_received = 0;
    CHECK(tcs_receive(client_socket, recv_buffer, 8192, 0, &bytes_received) == TCS_SUCCESS);
    CHECK(tcs_destroy(&client_socket) == TCS_SUCCESS);

    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Init Test")
{
    CHECK(tcs_lib_init() == TCS_SUCCESS);
    CHECK(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("UDP Test")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Gíven
    TcsSocket socket_recv = TCS_NULLSOCKET;
    TcsSocket socket_send = TCS_NULLSOCKET;
    CHECK(tcs_create(&socket_recv, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_send, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    CHECK(tcs_set_receive_timeout(socket_recv, 5000) == TCS_SUCCESS);
    CHECK(tcs_set_reuse_address(socket_recv, true) == TCS_SUCCESS);
    TcsAddress found_addresses[32];
    size_t no_of_found_addresses;
    size_t address_info_used = 0;

    CHECK(tcs_get_addresses("localhost", "1212", TCS_AF_IP4, found_addresses, 32, &no_of_found_addresses) ==
          TCS_SUCCESS);

    bool didBind = false;

    for (size_t i = 0; i < no_of_found_addresses; ++i)
    {
        if (tcs_bind(socket_recv, &found_addresses[i]) != TCS_SUCCESS)
            continue;

        address_info_used = i;
        didBind = true;
        break;
    }

    CHECK(didBind);
    uint8_t msg[] = "hello world\n";
    size_t sent = 0;
    uint8_t recv_buffer[1024] = {0};
    size_t bytes_received = 0;

    // When
    CHECK(tcs_send_to(socket_send, msg, sizeof(msg), 0, &found_addresses[address_info_used], &sent) == TCS_SUCCESS);
    CHECK(tcs_receive_from(socket_recv, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, NULL, &bytes_received) ==
          TCS_SUCCESS);
    recv_buffer[bytes_received] = '\0';

    // Then
    CHECK(sent > 0);
    CHECK(strcmp(reinterpret_cast<const char*>(recv_buffer), reinterpret_cast<const char*>(msg)) == 0);

    // Clean up
    CHECK(tcs_destroy(&socket_recv) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_send) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple TCP Test")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_NULLSOCKET;
    TcsSocket accept_socket = TCS_NULLSOCKET;
    TcsSocket client_socket = TCS_NULLSOCKET;

    CHECK(tcs_simple_create_and_listen(&listen_socket, "localhost", "1212", TCS_AF_IP4) == TCS_SUCCESS);
    CHECK(tcs_simple_create_and_connect(&client_socket, "localhost", "1212", TCS_AF_IP4) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_destroy(&listen_socket) == TCS_SUCCESS);

    // When
    uint8_t recv_buffer[8] = {0};
    uint8_t* send_buffer = (uint8_t*)"12345678";
    CHECK(tcs_simple_send_all(client_socket, send_buffer, 8, 0) == TCS_SUCCESS);
    CHECK(tcs_simple_recv_all(accept_socket, recv_buffer, 8) == TCS_SUCCESS);

    // Then
    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    // Clean up
    CHECK(tcs_destroy(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_destroy(&accept_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple TCP Netstring Test")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_NULLSOCKET;
    TcsSocket accept_socket = TCS_NULLSOCKET;
    TcsSocket client_socket = TCS_NULLSOCKET;
    CHECK(tcs_simple_create_and_listen(&listen_socket, "localhost", "1212", TCS_AF_IP4) == TCS_SUCCESS);
    CHECK(tcs_simple_create_and_connect(&client_socket, "localhost", "1212", TCS_AF_IP4) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_destroy(&listen_socket) == TCS_SUCCESS);

    // When
    uint8_t recv_buffer[16] = {0};
    uint8_t* send_buffer = (uint8_t*)"12345678";
    CHECK(tcs_simple_send_netstring(client_socket, send_buffer, 8) == TCS_SUCCESS);
    CHECK(tcs_simple_recv_netstring(accept_socket, recv_buffer, 16, NULL) == TCS_SUCCESS);

    // Then
    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    // Clean up
    CHECK(tcs_destroy(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_destroy(&accept_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
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
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
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
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Get loopback address")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

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

    // Debug
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

TEST_CASE("tcs_util_address_to_string with only IPv4")
{
    // Given
    TcsAddress addr;
    addr.family = TCS_AF_IP4;
    addr.data.af_inet.port = 0;
    addr.data.af_inet.address = tcs_util_ipv4_args(127, 0, 0, 1);

    // When
    char address_str[40];
    tcs_util_address_to_string(&addr, address_str);

    // Then
    CHECK_EQ(address_str, "127.0.0.1");
}

TEST_CASE("tcs_util_address_to_string with IPv4 and port")
{
    // Given
    TcsAddress addr;
    addr.family = TCS_AF_IP4;
    addr.data.af_inet.port = 1234;
    addr.data.af_inet.address = tcs_util_ipv4_args(127, 0, 0, 1);

    // When
    char address_str[40];
    tcs_util_address_to_string(&addr, address_str);

    // Then
    CHECK_EQ(address_str, "127.0.0.1:1234");
}

TEST_CASE("tcs_util_string_to_address with only IPv4")
{
    // Given
    const char str[] = "127.0.0.1";

    // When
    TcsAddress address;
    tcs_util_string_to_address(str, &address);

    // Then
    CHECK(address.family == TCS_AF_IP4);
    CHECK(address.data.af_inet.port == 0);
    CHECK(address.data.af_inet.address == tcs_util_ipv4_args(127, 0, 0, 1));
    CHECK(tcs_util_string_to_address("127.256.0.1", &address) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_util_string_to_address("-1.0.0.1", &address) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_util_string_to_address("0xFF.01.1.0xFF", &address) == TCS_SUCCESS);
    CHECK(tcs_util_string_to_address("0xFF.01.1", &address) == TCS_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("tcs_util_string_to_address with IPv4and port")
{
    // Given
    const char str[] = "127.0.0.1:1234";

    // When
    TcsAddress address;
    tcs_util_string_to_address(str, &address);

    // Then
    CHECK(address.family == TCS_AF_IP4);
    CHECK(address.data.af_inet.port == 1234);
    CHECK(address.data.af_inet.address == tcs_util_ipv4_args(127, 0, 0, 1));
    CHECK(tcs_util_string_to_address("127.255.0.1:65536", &address) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_util_string_to_address("1.0.0.1:-1", &address) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_util_string_to_address("0xFF.01.1.0xFF:0xFFFF", &address) == TCS_SUCCESS);
    CHECK(tcs_util_string_to_address("0xFF.01.1:123", &address) == TCS_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("TCS_SO_BROADCAST")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_false = TCS_NULLSOCKET;
    TcsSocket socket_true = TCS_NULLSOCKET;
    TcsSocket socket_fail = TCS_NULLSOCKET;
    TcsSocket socket_null = TCS_NULLSOCKET;
    CHECK(tcs_create(&socket_false, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_true, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_fail, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);

    // When
    CHECK(tcs_set_broadcast(socket_false, false) == TCS_SUCCESS);
    CHECK(tcs_set_broadcast(socket_true, true) == TCS_SUCCESS);
#ifdef CROSS_ISSUES
    CHECK(tcs_set_broadcast(socket_fail, true) != TCS_SUCCESS); // TODO(markusl): Not a dgram socket, success on linux
#endif

    // Then
    bool socket_false_value;
    bool socket_true_value;
#ifdef CROSS_ISSUES
    bool socket_fail_value;
#endif
    bool socket_null_value;
    CHECK(tcs_get_broadcast(socket_false, &socket_false_value) == TCS_SUCCESS);
    CHECK(tcs_get_broadcast(socket_true, &socket_true_value) == TCS_SUCCESS);
#ifdef CROSS_ISSUES
    CHECK(tcs_get_broadcast(socket_fail, &socket_fail_value) !=
          TCS_SUCCESS); // TODO(markusl): Not a dgram socket, success on linux
#endif
    CHECK(tcs_get_broadcast(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_false_value == false);
    CHECK(socket_true_value == true);

    // Clean up
    CHECK(tcs_destroy(&socket_false) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_true) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_fail) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_KEEPALIVE")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_false = TCS_NULLSOCKET;
    TcsSocket socket_true = TCS_NULLSOCKET;
    TcsSocket socket_fail = TCS_NULLSOCKET;
    TcsSocket socket_null = TCS_NULLSOCKET;

    CHECK(tcs_create(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_set_keep_alive(socket_false, false) == TCS_SUCCESS);
    CHECK(tcs_set_keep_alive(socket_true, true) == TCS_SUCCESS);

    // Then
    bool socket_false_value;
    bool socket_true_value;
#ifdef CROSS_ISSUE
    bool socket_fail_value;
#endif
    bool socket_null_value;
    CHECK(tcs_get_keep_alive(socket_false, &socket_false_value) == TCS_SUCCESS);
    CHECK(tcs_get_keep_alive(socket_true, &socket_true_value) == TCS_SUCCESS);
#ifdef CROSS_ISSUES
    CHECK(tcs_get_keep_alive(socket_fail, &socket_fail_value) != TCS_SUCCESS); // Not a stream socket
#endif
    CHECK(tcs_get_keep_alive(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_false_value == false);
    CHECK(socket_true_value == true);

    // Clean up
    CHECK(tcs_destroy(&socket_false) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_true) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_fail) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_LINGER")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_false = TCS_NULLSOCKET;
    TcsSocket socket_true = TCS_NULLSOCKET;
    TcsSocket socket_fail = TCS_NULLSOCKET;
    TcsSocket socket_null = TCS_NULLSOCKET;

    CHECK(tcs_create(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_set_linger(socket_false, false, 3) == TCS_SUCCESS);
    CHECK(tcs_set_linger(socket_true, true, 2) == TCS_SUCCESS);

    // Then
    bool socket_false_value;
    int socket_false_time;
    bool socket_true_value;
    int socket_true_time;
#ifdef CROSS_ISSUES
    bool socket_fail_value;
    int socket_fail_time;
#endif
    bool socket_null_value;
    CHECK(tcs_get_linger(socket_false, &socket_false_value, &socket_false_time) == TCS_SUCCESS);
    CHECK(tcs_get_linger(socket_true, &socket_true_value, &socket_true_time) == TCS_SUCCESS);
#ifdef CROSS_ISSUES
    CHECK(tcs_get_linger(socket_fail, &socket_fail_value, &socket_fail_time) != TCS_SUCCESS); // Not a stream socket
#endif
    CHECK(tcs_get_linger(socket_null, &socket_null_value, NULL) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_false_value == false);
    CHECK(socket_true_value == true);

    // Clean up
    CHECK(tcs_destroy(&socket_false) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_true) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_fail) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_REUSEADDR")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_false = TCS_NULLSOCKET;
    TcsSocket socket_true = TCS_NULLSOCKET;
    TcsSocket socket_fail = TCS_NULLSOCKET;
    TcsSocket socket_null = TCS_NULLSOCKET;

    CHECK(tcs_create(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_set_reuse_address(socket_false, false) == TCS_SUCCESS);
    CHECK(tcs_set_reuse_address(socket_true, true) == TCS_SUCCESS);
    CHECK(tcs_set_reuse_address(socket_fail, true) == TCS_SUCCESS);

    // Then
    bool socket_false_value;
    bool socket_true_value;
    bool socket_dgram_value;
    bool socket_null_value;
    CHECK(tcs_get_reuse_address(socket_false, &socket_false_value) == TCS_SUCCESS);
    CHECK(tcs_get_reuse_address(socket_true, &socket_true_value) == TCS_SUCCESS);
    CHECK(tcs_get_reuse_address(socket_fail, &socket_dgram_value) == TCS_SUCCESS);
    CHECK(tcs_get_reuse_address(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_false_value == false);
    CHECK(socket_true_value == true);
    CHECK(socket_dgram_value == true);

    // Clean up
    CHECK(tcs_destroy(&socket_false) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_true) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_fail) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_RCVBUF")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_NULLSOCKET;
    TcsSocket socket_null = TCS_NULLSOCKET;

    CHECK(tcs_create(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_set_receive_buffer_size(socket, 8192) == TCS_SUCCESS);

    // Then
    size_t socket_rcvbuf_value;
    size_t socket_null_value;
    CHECK(tcs_get_receive_buffer_size(socket, &socket_rcvbuf_value) == TCS_SUCCESS);
    CHECK(tcs_get_receive_buffer_size(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_rcvbuf_value == 8192);

    // Clean up
    CHECK(tcs_destroy(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_SNDBUF")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_NULLSOCKET;
    TcsSocket socket_null = TCS_NULLSOCKET;

    CHECK(tcs_create(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_set_send_buffer_size(socket, 8192) == TCS_SUCCESS);

    // Then
    size_t socket_rcvbuf_value;
    size_t socket_null_value;
    CHECK(tcs_get_send_buffer_size(socket, &socket_rcvbuf_value) == TCS_SUCCESS);
    CHECK(tcs_get_send_buffer_size(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_rcvbuf_value == 8192);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_RCVTIMEO")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_NULLSOCKET;
    TcsSocket socket_null = TCS_NULLSOCKET;

    CHECK(tcs_create(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_set_receive_timeout(socket, 920) == TCS_SUCCESS);

    // Then
    int socket_receive_timeout_value;
    int socket_null_value;
    CHECK(tcs_get_receive_timeout(socket, &socket_receive_timeout_value) == TCS_SUCCESS);
    CHECK(tcs_get_receive_timeout(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    // TODO(markusl): 918 -> 920 for WSL1 linux... make some comment or make it coherent
    CHECK(socket_receive_timeout_value == 920);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_OOBINLINE")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_false = TCS_NULLSOCKET;
    TcsSocket socket_true = TCS_NULLSOCKET;
    TcsSocket socket_fail = TCS_NULLSOCKET;
    TcsSocket socket_null = TCS_NULLSOCKET;

    CHECK(tcs_create(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_set_out_of_band_inline(socket_false, false) == TCS_SUCCESS);
    CHECK(tcs_set_out_of_band_inline(socket_true, true) == TCS_SUCCESS);

    // Then
    bool socket_false_value;
    bool socket_true_value;
#ifdef CROSS_ISSUES
    bool socket_fail_value;
#endif
    bool socket_null_value;
    CHECK(tcs_get_out_of_band_inline(socket_false, &socket_false_value) == TCS_SUCCESS);
    CHECK(tcs_get_out_of_band_inline(socket_true, &socket_true_value) == TCS_SUCCESS);
#ifdef CROSS_ISSUES
    CHECK(tcs_get_out_of_band_inline(socket_fail, &socket_fail_value) != TCS_SUCCESS); // Not a stream socket
#endif
    CHECK(tcs_get_out_of_band_inline(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_false_value == false);
    CHECK(socket_true_value == true);

    // Clean up
    CHECK(tcs_destroy(&socket_false) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_true) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_fail) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple Multicast Add Membership")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsAddress address_any = {TCS_AF_IP4, {0, 0}};
    address_any.data.af_inet.port = 1901;

    TcsAddress multicast_address = {TCS_AF_IP4, {0, 0}};
    multicast_address.data.af_inet.address = tcs_util_ipv4_args(239, 255, 255, 251);
    multicast_address.data.af_inet.port = 1901;

    TcsSocket socket = TCS_NULLSOCKET;
    CHECK(tcs_create(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, 0) == TCS_SUCCESS);
    CHECK(tcs_set_reuse_address(socket, true) == TCS_SUCCESS);
    CHECK(tcs_set_receive_timeout(socket, 5000) == TCS_SUCCESS);

    CHECK(tcs_bind(socket, &address_any) == TCS_SUCCESS);
    uint8_t msg[] = "hello world\n";

    // When
    CHECK(tcs_set_ip_multicast_add(socket, &address_any, &multicast_address) == TCS_SUCCESS);

    uint8_t c;
    size_t a = sizeof(c);
    CHECK(tcs_getsockopt(socket, TCS_SOL_IP, TCS_SO_IP_MULTICAST_LOOP, &c, &a) == TCS_SUCCESS);

    CHECK(tcs_send_to(socket, msg, sizeof(msg), 0, &multicast_address, NULL) == TCS_SUCCESS);

    // Then
    uint8_t recv_buffer[1024] = {0};
    size_t bytes_received = 0;
    CHECK(tcs_receive(socket, recv_buffer, sizeof(recv_buffer), 0, &bytes_received) == TCS_SUCCESS);
    CHECK(strcmp(reinterpret_cast<const char*>(recv_buffer), reinterpret_cast<const char*>(msg)) == 0);

    // Clean up
    CHECK(tcs_destroy(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Multicast Add-Drop-Add Membership")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_send = TCS_NULLSOCKET;
    TcsSocket socket_recv = TCS_NULLSOCKET;
    CHECK(tcs_create(&socket_send, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    CHECK(tcs_create(&socket_recv, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    tcs_set_receive_timeout(socket_recv, 1000);
    CHECK(tcs_set_reuse_address(socket_recv, true) == TCS_SUCCESS);

    TcsAddress address_any = {TCS_AF_IP4, {0, 0}};
    address_any.data.af_inet.port = 1901;

    TcsAddress multicast_address = {TCS_AF_IP4, {0, 0}};
    multicast_address.data.af_inet.address = tcs_util_ipv4_args(239, 255, 255, 251);
    multicast_address.data.af_inet.port = 1901;

    CHECK(tcs_bind(socket_recv, &address_any) == TCS_SUCCESS);
    uint8_t msg_1[] = "hello world1\n";
    uint8_t msg_2[] = "hello world2\n";
    uint8_t msg_missed[] = "you can not read me\n";

    // When
    CHECK(tcs_set_ip_multicast_add(socket_recv, &address_any, &multicast_address) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket_send, msg_1, sizeof(msg_1), 0, &multicast_address, NULL) == TCS_SUCCESS);
    CHECK(tcs_set_ip_multicast_drop(socket_recv, &address_any, &multicast_address) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket_send, msg_missed, sizeof(msg_missed), 0, &multicast_address, NULL) == TCS_SUCCESS);
    CHECK(tcs_set_ip_multicast_add(socket_recv, &address_any, &multicast_address) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket_send, msg_2, sizeof(msg_2), 0, &multicast_address, NULL) == TCS_SUCCESS);

    // Then
    uint8_t recv_buffer_1[1024] = {0};
    uint8_t recv_buffer_2[1024] = {0};
    size_t bytes_received_1;
    size_t bytes_received_2;

    CHECK(tcs_receive(socket_recv, recv_buffer_1, sizeof(recv_buffer_1), 0, &bytes_received_1) == TCS_SUCCESS);
    CHECK(tcs_receive(socket_recv, recv_buffer_2, sizeof(recv_buffer_2), 0, &bytes_received_2) == TCS_SUCCESS);
    recv_buffer_1[bytes_received_1] = '\0';
    recv_buffer_2[bytes_received_2] = '\0';

    CHECK(strcmp(reinterpret_cast<const char*>(recv_buffer_1), reinterpret_cast<const char*>(msg_1)) == 0);
    CHECK(strcmp(reinterpret_cast<const char*>(recv_buffer_2), reinterpret_cast<const char*>(msg_2)) == 0);

    // Clean up
    CHECK(tcs_destroy(&socket_recv) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_send) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple create socket")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_NULLSOCKET;

    // When
    TcsReturnCode sts = tcs_simple_create(&socket, TCS_ST_UDP_IP4);

    // Then
    CHECK(sts == TCS_SUCCESS);
    CHECK(socket != TCS_NULLSOCKET);

    // Clean up
    CHECK(tcs_destroy(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);

}