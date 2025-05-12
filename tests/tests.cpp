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

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#ifdef DO_WRAP
#include "dbg_wrap.h"
#endif
#include "mock.h"

#include <tinycsocket.h>
#include <cstring>
#include <iostream>
#include <thread>

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

int main(int argc, char** argv)
{
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    doctest::Context context;

    context.applyCommandLine(argc, argv);

    int res = context.run(); // run

    if (context.shouldExit()) // important - query flags (and --exit) rely on the user doing this
        return res;           // propagate the result of the tests

    return res; // the result from doctest is propagated here as well
}

TEST_CASE("Check mock")
{
    int pre_alloc = MOCK_ALLOC_COUNTER;
    int pre_free = MOCK_FREE_COUNTER;
    void* a = malloc(8);
    free(a);
    CHECK(MOCK_ALLOC_COUNTER > pre_alloc);
    CHECK(MOCK_FREE_COUNTER > pre_free);
}

TEST_CASE("Example from README")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket client_socket = TCS_NULLSOCKET;
    CHECK(tcs_create(&client_socket, TCS_TYPE_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_connect(client_socket, "example.com", 80) == TCS_SUCCESS);

    uint8_t send_buffer[] = "GET / HTTP/1.1\nHost: example.com\n\n";
    CHECK(tcs_send(client_socket, send_buffer, sizeof(send_buffer), TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);

    uint8_t recv_buffer[8192] = {0};
    size_t bytes_received = 0;
    CHECK(tcs_receive(client_socket, recv_buffer, 8192, TCS_NO_FLAGS, &bytes_received) == TCS_SUCCESS);
    CHECK(tcs_shutdown(client_socket, TCS_SD_BOTH) == TCS_SUCCESS);
    CHECK(tcs_destroy(&client_socket) == TCS_SUCCESS);

    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Init Test")
{
    // Given
    int pre_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // When
    CHECK(tcs_lib_init() == TCS_SUCCESS);
    CHECK(tcs_lib_free() == TCS_SUCCESS);
    int post_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // Then
    CHECK(post_mem_diff == pre_mem_diff);
}

TEST_CASE("Create socket")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_NULLSOCKET;
    int pre_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // When
    TcsReturnCode sts = tcs_create(&socket, TCS_TYPE_UDP_IP4);
    int post_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // Then
    CHECK(sts == TCS_SUCCESS);
    CHECK(socket != TCS_NULLSOCKET);
    CHECK(post_mem_diff == pre_mem_diff);

    // Clean up
    CHECK(tcs_destroy(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("UDP Test")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Gíven
    TcsSocket socket_recv = TCS_NULLSOCKET;
    TcsSocket socket_send = TCS_NULLSOCKET;
    CHECK(tcs_create_ext(&socket_recv, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_send, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    CHECK(tcs_set_receive_timeout(socket_recv, 5000) == TCS_SUCCESS);
    CHECK(tcs_set_reuse_address(socket_recv, true) == TCS_SUCCESS);

    tcs_bind(socket_recv, 1432);

    uint8_t msg[] = "hello world\n";
    size_t sent = 0;
    uint8_t recv_buffer[1024] = {0};
    size_t recv_size = sizeof(recv_buffer) - sizeof('\0');
    size_t bytes_received = 0;

    TcsAddress address;
    address.family = TCS_AF_IP4;
    address.data.af_inet.address = TCS_ADDRESS_LOOPBACK_IP4;
    address.data.af_inet.port = 1432;

    // When
    CHECK(tcs_send_to(socket_send, msg, sizeof(msg), TCS_NO_FLAGS, &address, &sent) == TCS_SUCCESS);
    CHECK(tcs_receive(socket_recv, recv_buffer, recv_size, TCS_NO_FLAGS, &bytes_received) == TCS_SUCCESS);
    recv_buffer[bytes_received] = '\0';

    // Then
    CHECK(sent > 0);
    CHECK(strcmp((const char*)recv_buffer, (const char*)msg) == 0);

    // Clean up
    CHECK(tcs_destroy(&socket_recv) == TCS_SUCCESS);
    CHECK(tcs_destroy(&socket_send) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Bind UDP")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_NULLSOCKET;
    CHECK(tcs_create(&socket, TCS_TYPE_UDP_IP4) == TCS_SUCCESS);

    // When
    TcsReturnCode sts = tcs_bind(socket, 1465);

    // Then
    CHECK(sts == TCS_SUCCESS);

    // Clean up
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

    CHECK(tcs_create(&listen_socket, TCS_TYPE_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_create(&client_socket, TCS_TYPE_TCP_IP4) == TCS_SUCCESS);

    CHECK(tcs_set_reuse_address(listen_socket, true) == TCS_SUCCESS);
    CHECK(tcs_listen_to(listen_socket, 1212) == TCS_SUCCESS);
    CHECK(tcs_connect(client_socket, "localhost", 1212) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_destroy(&listen_socket) == TCS_SUCCESS);

    // When
    uint8_t recv_buffer[8] = {0};
    uint8_t* send_buffer = (uint8_t*)"12345678";
    CHECK(tcs_send(client_socket, send_buffer, 8, TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);
    CHECK(tcs_receive(accept_socket, recv_buffer, 8, TCS_MSG_WAITALL, NULL) == TCS_SUCCESS);

    // Then
    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    // Clean up
    CHECK(tcs_destroy(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_destroy(&accept_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple 2 msg tcs_receive_line")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_NULLSOCKET;
    TcsSocket server_socket = TCS_NULLSOCKET;
    TcsSocket client_socket = TCS_NULLSOCKET;

    CHECK(tcs_create(&listen_socket, TCS_TYPE_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_create(&client_socket, TCS_TYPE_TCP_IP4) == TCS_SUCCESS);

    CHECK(tcs_set_reuse_address(listen_socket, true) == TCS_SUCCESS);
    CHECK(tcs_listen_to(listen_socket, 1212) == TCS_SUCCESS);
    CHECK(tcs_connect(client_socket, "localhost", 1212) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &server_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_destroy(&listen_socket) == TCS_SUCCESS);

    uint8_t msg[] = "hello:world";
    uint8_t part1[32] = {0};
    uint8_t part2[6] = {0};
    uint8_t part3[32] = {0};
    size_t part1_length = 0;
    size_t part2_length = 0;
    size_t part3_length = 0;

    // When
    CHECK(tcs_send(client_socket, msg, sizeof(msg), TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);
    CHECK(tcs_receive_line(server_socket, part1, sizeof(part1), &part1_length, ':') == TCS_SUCCESS);
    CHECK(tcs_receive_line(server_socket, part2, sizeof(part2), &part2_length, '\0') == TCS_SUCCESS);

    CHECK(tcs_set_receive_timeout(server_socket, 10) == TCS_SUCCESS);
    CHECK(tcs_receive_line(server_socket, part3, sizeof(part3), &part3_length, '\0') == TCS_ERROR_TIMED_OUT);

    CHECK(tcs_send(client_socket, msg, sizeof(msg) - 3, TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);
    CHECK(tcs_receive_line(server_socket, part3, sizeof(part3), &part3_length, '\0') == TCS_ERROR_TIMED_OUT);
    CHECK(tcs_send(client_socket, msg + sizeof(msg) - 3, 3, TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);
    CHECK(tcs_receive_line(server_socket, part3 + part3_length, sizeof(part3), &part3_length, '\0') == TCS_SUCCESS);

    // Then
    CHECK(part1_length == 6);
    CHECK(part2_length == 6);
    CHECK(part3_length == 3);
    CHECK(memcmp(part1, msg, 6) == 0);
    CHECK(memcmp(part2, msg + 6, 6) == 0);
    CHECK(memcmp(part3, msg, sizeof(msg)) == 0);

    // Clean up
    CHECK(tcs_destroy(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_destroy(&server_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Partial msg tcs_receive_line")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_NULLSOCKET;
    TcsSocket server_socket = TCS_NULLSOCKET;
    TcsSocket client_socket = TCS_NULLSOCKET;

    CHECK(tcs_create(&listen_socket, TCS_TYPE_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_create(&client_socket, TCS_TYPE_TCP_IP4) == TCS_SUCCESS);

    CHECK(tcs_set_reuse_address(listen_socket, true) == TCS_SUCCESS);
    CHECK(tcs_listen_to(listen_socket, 1212) == TCS_SUCCESS);
    CHECK(tcs_connect(client_socket, "localhost", 1212) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &server_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_destroy(&listen_socket) == TCS_SUCCESS);

    uint8_t msg[] = "hello world\n";
    uint8_t part1[10] = {0};
    uint8_t part2[10] = {0};
    size_t part1_length = 0;
    size_t part2_length = 0;

    // When
    CHECK(tcs_send(client_socket, msg, sizeof(msg), TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);
    CHECK(tcs_receive_line(server_socket, part1, sizeof(part1), &part1_length, '\n') == TCS_AGAIN);
    CHECK(tcs_receive_line(server_socket, part2, sizeof(part2), &part2_length, '\n') == TCS_SUCCESS);

    // Then
    CHECK(part1_length == 10);
    CHECK(part2_length == 2);
    CHECK(memcmp(part1, msg, 10) == 0);
    CHECK(memcmp(part2, msg + 10, 2) == 0);

    // Clean up
    CHECK(tcs_destroy(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_destroy(&server_socket) == TCS_SUCCESS);
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

    CHECK(tcs_create(&listen_socket, TCS_TYPE_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_create(&client_socket, TCS_TYPE_TCP_IP4) == TCS_SUCCESS);

    CHECK(tcs_set_reuse_address(listen_socket, true) == TCS_SUCCESS);
    CHECK(tcs_listen_to(listen_socket, 1212) == TCS_SUCCESS);
    CHECK(tcs_connect(client_socket, "localhost", 1212) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_destroy(&listen_socket) == TCS_SUCCESS);

    // When
    uint8_t recv_buffer[16] = {0};
    uint8_t* send_buffer = (uint8_t*)"12345678";
    CHECK(tcs_send_netstring(client_socket, send_buffer, 8) == TCS_SUCCESS);
    CHECK(tcs_receive_netstring(accept_socket, recv_buffer, 16, NULL) == TCS_SUCCESS);

    // Then
    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    // Clean up
    CHECK(tcs_destroy(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_destroy(&accept_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

// TODO(markusl): Broken on Windows (use nonblocking behind the curton?)
#ifdef CROSS_ISSUE
TEST_CASE("shutdown")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket peer1 = TCS_NULLSOCKET;
    TcsSocket peer2 = TCS_NULLSOCKET;

    CHECK(tcs_create(&peer1, TCS_TYPE_UDP_IP4) == TCS_SUCCESS);
    CHECK(tcs_create(&peer2, TCS_TYPE_UDP_IP4) == TCS_SUCCESS);

    CHECK(tcs_bind(peer1, 5678) == TCS_SUCCESS);
    CHECK(tcs_bind(peer2, 5679) == TCS_SUCCESS);

    std::thread t1([&]() {
        TcsAddress peer1_addr = TCS_ADDRESS_NULL;
        tcs_util_string_to_address("localhost:5678", &peer1_addr);
        peer1_addr.family = TCS_AF_IP4;
        uint8_t buffer2[1024] = "go!";
        tcs_send_to(peer2, buffer2, 4, TCS_NO_FLAGS, &peer1_addr, NULL);

        size_t received = 0;
        tcs_receive(peer2, buffer2, 1024, TCS_NO_FLAGS, &received);
        tcs_destroy(&peer2);
    });

    uint8_t buffer1[1024];
    tcs_receive(peer1, buffer1, 1024, TCS_NO_FLAGS, NULL);
    tcs_shutdown(peer2, TCS_SD_RECEIVE);
    tcs_destroy(&peer1);

    t1.join();

    // Clean up
    tcs_lib_free();
}
#endif

TEST_CASE("tcs_pool simple memory check")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    int pre_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // When
    struct TcsPool* pool = NULL;
    CHECK(tcs_pool_create(&pool) == TCS_SUCCESS);
    CHECK(tcs_pool_destory(&pool) == TCS_SUCCESS);
    int post_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // Then
    CHECK(pre_mem_diff == post_mem_diff);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_pool_poll simple write")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    int pre_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;
    TcsSocket socket = TCS_NULLSOCKET;

    CHECK(tcs_create(&socket, TCS_TYPE_UDP_IP4) == TCS_SUCCESS);

    CHECK(tcs_bind(socket, 5678) == TCS_SUCCESS);
    int user_data = 1337;
    struct TcsPool* pool = NULL;
    CHECK(tcs_pool_create(&pool) == TCS_SUCCESS);
    CHECK(tcs_pool_add(pool, socket, (void*)&user_data, false, true, false) == TCS_SUCCESS);

    // When
    size_t populated = 0;
    TcsPollEvent ev = TCS_NULLEVENT;
    CHECK(tcs_pool_poll(pool, &ev, 1, &populated, 5000) == TCS_SUCCESS);
    CHECK(tcs_pool_destory(&pool) == TCS_SUCCESS);
    int post_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // Then
    CHECK(populated == 1);
    CHECK(ev.can_read == false);
    CHECK(ev.can_write == true);
    CHECK(ev.error == TCS_SUCCESS);
    CHECK(ev.user_data == &user_data);
    CHECK(*(int*)(ev.user_data) == user_data);
    CHECK(pre_mem_diff == post_mem_diff);

    // Clean up
    CHECK(tcs_destroy(&socket) == TCS_SUCCESS);
    tcs_lib_free();
}

TEST_CASE("tcs_pool_poll simple read")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_NULLSOCKET;

    CHECK(tcs_create(&socket, TCS_TYPE_UDP_IP4) == TCS_SUCCESS);
    int allocation_diff_before = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;
    CHECK(tcs_bind(socket, 5679) == TCS_SUCCESS);
    int user_data = 1337;
    struct TcsPool* pool = NULL;
    CHECK(tcs_pool_create(&pool) == TCS_SUCCESS);
    CHECK(tcs_pool_add(pool, socket, (void*)&user_data, true, false, false) == TCS_SUCCESS);

    // When
    size_t populated = 0;
    TcsPollEvent ev = TCS_NULLEVENT;
    CHECK(tcs_pool_poll(pool, &ev, 1, &populated, 0) == TCS_ERROR_TIMED_OUT);

    // Then
    CHECK(populated == 0);

    // When
    TcsAddress receiver = TCS_ADDRESS_NULL;
    CHECK(tcs_util_string_to_address("127.0.0.1:5679", &receiver) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket, (const uint8_t*)"hej", 4, TCS_NO_FLAGS, &receiver, NULL) == TCS_SUCCESS);
    CHECK(tcs_pool_poll(pool, &ev, 1, &populated, 10) == TCS_SUCCESS);
    CHECK(tcs_pool_destory(&pool) == TCS_SUCCESS);
    int allocation_diff_after = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // Then
    CHECK(allocation_diff_before == allocation_diff_after);
    CHECK(populated == 1);
    CHECK(ev.can_read == true);
    CHECK(ev.can_write == false);
    CHECK(ev.error == TCS_SUCCESS);
    CHECK(ev.user_data == &user_data);
    CHECK(*(int*)(ev.user_data) == user_data);

    // Clean up
    CHECK(tcs_destroy(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Address information count")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    size_t no_of_found_addresses = 0;

    // When
    CHECK(tcs_resolve_hostname("localhost", TCS_AF_IP4, NULL, 0, &no_of_found_addresses) == TCS_SUCCESS);

    // Then
    CHECK(no_of_found_addresses > 0);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Get number of local addresses")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);
    int pre_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // Given
    size_t peek_no_of_found_addresses = 0;
    size_t no_of_found_addresses = 0;
    struct TcsInterface interfaces[128];

    // When
    CHECK(tcs_local_interfaces(NULL, 0, &peek_no_of_found_addresses) == TCS_SUCCESS);
    CHECK(tcs_local_interfaces(interfaces, 128, &no_of_found_addresses) == TCS_SUCCESS);
    int post_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // Then
    CHECK(no_of_found_addresses > 0);
    CHECK(peek_no_of_found_addresses == no_of_found_addresses); // Fails if computer has more than 128 addresses
    CHECK(post_mem_diff == pre_mem_diff);

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
    int pre_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // When
    CHECK(tcs_local_interfaces(interfaces, 32, &no_of_found_addresses) == TCS_SUCCESS);
    // find IPv4 loopback
    for (size_t i = 0; i < no_of_found_addresses; ++i)
    {
        if (interfaces[i].address.family == TCS_AF_IP4 &&
            interfaces[i].address.data.af_inet.address == TCS_ADDRESS_LOOPBACK_IP4)
        {
            found_loopback = true;
        }
    }

    // Debug
    for (size_t i = 0; i < no_of_found_addresses; ++i)
    {
        std::cout << interfaces[i].name << " => " << interfaces[i].address << std::endl;
    }
    int post_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // Then
    CHECK(no_of_found_addresses > 0);
    CHECK(found_loopback);
    CHECK(post_mem_diff == pre_mem_diff);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS); // We are in C++, we should use defer
}

TEST_CASE("tcs_util_address_to_string with only IPv4")
{
    // Given
    TcsAddress addr;
    addr.family = TCS_AF_IP4;
    addr.data.af_inet.port = 0;
    addr.data.af_inet.address = TCS_ADDRESS_LOOPBACK_IP4;

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
    addr.data.af_inet.address = TCS_ADDRESS_LOOPBACK_IP4;

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
    CHECK(address.data.af_inet.address == TCS_ADDRESS_LOOPBACK_IP4);
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
    CHECK(address.data.af_inet.address == TCS_ADDRESS_LOOPBACK_IP4);
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
    CHECK(tcs_create_ext(&socket_false, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_true, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_fail, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);

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

    CHECK(tcs_create_ext(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_create_ext(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_create_ext(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_create_ext(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_create_ext(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_create_ext(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_create_ext(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_IPPROTO_TCP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);

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
    CHECK(tcs_util_ipv4_args(239, 255, 255, 251, &multicast_address.data.af_inet.address) == TCS_SUCCESS);
    multicast_address.data.af_inet.port = 1901;

    TcsSocket socket = TCS_NULLSOCKET;
    CHECK(tcs_create_ext(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, 0) == TCS_SUCCESS);
    CHECK(tcs_set_reuse_address(socket, true) == TCS_SUCCESS);
    CHECK(tcs_set_receive_timeout(socket, 5000) == TCS_SUCCESS);

    CHECK(tcs_bind_address(socket, &address_any) == TCS_SUCCESS);
    uint8_t msg[] = "hello world\n";

    // When
    CHECK(tcs_set_ip_multicast_add(socket, &address_any, &multicast_address) == TCS_SUCCESS);

    uint8_t c;
    size_t a = sizeof(c);
    CHECK(tcs_get_option(socket, TCS_SOL_IP, TCS_SO_IP_MULTICAST_LOOP, &c, &a) == TCS_SUCCESS);

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
    CHECK(tcs_create_ext(&socket_send, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    CHECK(tcs_create_ext(&socket_recv, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_IPPROTO_UDP) == TCS_SUCCESS);
    tcs_set_receive_timeout(socket_recv, 1000);
    CHECK(tcs_set_reuse_address(socket_recv, true) == TCS_SUCCESS);

    TcsAddress address_any = {TCS_AF_IP4, {0, 0}};
    address_any.data.af_inet.port = 1901;

    TcsAddress multicast_address = {TCS_AF_IP4, {0, 0}};
    CHECK(tcs_util_ipv4_args(239, 255, 255, 251, &multicast_address.data.af_inet.address) == TCS_SUCCESS);
    multicast_address.data.af_inet.port = 1901;

    CHECK(tcs_bind_address(socket_recv, &address_any) == TCS_SUCCESS);
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
