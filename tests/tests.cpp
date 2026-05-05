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

#include "tinycsocket.h"
#include "tinydatastructures.h"

// illumos <cmath> lacks the C++11 required integer overload for log10 (section 26.8)
#ifdef __sun
#include <cmath>
static inline double log10(unsigned int x)
{
    return std::log10(static_cast<double>(x));
}
#endif

#define DOCTEST_CONFIG_IMPLEMENT
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#include "doctest.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#ifdef DO_WRAP
#include "dbg_wrap.h"
#endif
#include "mock.h"

#include <cstring>
#include <thread>

#ifdef TINYCSOCKET_USE_POSIX_IMPL
#define CHECK_POSIX CHECK
#else
#define CHECK_POSIX WARN
#endif

// We need to be able to run the tests without wrap due to header_only clashes
#ifdef DO_WRAP
#define TCS_MEM_DIFF() (MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER)
#define CHECK_NO_LEAK(pre) CHECK(TCS_MEM_DIFF() == (pre))
#else
#define TCS_MEM_DIFF() 0
#define CHECK_NO_LEAK(pre) ((void)(pre))
#endif

static constexpr size_t kInterfaceListCapacity = 128;
static constexpr size_t kAddressListCapacity = 128;

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

#ifdef DO_WRAP
TEST_CASE("Check mock")
{
    int pre_alloc = MOCK_ALLOC_COUNTER;
    int pre_free = MOCK_FREE_COUNTER;
    void* a = malloc(8);
    free(a);
    CHECK(MOCK_ALLOC_COUNTER > pre_alloc);
    CHECK(MOCK_FREE_COUNTER > pre_free);
}
#endif

TEST_CASE("Example from README")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket client_socket = TCS_SOCKET_INVALID;
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_connect_str(client_socket, "example.com", 80) == TCS_SUCCESS);

    uint8_t send_buffer[] =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";
    CHECK(tcs_send(client_socket, send_buffer, sizeof(send_buffer) - 1, TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);

    static uint8_t recv_buffer[8192] = {0};
    size_t bytes_received = 0;
    tcs_receive(client_socket, recv_buffer, sizeof(recv_buffer), TCS_MSG_WAITALL, &bytes_received);
    CHECK(bytes_received > 0);
    TcsResult shutdown_res = tcs_shutdown(client_socket, TCS_SHUTDOWN_BOTH);
    CHECK((shutdown_res == TCS_SUCCESS || shutdown_res == TCS_ERROR_NOT_CONNECTED ||
           shutdown_res == TCS_ERROR_CONNECTION_RESET || shutdown_res == TCS_ERROR_SOCKET_CLOSED));
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);

    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Init Test")
{
    // Given
    int pre_mem_diff = TCS_MEM_DIFF();

    // When
    CHECK(tcs_lib_init() == TCS_SUCCESS);
    CHECK(tcs_lib_free() == TCS_SUCCESS);

    // Then
    CHECK_NO_LEAK(pre_mem_diff);
}

TEST_CASE("Create socket")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;
    int pre_mem_diff = TCS_MEM_DIFF();

    // When
    TcsResult sts = tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP);

    // Then
    CHECK(sts == TCS_SUCCESS);
    CHECK(socket != TCS_SOCKET_INVALID);
    CHECK_NO_LEAK(pre_mem_diff);

    // Clean up
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("UDP Test")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Gíven
    TcsSocket socket_recv = TCS_SOCKET_INVALID;
    TcsSocket socket_send = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket_recv, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_send, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_opt_receive_timeout_set(socket_recv, 5000) == TCS_SUCCESS);
    CHECK(tcs_opt_reuse_address_set(socket_recv, true) == TCS_SUCCESS);

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1432;

    CHECK(tcs_bind(socket_recv, &local_address) == TCS_SUCCESS);

    uint8_t msg[] = "hello world\n";
    size_t sent = 0;
    uint8_t recv_buffer[1024] = {0};
    size_t recv_size = sizeof(recv_buffer) - sizeof('\0');
    size_t bytes_received = 0;

    TcsAddress address;
    address.family = TCS_FAMILY_IP4;
    address.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    address.data.ip4.port = 1432;

    // When
    CHECK(tcs_send_to(socket_send, msg, sizeof(msg), TCS_FLAG_NONE, &address, &sent) == TCS_SUCCESS);
    CHECK(tcs_receive(socket_recv, recv_buffer, recv_size, TCS_FLAG_NONE, &bytes_received) == TCS_SUCCESS);
    recv_buffer[bytes_received] = '\0';

    // Then
    CHECK(sent > 0);
    CHECK(strcmp((const char*)recv_buffer, (const char*)msg) == 0);

    // Clean up
    CHECK(tcs_close(&socket_recv) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_send) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Bind UDP")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    // When
    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1465;

    TcsResult sts = tcs_bind(socket, &local_address);

    // Then
    CHECK(sts == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Non-blocking")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;
    REQUIRE(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    // When
    TcsResult sts = tcs_opt_nonblocking_set(socket, true);
    bool is_non_blocking = false;
    TcsResult get_sts = tcs_opt_nonblocking_get(socket, &is_non_blocking);

    TcsResult connect_res = tcs_connect_str(socket, "127.0.0.1", 1337);
    CHECK((connect_res == TCS_IN_PROGRESS || connect_res == TCS_ERROR_CONNECTION_REFUSED));

    // Then
    CHECK(sts == TCS_SUCCESS);
    CHECK_POSIX(get_sts == TCS_SUCCESS);
    CHECK_POSIX(is_non_blocking == true);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple TCP Test")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    REQUIRE(tcs_socket(&listen_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1212;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    REQUIRE(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    REQUIRE(tcs_connect_str(client_socket, "localhost", 1212) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    // When
    uint8_t recv_buffer[8] = {0};
    const uint8_t* send_buffer = (const uint8_t*)"12345678";
    CHECK(tcs_send(client_socket, send_buffer, 8, TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);
    CHECK(tcs_receive(accept_socket, recv_buffer, 8, TCS_MSG_WAITALL, NULL) == TCS_SUCCESS);

    // Then
    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    // Clean up
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple 2 msg tcs_receive_line")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket server_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    REQUIRE(tcs_socket(&listen_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1215;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    CHECK(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    REQUIRE(tcs_connect_str(client_socket, "localhost", 1215) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &server_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

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

    CHECK(tcs_opt_receive_timeout_set(server_socket, 2000) == TCS_SUCCESS);
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
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&server_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Partial msg tcs_receive_line")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket server_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    REQUIRE(tcs_socket(&listen_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1216;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    REQUIRE(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    REQUIRE(tcs_connect_str(client_socket, "localhost", 1216) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &server_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

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
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&server_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("sendv")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    REQUIRE(tcs_socket(&listen_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1217;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    REQUIRE(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    REQUIRE(tcs_connect_str(client_socket, "localhost", 1217) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    // When
    TcsBuffer send_buffers[3];
    send_buffers[0].data = (const uint8_t*)"12345678";
    send_buffers[0].size = 8;
    send_buffers[1].data = (const uint8_t*)"ABCDEFGH";
    send_buffers[1].size = 8;
    send_buffers[2].data = (const uint8_t*)"abcdefgh";
    send_buffers[2].size = 8;
    CHECK(tcs_sendv(client_socket, send_buffers, 3, TCS_FLAG_NONE, NULL) == TCS_SUCCESS);

    uint8_t recv_buffer[24] = {0};
    CHECK(tcs_receive(accept_socket, recv_buffer, 24, TCS_FLAG_NONE, NULL) == TCS_SUCCESS);

    // Then
    for (int i = 0; i < 3; ++i)
        CHECK(memcmp(send_buffers[i].data, recv_buffer + i * 8, 8) == 0);

    // Clean up
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple TCP Netstring Test")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    REQUIRE(tcs_socket(&listen_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1218;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    REQUIRE(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    REQUIRE(tcs_connect_str(client_socket, "localhost", 1218) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    // When
    uint8_t recv_buffer[16] = {0};
    const uint8_t* send_buffer = (const uint8_t*)"12345678";
    CHECK(tcs_send_netstring(client_socket, send_buffer, 8) == TCS_SUCCESS);
    CHECK(tcs_receive_netstring(accept_socket, recv_buffer, 16, NULL) == TCS_SUCCESS);

    // Then
    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    // Clean up
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Netstring with multi-digit length")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    REQUIRE(tcs_socket(&listen_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1213;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    CHECK(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    REQUIRE(tcs_connect_str(client_socket, "localhost", 1213) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    // When
    uint8_t recv_buffer[64] = {0};
    const uint8_t* send_buffer = (const uint8_t*)"abcdefghijkl"; // 12 bytes
    CHECK(tcs_send_netstring(client_socket, send_buffer, 12) == TCS_SUCCESS);

    size_t bytes_received = 0;
    CHECK(tcs_receive_netstring(accept_socket, recv_buffer, 64, &bytes_received) == TCS_SUCCESS);

    // Then
    CHECK(bytes_received == 12);
    CHECK(memcmp(recv_buffer, send_buffer, 12) == 0);

    // Clean up
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Netstring with overflowing length is rejected")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    REQUIRE(tcs_socket(&listen_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1214;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    CHECK(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    REQUIRE(tcs_connect_str(client_socket, "localhost", 1214) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    // Send a raw netstring header with a length that would overflow size_t
    const uint8_t overflow_header[] = "99999999999999999999:";
    CHECK(tcs_send(client_socket, overflow_header, sizeof(overflow_header) - 1, TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);

    uint8_t recv_buffer[32] = {0};
    CHECK(tcs_receive_netstring(accept_socket, recv_buffer, sizeof(recv_buffer), NULL) == TCS_ERROR_ILL_FORMED_MESSAGE);

    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);
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

    CHECK(tcs_socket(&peer1, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_socket(&peer2, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    CHECK(tcs_bind(peer1, 5678) == TCS_SUCCESS);
    CHECK(tcs_bind(peer2, 5679) == TCS_SUCCESS);

    std::thread t1([&]() {
        TcsAddress peer1_addr = TCS_ADDRESS_NULL;
        tcs_util_string_to_address("localhost:5678", &peer1_addr);
        peer1_addr.family = TCS_FAMILY_IP4;
        uint8_t buffer2[1024] = "go!";
        tcs_send_to(peer2, buffer2, 4, TCS_NO_FLAGS, &peer1_addr, NULL);

        size_t received = 0;
        tcs_receive(peer2, buffer2, 1024, TCS_NO_FLAGS, &received);
        tcs_close(&peer2);
    });

    uint8_t buffer1[1024];
    tcs_receive(peer1, buffer1, 1024, TCS_NO_FLAGS, NULL);
    tcs_shutdown(peer2, TCS_SHUTDOWN_RECEIVE);
    tcs_close(&peer1);

    t1.join();

    // Clean up
    tcs_lib_free();
}
#endif

TEST_CASE("tcs_socket_tcp bind and connect")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    local_address.data.ip4.port = 1470;

    CHECK(tcs_socket_tcp(&listen_socket, &local_address, NULL, 0) == TCS_SUCCESS);
    REQUIRE(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);

    struct TcsAddress remote_address = TCS_ADDRESS_NONE;
    remote_address.family = TCS_FAMILY_IP4;
    remote_address.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    remote_address.data.ip4.port = 1470;

    CHECK(tcs_socket_tcp(&client_socket, NULL, &remote_address, 5000) == TCS_SUCCESS);
    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);

    // When
    const uint8_t* send_buffer = (const uint8_t*)"hello";
    uint8_t recv_buffer[8] = {0};
    CHECK(tcs_send(client_socket, send_buffer, 5, TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);
    CHECK(tcs_receive(accept_socket, recv_buffer, 5, TCS_MSG_WAITALL, NULL) == TCS_SUCCESS);

    // Then
    CHECK(memcmp(recv_buffer, send_buffer, 5) == 0);

    // Clean up
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_tcp invalid arguments")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Both NULL
    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket_tcp(&socket, NULL, NULL, 0) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(socket == TCS_SOCKET_INVALID);

    // Family mismatch
    struct TcsAddress local4 = TCS_ADDRESS_NONE;
    local4.family = TCS_FAMILY_IP4;
    local4.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    local4.data.ip4.port = 1470;

    struct TcsAddress remote6 = TCS_ADDRESS_NONE;
    remote6.family = TCS_FAMILY_IP6;
    remote6.data.ip6.port = 1470;

    socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket_tcp(&socket, &local4, &remote6, 0) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(socket == TCS_SOCKET_INVALID);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_tcp connect timeout")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Connect to a non-listening port should time out
    struct TcsAddress remote_address = TCS_ADDRESS_NONE;
    remote_address.family = TCS_FAMILY_IP4;
    remote_address.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    remote_address.data.ip4.port = 1471;

    TcsSocket socket = TCS_SOCKET_INVALID;
    TcsResult res = tcs_socket_tcp(&socket, NULL, &remote_address, 100);

    if (res != TCS_ERROR_TIMED_OUT && res != TCS_ERROR_CONNECTION_REFUSED)
    {
        WARN(res == TCS_ERROR_TIMED_OUT);
        WARN(res == TCS_ERROR_CONNECTION_REFUSED);
        CHECK((res == TCS_ERROR_TIMED_OUT || res == TCS_ERROR_CONNECTION_REFUSED));
    }
    CHECK(socket == TCS_SOCKET_INVALID);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_tcp_str bind and connect")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    CHECK(tcs_socket_tcp_str(&listen_socket, "127.0.0.1:1472", NULL, 0) == TCS_SUCCESS);
    REQUIRE(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);

    CHECK(tcs_socket_tcp_str(&client_socket, NULL, "127.0.0.1:1472", 5000) == TCS_SUCCESS);
    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);

    // When
    const uint8_t* send_buffer = (const uint8_t*)"world";
    uint8_t recv_buffer[8] = {0};
    CHECK(tcs_send(client_socket, send_buffer, 5, TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);
    CHECK(tcs_receive(accept_socket, recv_buffer, 5, TCS_MSG_WAITALL, NULL) == TCS_SUCCESS);

    // Then
    CHECK(memcmp(recv_buffer, send_buffer, 5) == 0);

    // Clean up
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_udp bind and send_to")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_recv = TCS_SOCKET_INVALID;
    TcsSocket socket_send = TCS_SOCKET_INVALID;

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1473;

    CHECK(tcs_socket_udp(&socket_recv, &local_address, NULL) == TCS_SUCCESS);
    CHECK(tcs_opt_receive_timeout_set(socket_recv, 5000) == TCS_SUCCESS);

    struct TcsAddress remote_address = TCS_ADDRESS_NONE;
    remote_address.family = TCS_FAMILY_IP4;
    remote_address.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    remote_address.data.ip4.port = 1473;

    CHECK(tcs_socket_udp(&socket_send, NULL, &remote_address) == TCS_SUCCESS);

    // When
    const uint8_t* send_buffer = (const uint8_t*)"hello";
    uint8_t recv_buffer[8] = {0};
    size_t sent = 0;
    size_t received = 0;
    CHECK(tcs_send(socket_send, send_buffer, 5, TCS_FLAG_NONE, &sent) == TCS_SUCCESS);
    CHECK(tcs_receive(socket_recv, recv_buffer, 5, TCS_FLAG_NONE, &received) == TCS_SUCCESS);

    // Then
    CHECK(sent == 5);
    CHECK(received == 5);
    CHECK(memcmp(recv_buffer, send_buffer, 5) == 0);

    // Clean up
    CHECK(tcs_close(&socket_send) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_recv) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_udp invalid arguments")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Both NULL
    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket_udp(&socket, NULL, NULL) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(socket == TCS_SOCKET_INVALID);

    // Family mismatch
    struct TcsAddress local4 = TCS_ADDRESS_NONE;
    local4.family = TCS_FAMILY_IP4;
    local4.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    local4.data.ip4.port = 1473;

    struct TcsAddress remote6 = TCS_ADDRESS_NONE;
    remote6.family = TCS_FAMILY_IP6;
    remote6.data.ip6.port = 1473;

    socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket_udp(&socket, &local4, &remote6) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(socket == TCS_SOCKET_INVALID);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_udp_str bind and send_to")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_recv = TCS_SOCKET_INVALID;
    TcsSocket socket_send = TCS_SOCKET_INVALID;

    CHECK(tcs_socket_udp_str(&socket_recv, "0.0.0.0:1474", NULL) == TCS_SUCCESS);
    CHECK(tcs_opt_receive_timeout_set(socket_recv, 5000) == TCS_SUCCESS);

    CHECK(tcs_socket_udp_str(&socket_send, NULL, "127.0.0.1:1474") == TCS_SUCCESS);

    // When
    const uint8_t* send_buffer = (const uint8_t*)"world";
    uint8_t recv_buffer[8] = {0};
    size_t sent = 0;
    size_t received = 0;
    CHECK(tcs_send(socket_send, send_buffer, 5, TCS_FLAG_NONE, &sent) == TCS_SUCCESS);
    CHECK(tcs_receive(socket_recv, recv_buffer, 5, TCS_FLAG_NONE, &received) == TCS_SUCCESS);

    // Then
    CHECK(sent == 5);
    CHECK(received == 5);
    CHECK(memcmp(recv_buffer, send_buffer, 5) == 0);

    // Clean up
    CHECK(tcs_close(&socket_send) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_recv) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_poll simple memory check")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    int pre_mem_diff = TCS_MEM_DIFF();

    // When
    struct TcsPoll* poll = NULL;
    CHECK(tcs_poll_create(&poll) == TCS_SUCCESS);
    CHECK(tcs_poll_destroy(&poll) == TCS_SUCCESS);

    // Then
    CHECK_NO_LEAK(pre_mem_diff);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_poll_wait simple write")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    int pre_mem_diff = TCS_MEM_DIFF();
    TcsSocket socket = TCS_SOCKET_INVALID;

    CHECK(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1219;
    CHECK(tcs_bind(socket, &local_address) == TCS_SUCCESS);

    int user_data = 1337;
    struct TcsPoll* poll = NULL;
    CHECK(tcs_poll_create(&poll) == TCS_SUCCESS);
    CHECK(tcs_poll_add(poll, socket, (void*)&user_data, TCS_POLL_WRITE) == TCS_SUCCESS);

    // When
    size_t populated = 0;
    TcsPollEvent ev = TCS_POLL_EVENT_EMPTY;
    CHECK(tcs_poll_wait(poll, &ev, 1, &populated, 5000) == TCS_SUCCESS);
    CHECK(tcs_poll_destroy(&poll) == TCS_SUCCESS);

    // Then
    CHECK(populated == 1);
    CHECK(ev.can_read == false);
    CHECK(ev.can_write == true);
    CHECK(ev.error == TCS_SUCCESS);
    CHECK(ev.user_data == &user_data);
    CHECK(*(int*)(ev.user_data) == user_data);
    CHECK_NO_LEAK(pre_mem_diff);

    // Clean up
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    tcs_lib_free();
}

TEST_CASE("tcs_poll_wait simple read")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    CHECK(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    int allocation_diff_before = TCS_MEM_DIFF();
    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 5679;
    CHECK(tcs_bind(socket, &local_address) == TCS_SUCCESS);
    int user_data = 1337;
    struct TcsPoll* poll = NULL;
    CHECK(tcs_poll_create(&poll) == TCS_SUCCESS);
    CHECK(tcs_poll_add(poll, socket, (void*)&user_data, TCS_POLL_READ) == TCS_SUCCESS);

    // When
    size_t populated = 0;
    TcsPollEvent ev = TCS_POLL_EVENT_EMPTY;
    CHECK(tcs_poll_wait(poll, &ev, 1, &populated, 0) == TCS_ERROR_TIMED_OUT);

    // Then
    CHECK(populated == 0);

    // When
    TcsAddress receiver = TCS_ADDRESS_NONE;
    CHECK(tcs_address_parse("127.0.0.1:5679", &receiver) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket, (const uint8_t*)"hej", 4, TCS_FLAG_NONE, &receiver, NULL) == TCS_SUCCESS);
    CHECK(tcs_poll_wait(poll, &ev, 1, &populated, 10) == TCS_SUCCESS);
    CHECK(tcs_poll_destroy(&poll) == TCS_SUCCESS);

    // Then
    CHECK_NO_LEAK(allocation_diff_before);
    CHECK(populated == 1);
    CHECK(ev.can_read == true);
    CHECK(ev.can_write == false);
    CHECK(ev.error == TCS_SUCCESS);
    CHECK(ev.user_data == &user_data);
    CHECK(*(int*)(ev.user_data) == user_data);

    // Clean up
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_poll_wait partial")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    struct TcsPoll* poll = NULL;
    CHECK(tcs_poll_create(&poll) == TCS_SUCCESS);

    const int SOCKET_COUNT = 3;
    TcsSocket socket[SOCKET_COUNT];
    int user_data[SOCKET_COUNT];

    for (int i = 0; i < SOCKET_COUNT; ++i)
    {
        socket[i] = TCS_SOCKET_INVALID;
        CHECK(tcs_socket(&socket[i], TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

        struct TcsAddress local_address = TCS_ADDRESS_NONE;
        local_address.family = TCS_FAMILY_IP4;
        local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
        local_address.data.ip4.port = (uint16_t)(5000 + i);
        CHECK(tcs_bind(socket[i], &local_address) == TCS_SUCCESS);
        user_data[i] = 5000 + i;
        CHECK(tcs_poll_add(poll, socket[i], (void*)&user_data[i], TCS_POLL_READ) == TCS_SUCCESS);
    }

    // When
    size_t populated = 0;
    TcsPollEvent ev = TCS_POLL_EVENT_EMPTY;
    CHECK(tcs_poll_wait(poll, &ev, 1, &populated, 0) == TCS_ERROR_TIMED_OUT);

    // Then
    CHECK(populated == 0);

    // When
    TcsAddress destinaion_address = TCS_ADDRESS_NONE;
    CHECK(tcs_address_parse("127.0.0.1:5001", &destinaion_address) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket[0], (const uint8_t*)"hej", 4, TCS_FLAG_NONE, &destinaion_address, NULL) == TCS_SUCCESS);
    CHECK(tcs_poll_wait(poll, &ev, 1, &populated, 10) == TCS_SUCCESS);
    CHECK(populated == 1);
    CHECK(*(int*)ev.user_data == 5001);
    CHECK(ev.can_read == true);

    // Then

    // Check that the event and socket is still there after poll
    CHECK(tcs_poll_wait(poll, &ev, 1, &populated, 10) == TCS_SUCCESS);
    CHECK(populated == 1);
    CHECK(*(int*)ev.user_data == 5001);
    CHECK(ev.can_read == true);

    uint8_t receive_buffer[1024];
    size_t received_bytes = 0;
    CHECK(tcs_receive(socket[1], receive_buffer, 1024, TCS_FLAG_NONE, &received_bytes) == TCS_SUCCESS);

    // Check that the event is removed after received data
    CHECK(tcs_poll_wait(poll, &ev, 1, &populated, 10) == TCS_ERROR_TIMED_OUT);

    CHECK(populated == 0);

    // Clean up
    CHECK(tcs_poll_destroy(&poll) == TCS_SUCCESS);
    for (int i = 0; i < SOCKET_COUNT; ++i)
    {
        CHECK(tcs_close(&socket[i]) == TCS_SUCCESS);
    }
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_poll_modify")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 5680;
    CHECK(tcs_bind(socket, &local_address) == TCS_SUCCESS);

    struct TcsPoll* poll = NULL;
    CHECK(tcs_poll_create(&poll) == TCS_SUCCESS);
    CHECK(tcs_poll_add(poll, socket, NULL, TCS_POLL_READ) == TCS_SUCCESS);

    // When — no data, READ should time out
    size_t populated = 0;
    TcsPollEvent ev = TCS_POLL_EVENT_EMPTY;
    CHECK(tcs_poll_wait(poll, &ev, 1, &populated, 0) == TCS_ERROR_TIMED_OUT);
    CHECK(populated == 0);

    // Modify to WRITE — should trigger immediately
    CHECK(tcs_poll_modify(poll, socket, TCS_POLL_WRITE) == TCS_SUCCESS);
    ev = TCS_POLL_EVENT_EMPTY;
    CHECK(tcs_poll_wait(poll, &ev, 1, &populated, 5000) == TCS_SUCCESS);
    CHECK(populated == 1);
    CHECK(ev.can_write == true);
    CHECK(ev.can_read == false);

    // Modify back to READ — should time out again
    CHECK(tcs_poll_modify(poll, socket, TCS_POLL_READ) == TCS_SUCCESS);
    ev = TCS_POLL_EVENT_EMPTY;
    CHECK(tcs_poll_wait(poll, &ev, 1, &populated, 0) == TCS_ERROR_TIMED_OUT);
    CHECK(populated == 0);

    // Modify non-existent socket should fail
    CHECK(tcs_poll_modify(poll, TCS_SOCKET_INVALID, TCS_POLL_WRITE) == TCS_ERROR_INVALID_ARGUMENT);

    // Clean up
    CHECK(tcs_poll_destroy(&poll) == TCS_SUCCESS);
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Address information count")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    size_t no_of_found_addresses = 0;

    // When
    CHECK(tcs_address_resolve("localhost", TCS_FAMILY_IP4, NULL, 0, &no_of_found_addresses) == TCS_SUCCESS);

    // Then
    CHECK(no_of_found_addresses > 0);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}
TEST_CASE("Interface list")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    static struct TcsInterface interfaces[kInterfaceListCapacity];
    size_t no_of_found_interfaces = 0;

    // When
    TcsResult iface_res = tcs_interface_list(interfaces, kInterfaceListCapacity, &no_of_found_interfaces);
    if (iface_res != TCS_SUCCESS)
        printf("tcs_interface_list failed with error code: %d\n", iface_res);
    CHECK(iface_res == TCS_SUCCESS);

    // Then
    CHECK(no_of_found_interfaces > 0);
    size_t iface_display =
        no_of_found_interfaces < kInterfaceListCapacity ? no_of_found_interfaces : kInterfaceListCapacity;
    for (size_t i = 0; i < iface_display; ++i)
    {
        printf("Interface %u: %s\n", interfaces[i].id, interfaces[i].name);

        static struct TcsInterfaceAddress addresses[kAddressListCapacity];
        size_t found_addresses = 0;
        tcs_address_list(interfaces[i].id, TCS_FAMILY_ANY, addresses, kAddressListCapacity, &found_addresses);
        size_t addr_display = found_addresses < kAddressListCapacity ? found_addresses : kAddressListCapacity;
        for (size_t j = 0; j < addr_display; ++j)
        {
            char addr_str[70];
            tcs_address_to_str(&addresses[j].address, addr_str);
            printf("  Address %u,%zu: %s\n", interfaces[i].id, j, addr_str);
        }
    }

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Get loopback address")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    size_t ifaddrs_count = 0;
    static struct TcsInterfaceAddress ifaddrs[kAddressListCapacity];
    bool found_loopback = false;
    int pre_mem_diff = TCS_MEM_DIFF();

    // When
    WARN(tcs_address_list(0, TCS_FAMILY_ANY, ifaddrs, kAddressListCapacity, &ifaddrs_count) == TCS_SUCCESS);
    // find IPv4 loopback
    size_t ifaddrs_display = ifaddrs_count < kAddressListCapacity ? ifaddrs_count : kAddressListCapacity;
    for (size_t i = 0; i < ifaddrs_display; ++i)
    {
        if (ifaddrs[i].address.family.native == TCS_FAMILY_IP4.native &&
            ifaddrs[i].address.data.ip4.address == TCS_ADDRESS_LOOPBACK_IP4)
        {
            char out_str[70];
            tcs_address_to_str(&ifaddrs[i].address, out_str);
            printf("Interface loopback: %u; %s; %s\n", ifaddrs[i].iface.id, ifaddrs[i].iface.name, out_str);
            found_loopback = true;
        }
    }

    // Then
    WARN(ifaddrs_count > 0);
    WARN(found_loopback);
    CHECK_NO_LEAK(pre_mem_diff);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS); // We are in C++, we should use defer
}

TEST_CASE("tcs_util_address_to_string with only IPv4")
{
    // Given
    TcsAddress addr;
    addr.family = TCS_FAMILY_IP4;
    addr.data.ip4.port = 0;
    addr.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;

    // When
    char address_str[70];
    tcs_address_to_str(&addr, address_str);

    // Then
    CHECK_EQ(address_str, "127.0.0.1");
}

TEST_CASE("tcs_util_address_to_string with IPv4 and port")
{
    // Given
    TcsAddress addr;
    addr.family = TCS_FAMILY_IP4;
    addr.data.ip4.port = 1234;
    addr.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;

    // When
    char address_str[70];
    tcs_address_to_str(&addr, address_str);

    // Then
    CHECK_EQ(address_str, "127.0.0.1:1234");
}

TEST_CASE("tcs_util_string_to_address with only IPv4")
{
    // Given
    const char str[] = "127.0.0.1";

    // When
    TcsAddress address;
    tcs_address_parse(str, &address);

    // Then
    CHECK(address.family.native == TCS_FAMILY_IP4.native);
    CHECK(address.data.ip4.port == 0);
    CHECK(address.data.ip4.address == TCS_ADDRESS_LOOPBACK_IP4);
    CHECK(tcs_address_parse("127.256.0.1", &address) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("-1.0.0.1", &address) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("0xFF.01.1.0xFF", &address) == TCS_SUCCESS);
    CHECK(tcs_address_parse("0xFF.01.1", &address) == TCS_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("tcs_util_string_to_address with IPv4and port")
{
    // Given
    const char str[] = "127.0.0.1:1234";

    // When
    TcsAddress address;
    tcs_address_parse(str, &address);

    // Then
    CHECK(address.family.native == TCS_FAMILY_IP4.native);
    CHECK(address.data.ip4.port == 1234);
    CHECK(address.data.ip4.address == TCS_ADDRESS_LOOPBACK_IP4);
    CHECK(tcs_address_parse("127.255.0.1:65536", &address) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("1.0.0.1:-1", &address) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("0xFF.01.1.0xFF:0xFFFF", &address) == TCS_SUCCESS);
    CHECK(tcs_address_parse("0xFF.01.1:123", &address) == TCS_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("TCS_SO_BROADCAST")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_false = TCS_SOCKET_INVALID;
    TcsSocket socket_true = TCS_SOCKET_INVALID;
    TcsSocket socket_fail = TCS_SOCKET_INVALID;
    TcsSocket socket_null = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket_false, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_true, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_fail, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    // When
    CHECK(tcs_opt_broadcast_set(socket_false, false) == TCS_SUCCESS);
    CHECK(tcs_opt_broadcast_set(socket_true, true) == TCS_SUCCESS);
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
    CHECK(tcs_opt_broadcast_get(socket_false, &socket_false_value) == TCS_SUCCESS);
    CHECK(tcs_opt_broadcast_get(socket_true, &socket_true_value) == TCS_SUCCESS);
#ifdef CROSS_ISSUES
    CHECK(tcs_get_broadcast(socket_fail, &socket_fail_value) !=
          TCS_SUCCESS); // TODO(markusl): Not a dgram socket, success on linux
#endif
    CHECK(tcs_opt_broadcast_get(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_false_value == false);
    CHECK(socket_true_value == true);

    // Clean up
    CHECK(tcs_close(&socket_false) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_true) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_fail) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_KEEPALIVE")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_false = TCS_SOCKET_INVALID;
    TcsSocket socket_true = TCS_SOCKET_INVALID;
    TcsSocket socket_fail = TCS_SOCKET_INVALID;
    TcsSocket socket_null = TCS_SOCKET_INVALID;

    CHECK(tcs_socket(&socket_false, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_true, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_fail, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_opt_keep_alive_set(socket_false, false) == TCS_SUCCESS);
    CHECK(tcs_opt_keep_alive_set(socket_true, true) == TCS_SUCCESS);

    // Then
    bool socket_false_value;
    bool socket_true_value;
#ifdef CROSS_ISSUE
    bool socket_fail_value;
#endif
    bool socket_null_value;
    CHECK(tcs_opt_keep_alive_get(socket_false, &socket_false_value) == TCS_SUCCESS);
    CHECK(tcs_opt_keep_alive_get(socket_true, &socket_true_value) == TCS_SUCCESS);
#ifdef CROSS_ISSUES
    CHECK(tcs_get_keep_alive(socket_fail, &socket_fail_value) != TCS_SUCCESS); // Not a stream socket
#endif
    CHECK(tcs_opt_keep_alive_get(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_false_value == false);
    CHECK(socket_true_value == true);

    // Clean up
    CHECK(tcs_close(&socket_false) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_true) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_fail) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_LINGER")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_false = TCS_SOCKET_INVALID;
    TcsSocket socket_true = TCS_SOCKET_INVALID;
    TcsSocket socket_fail = TCS_SOCKET_INVALID;
    TcsSocket socket_null = TCS_SOCKET_INVALID;

    CHECK(tcs_socket(&socket_false, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_true, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_fail, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_opt_linger_set(socket_false, false, 3) == TCS_SUCCESS);
    CHECK(tcs_opt_linger_set(socket_true, true, 2) == TCS_SUCCESS);

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
    CHECK(tcs_opt_linger_get(socket_false, &socket_false_value, &socket_false_time) == TCS_SUCCESS);
    CHECK(tcs_opt_linger_get(socket_true, &socket_true_value, &socket_true_time) == TCS_SUCCESS);
#ifdef CROSS_ISSUES
    CHECK(tcs_opt_linger_get(socket_fail, &socket_fail_value, &socket_fail_time) != TCS_SUCCESS); // Not a stream socket
#endif
    CHECK(tcs_opt_linger_get(socket_null, &socket_null_value, NULL) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_false_value == false);
    CHECK(socket_true_value == true);

    // Clean up
    CHECK(tcs_close(&socket_false) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_true) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_fail) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_REUSEADDR")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_false = TCS_SOCKET_INVALID;
    TcsSocket socket_true = TCS_SOCKET_INVALID;
    TcsSocket socket_fail = TCS_SOCKET_INVALID;
    TcsSocket socket_null = TCS_SOCKET_INVALID;

    CHECK(tcs_socket(&socket_false, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_true, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_fail, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_opt_reuse_address_set(socket_false, false) == TCS_SUCCESS);
    CHECK(tcs_opt_reuse_address_set(socket_true, true) == TCS_SUCCESS);
    CHECK(tcs_opt_reuse_address_set(socket_fail, true) == TCS_SUCCESS);

    // Then
    bool socket_false_value;
    bool socket_true_value;
    bool socket_dgram_value;
    bool socket_null_value;
    CHECK(tcs_opt_reuse_address_get(socket_false, &socket_false_value) == TCS_SUCCESS);
    CHECK(tcs_opt_reuse_address_get(socket_true, &socket_true_value) == TCS_SUCCESS);
    CHECK(tcs_opt_reuse_address_get(socket_fail, &socket_dgram_value) == TCS_SUCCESS);
    CHECK(tcs_opt_reuse_address_get(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_false_value == false);
    CHECK(socket_true_value == true);
    CHECK(socket_dgram_value == true);

    // Clean up
    CHECK(tcs_close(&socket_false) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_true) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_fail) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_RCVBUF")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;
    TcsSocket socket_null = TCS_SOCKET_INVALID;

    CHECK(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_opt_receive_buffer_size_set(socket, 8192) == TCS_SUCCESS);

    // Then
    size_t socket_rcvbuf_value;
    size_t socket_null_value;
    CHECK(tcs_opt_receive_buffer_size_get(socket, &socket_rcvbuf_value) == TCS_SUCCESS);
    CHECK(tcs_opt_receive_buffer_size_get(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_rcvbuf_value == 8192);

    // Clean up
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_SNDBUF")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;
    TcsSocket socket_null = TCS_SOCKET_INVALID;

    CHECK(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_opt_send_buffer_size_set(socket, 8192) == TCS_SUCCESS);

    // Then
    size_t socket_rcvbuf_value;
    size_t socket_null_value;
    CHECK(tcs_opt_send_buffer_size_get(socket, &socket_rcvbuf_value) == TCS_SUCCESS);
    CHECK(tcs_opt_send_buffer_size_get(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_rcvbuf_value == 8192);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TCS_SO_RCVTIMEO")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;
    TcsSocket socket_null = TCS_SOCKET_INVALID;

    CHECK(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_opt_receive_timeout_set(socket, 920) == TCS_SUCCESS);

    // Then
    int socket_receive_timeout_value;
    int socket_null_value;
    CHECK(tcs_opt_receive_timeout_get(socket, &socket_receive_timeout_value) == TCS_SUCCESS);
    CHECK(tcs_opt_receive_timeout_get(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

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
    TcsSocket socket_false = TCS_SOCKET_INVALID;
    TcsSocket socket_true = TCS_SOCKET_INVALID;
    TcsSocket socket_fail = TCS_SOCKET_INVALID;
    TcsSocket socket_null = TCS_SOCKET_INVALID;

    CHECK(tcs_socket(&socket_false, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_true, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_fail, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

    // When
    CHECK(tcs_opt_out_of_band_inline_set(socket_false, false) == TCS_SUCCESS);
    CHECK(tcs_opt_out_of_band_inline_set(socket_true, true) == TCS_SUCCESS);

    // Then
    bool socket_false_value;
    bool socket_true_value;
#ifdef CROSS_ISSUES
    bool socket_fail_value;
#endif
    bool socket_null_value;
    CHECK(tcs_opt_out_of_band_inline_get(socket_false, &socket_false_value) == TCS_SUCCESS);
    CHECK(tcs_opt_out_of_band_inline_get(socket_true, &socket_true_value) == TCS_SUCCESS);
#ifdef CROSS_ISSUES
    CHECK(tcs_get_out_of_band_inline(socket_fail, &socket_fail_value) != TCS_SUCCESS); // Not a stream socket
#endif
    CHECK(tcs_opt_out_of_band_inline_get(socket_null, &socket_null_value) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(socket_false_value == false);
    CHECK(socket_true_value == true);

    // Clean up
    CHECK(tcs_close(&socket_false) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_true) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_fail) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple Multicast Add Membership")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsAddress loopback = TCS_ADDRESS_NONE;
    loopback.family = TCS_FAMILY_IP4;
    loopback.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;

    TcsAddress address_any = {TCS_FAMILY_IP4, {{0, 0}}};
    address_any.data.ip4.port = 1901;

    TcsAddress multicast_address = TCS_ADDRESS_NONE;
    tcs_address_parse("239.255.255.251:1901", &multicast_address);

    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, 0) == TCS_SUCCESS);
    CHECK(tcs_opt_reuse_address_set(socket, true) == TCS_SUCCESS);
    CHECK(tcs_opt_receive_timeout_set(socket, 5000) == TCS_SUCCESS);
    CHECK(tcs_opt_multicast_interface_set(socket, &loopback) == TCS_SUCCESS);
    CHECK(tcs_opt_multicast_loop_set(socket, true) == TCS_SUCCESS);

    CHECK(tcs_bind(socket, &address_any) == TCS_SUCCESS);
    uint8_t msg[] = "hello world\n";

    // When
    CHECK(tcs_opt_membership_add_to(socket, &loopback, &multicast_address) == TCS_SUCCESS);

    CHECK(tcs_send_to(socket, msg, sizeof(msg), 0, &multicast_address, NULL) == TCS_SUCCESS);

    // Then
    uint8_t recv_buffer[1024] = {0};
    size_t bytes_received = 0;
    CHECK(tcs_receive(socket, recv_buffer, sizeof(recv_buffer), 0, &bytes_received) == TCS_SUCCESS);
    CHECK(strcmp(reinterpret_cast<const char*>(recv_buffer), reinterpret_cast<const char*>(msg)) == 0);

    // Clean up
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_opt_membership_add_str")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsAddress loopback = TCS_ADDRESS_NONE;
    loopback.family = TCS_FAMILY_IP4;
    loopback.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    loopback.data.ip4.port = 1902;

    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, 0) == TCS_SUCCESS);
    CHECK(tcs_bind(socket, &loopback) == TCS_SUCCESS);

    // When/Then
    CHECK(tcs_opt_membership_add_str(socket, "239.255.255.251") == TCS_SUCCESS);

    // Clean up
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Multicast Add-Drop-Add Membership")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsAddress loopback = TCS_ADDRESS_NONE;
    loopback.family = TCS_FAMILY_IP4;
    loopback.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;

    TcsSocket socket_send = TCS_SOCKET_INVALID;
    TcsSocket socket_recv = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket_send, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_recv, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    tcs_opt_receive_timeout_set(socket_recv, 2000);
    CHECK(tcs_opt_reuse_address_set(socket_recv, true) == TCS_SUCCESS);
    CHECK(tcs_opt_multicast_interface_set(socket_send, &loopback) == TCS_SUCCESS);
    CHECK(tcs_opt_multicast_loop_set(socket_send, true) == TCS_SUCCESS);

    TcsAddress address_any = {TCS_FAMILY_IP4, {{0, 0}}};
    address_any.data.ip4.port = 1901;

    TcsAddress multicast_address = TCS_ADDRESS_NONE;
    CHECK(tcs_address_parse("239.255.255.251:1901", &multicast_address) == TCS_SUCCESS);
    CHECK(tcs_bind(socket_recv, &address_any) == TCS_SUCCESS);
    uint8_t msg_1[] = "hello world1\n";
    uint8_t msg_2[] = "hello world2\n";
    uint8_t msg_missed[] = "you can not read me\n";

    // When
    CHECK(tcs_opt_membership_add_to(socket_recv, &loopback, &multicast_address) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket_send, msg_1, sizeof(msg_1), 0, &multicast_address, NULL) == TCS_SUCCESS);
    CHECK(tcs_opt_membership_drop_from(socket_recv, &loopback, &multicast_address) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket_send, msg_missed, sizeof(msg_missed), 0, &multicast_address, NULL) == TCS_SUCCESS);
    CHECK(tcs_opt_membership_add_to(socket_recv, &loopback, &multicast_address) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket_send, msg_2, sizeof(msg_2), 0, &multicast_address, NULL) == TCS_SUCCESS);

    // Then
    static uint8_t recv_buffer_1[1024] = {0};
    static uint8_t recv_buffer_2[1024] = {0};
    size_t bytes_received_1;
    size_t bytes_received_2;

    CHECK(tcs_receive(socket_recv, recv_buffer_1, sizeof(recv_buffer_1), 0, &bytes_received_1) == TCS_SUCCESS);
    CHECK(tcs_receive(socket_recv, recv_buffer_2, sizeof(recv_buffer_2), 0, &bytes_received_2) == TCS_SUCCESS);
    recv_buffer_1[bytes_received_1] = '\0';
    recv_buffer_2[bytes_received_2] = '\0';

    CHECK(reinterpret_cast<const char*>(recv_buffer_1) == reinterpret_cast<const char*>(msg_1));
    CHECK(reinterpret_cast<const char*>(recv_buffer_2) == reinterpret_cast<const char*>(msg_2));

    // Clean up
    CHECK(tcs_close(&socket_recv) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_send) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_opt_membership_drop_str")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsAddress loopback = TCS_ADDRESS_NONE;
    loopback.family = TCS_FAMILY_IP4;
    loopback.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    loopback.data.ip4.port = 1903;

    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, 0) == TCS_SUCCESS);
    CHECK(tcs_bind(socket, &loopback) == TCS_SUCCESS);

    // When/Then
    CHECK(tcs_opt_membership_add_str(socket, "239.255.255.251") == TCS_SUCCESS);
    CHECK(tcs_opt_membership_drop_str(socket, "239.255.255.251") == TCS_SUCCESS);

    // Clean up
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

#if defined(__linux__)
TEST_CASE("Simple AVTP talker")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket, TCS_FAMILY_PACKET, TCS_SOCK_RAW, TCS_PROTOCOL_ETH_ALL) == TCS_SUCCESS);
    TcsAddress address = TCS_ADDRESS_NONE;
    tcs_address_parse("", &address);

    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}
#endif

// ************* TINY DATA STRUCTURE (TDS) UNIT TESTS *************

TEST_CASE("TdsList capacity hysteresis")
{
    // Do not grow if not needed
    CHECK(tds_ulist_best_capacity_fit(8, 8) == 8);
    CHECK(tds_ulist_best_capacity_fit(16, 15) == 16);
    CHECK(tds_ulist_best_capacity_fit(16, 9) == 16);

    // Do not decrease if the change is small
    CHECK(tds_ulist_best_capacity_fit(32, 15) == 32);

    // Do increase if needed
    CHECK(tds_ulist_best_capacity_fit(32, 33) == 64);

    // Do decrease if change is large
    CHECK(tds_ulist_best_capacity_fit(64, 16) == 16);
}

TDS_ULIST_IMPL(int, int)

TEST_CASE("TdsUList create / destroy")
{
    // Given
    struct TdsUList_int list;
    CHECK(tds_ulist_int_create(&list) == 0);
    CHECK(list.capacity == 8);
    CHECK(list.count == 0);
    CHECK(list.data != NULL);

    // Clean up
    CHECK(tds_ulist_int_destroy(&list) == 0);
    CHECK(list.capacity == 0);
    CHECK(list.count == 0);
    CHECK(list.data == NULL);
}

TEST_CASE("TdsUList reserve")
{
    // Given
    struct TdsUList_int list;
    CHECK(tds_ulist_int_create(&list) == 0);
    CHECK(list.capacity == 8);
    CHECK(list.count == 0);
    CHECK(list.data != NULL);

    // When
    CHECK(tds_ulist_int_reserve(&list, 128) == 0);

    // Then
    CHECK(list.capacity >= 128);
    CHECK(list.count == 0);
    CHECK(list.data != NULL);

    // Clean up
    CHECK(tds_ulist_int_destroy(&list) == 0);
}

TEST_CASE("TdsUList add")
{
    // Given
    struct TdsUList_int list;
    CHECK(tds_ulist_int_create(&list) == 0);

    // When
    for (int i = 0; i < 128; ++i)
    {
        CHECK(tds_ulist_int_add(&list, &i, 1) == 0);
    }

    // Then
    CHECK(list.count == 128);
    for (int i = 0; i < 128; ++i)
    {
        CHECK(list.data[i] == i);
    }

    // Clean up
    CHECK(tds_ulist_int_destroy(&list) == 0);
}

TEST_CASE("TdsUList add many")
{
    // Given
    struct TdsUList_int list;
    CHECK(tds_ulist_int_create(&list) == 0);

    int user_data[5] = {0, 1, 2, 3, 4};

    // When
    CHECK(tds_ulist_int_add(&list, user_data, 5) == 0);

    // Then
    CHECK(list.count == 5);
    CHECK(list.data[0] == 0);
    CHECK(list.data[1] == 1);
    CHECK(list.data[2] == 2);
    CHECK(list.data[3] == 3);
    CHECK(list.data[4] == 4);

    // Clean up
    CHECK(tds_ulist_int_destroy(&list) == 0);
}

TEST_CASE("TdsUList remove")
{
    // Given
    struct TdsUList_int list;
    CHECK(tds_ulist_int_create(&list) == 0);
    for (int i = 0; i < 100; ++i)
    {
        CHECK(tds_ulist_int_add(&list, &i, 1) == 0);
    }

    // When
    tds_ulist_int_remove(&list, 1, 3);

    // Then
    CHECK(list.count == 97);
    CHECK(list.data[0] == 0);
    CHECK(list.data[1] != 1);
    CHECK(list.data[2] != 2);
    CHECK(list.data[3] != 3);

    // Clean up
    CHECK(tds_ulist_int_destroy(&list) == 0);
}

TEST_CASE("TdsUList remove capacity")
{
    // Given
    struct TdsUList_int list;
    CHECK(tds_ulist_int_create(&list) == 0);
    for (int i = 0; i < 100; ++i)
    {
        CHECK(tds_ulist_int_add(&list, &i, 1) == 0);
    }

    // When

    for (int i = 0; i < 90; ++i)
    {
        CHECK(tds_ulist_int_remove(&list, 0, 1) == 0);
    }

    // Then
    CHECK(list.count == 10);
    CHECK(list.data[0] != 0);
    CHECK(list.capacity == 32);

    // Clean up
    CHECK(tds_ulist_int_destroy(&list) == 0);
}

TEST_CASE("TdsUList remove overlap")
{
    // Given
    struct TdsUList_int list;
    CHECK(tds_ulist_int_create(&list) == 0);
    for (int i = 0; i < 100; ++i)
    {
        CHECK(tds_ulist_int_add(&list, &i, 1) == 0);
    }

    // When
    CHECK(tds_ulist_int_remove(&list, 0, 90) == 0);

    // Then
    CHECK(list.count == 10);
    CHECK(list.data[0] != 0);
    CHECK(list.capacity == 16); // When removing more than one capacity grow step, hysteresis does not kick in

    // Clean up
    CHECK(tds_ulist_int_destroy(&list) == 0);
}

#ifndef TDS_MAP_int_double
#define TDS_MAP_int_double
TDS_MAP_IMPL(int, double, mymap)
#endif

TEST_CASE("TdsMap_int_double create")
{
    struct TdsMap_mymap map;
    CHECK(tds_map_mymap_create(&map) == 0);
    CHECK(map.capacity > 0);
    CHECK(map.count == 0);
    CHECK(map.keys != NULL);
    CHECK(map.values != NULL);
    CHECK(tds_map_mymap_destroy(&map) == 0);
    CHECK(map.capacity == 0);
    CHECK(map.count == 0);
    CHECK(map.keys == NULL);
    CHECK(map.values == NULL);
}

TEST_CASE("TdsMap_int_double add and remove")
{
    struct TdsMap_mymap map;
    CHECK(tds_map_mymap_create(&map) == 0);
    CHECK(tds_map_mymap_add(&map, 0, 0.0) == 0);
    CHECK(tds_map_mymap_add(&map, 1, 1.0) == 0);
    CHECK(tds_map_mymap_add(&map, 2, 2.0) == 0);
    CHECK(map.keys[0] == 0);
    CHECK(map.keys[1] == 1);
    CHECK(map.keys[2] == 2);
    CHECK_EQ(map.values[0], 0.0);
    CHECK_EQ(map.values[1], 1.0);
    CHECK_EQ(map.values[2], 2.0);
    CHECK(map.count == 3);
    CHECK(tds_map_mymap_remove(&map, 1) == 0);
    CHECK(map.count == 2);
    CHECK(map.keys[1] != 1);
    CHECK_NE(map.values[1], 1.0);
    CHECK(tds_map_mymap_destroy(&map) == 0);
}

#if defined(__linux__)
const uint8_t AVTP_DEST_ADDR[6] = {0x91, 0xe0, 0xf0, 0x00, 0xfe, 0x00};

TEST_CASE("Create packet socket")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    // When
    CHECK(tcs_socket(&socket, TCS_FAMILY_PACKET, TCS_SOCK_RAW, TCS_PROTOCOL_ETH_ALL) == TCS_SUCCESS);

    // Then
    CHECK(socket != TCS_SOCKET_INVALID);
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Create AVTP socket")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    // When
    CHECK(tcs_socket(&socket, TCS_FAMILY_PACKET, TCS_SOCK_DGRAM, 0x22F0) == TCS_SUCCESS);

    // Then
    CHECK(socket != TCS_SOCKET_INVALID);
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("AVTP Create talker socket sendto")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    // When
    CHECK(tcs_socket(&socket, TCS_FAMILY_PACKET, TCS_SOCK_DGRAM, 0x22F0) == TCS_SUCCESS);

    CHECK(tcs_opt_priority_set(socket, 6) == TCS_SUCCESS); // Set priority to 6 (VLAN priority 6)
    static struct TcsInterfaceAddress addrs[kAddressListCapacity];
    size_t addresses_found = 0;
    CHECK(tcs_address_list(0, TCS_FAMILY_PACKET, addrs, kAddressListCapacity, &addresses_found) == TCS_SUCCESS);
    CHECK(addresses_found > 0);

    TcsAddress address = TCS_ADDRESS_NONE;
    address.family = TCS_FAMILY_PACKET;
    address.data.packet.interface_id = addrs[0].iface.id;
    address.data.packet.protocol = 0x22F0;
    memcpy(address.data.packet.mac, AVTP_DEST_ADDR, sizeof(AVTP_DEST_ADDR));

    uint8_t msg[] = "hello world\n";
    size_t bytes_sent = 0;
    CHECK(tcs_send_to(socket, msg, sizeof(msg), TCS_FLAG_NONE, &address, &bytes_sent) == TCS_SUCCESS);
    CHECK(bytes_sent == sizeof(msg));
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Then

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TSN Create talker socket bind")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    // When
    CHECK(tcs_socket(&socket, TCS_FAMILY_PACKET, TCS_SOCK_RAW, TCS_PROTOCOL_ETH_ALL) == TCS_SUCCESS);

    CHECK(tcs_opt_priority_set(socket, 6) == TCS_SUCCESS); // Set priority to 6 (VLAN priority 6)
    static struct TcsInterfaceAddress addr[kAddressListCapacity];
    size_t addresses_found = 0;
    CHECK(tcs_address_list(0, TCS_FAMILY_PACKET, addr, kAddressListCapacity, &addresses_found) == TCS_SUCCESS);
    CHECK(addresses_found > 0);

    TcsAddress address = TCS_ADDRESS_NONE;
    address.family = TCS_FAMILY_PACKET;
    address.data.packet.interface_id = addr[0].iface.id;
    address.data.packet.protocol = 0x22F0; // Ethertype for AVTP
    memcpy(address.data.packet.mac, AVTP_DEST_ADDR, sizeof(AVTP_DEST_ADDR));

    CHECK(tcs_bind(socket, &address) == TCS_SUCCESS);

    // SOCK_RAW requires a complete Ethernet frame: [6 dst MAC][6 src MAC][2 EtherType][payload]
    uint8_t frame[64] = {0};
    memcpy(&frame[0], AVTP_DEST_ADDR, 6);                  // Destination MAC
    memcpy(&frame[6], addr[0].address.data.packet.mac, 6); // Source MAC
    frame[12] = 0x22;                                      // EtherType high byte (AVTP)
    frame[13] = 0xF0;                                      // EtherType low byte
    memcpy(&frame[14], "hello world\n", 12);               // Payload

    size_t bytes_sent = 0;
    CHECK(tcs_send_to(socket, frame, sizeof(frame), TCS_FLAG_NONE, &address, &bytes_sent) == TCS_SUCCESS);
    CHECK(bytes_sent == sizeof(frame));
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Then

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("TSN Create listener")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    // When
    CHECK(tcs_socket(&socket, TCS_FAMILY_PACKET, TCS_SOCK_RAW, TCS_PROTOCOL_ETH_ALL) == TCS_SUCCESS);

    static struct TcsInterfaceAddress addr[kAddressListCapacity];
    size_t addresses_found = 0;
    CHECK(tcs_address_list(0, TCS_FAMILY_PACKET, addr, kAddressListCapacity, &addresses_found) == TCS_SUCCESS);
    CHECK(addresses_found > 0);

    TcsAddress address = TCS_ADDRESS_NONE;
    address.family = TCS_FAMILY_PACKET;
    address.data.packet.interface_id = addr[0].iface.id;
    memcpy(address.data.packet.mac, AVTP_DEST_ADDR, sizeof(AVTP_DEST_ADDR));
    address.data.packet.protocol = 0x22F0; // Ethertype for TSN

    CHECK(tcs_bind(socket, &address) == TCS_SUCCESS);

    CHECK(tcs_opt_membership_add(socket, &address) == TCS_SUCCESS);

    // Then
    // tcs_receive...
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Create DGRAM packet socket with preset")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    // When
    CHECK(tcs_socket(&socket, TCS_FAMILY_PACKET, TCS_SOCK_DGRAM, TCS_PROTOCOL_ETH_ALL) == TCS_SUCCESS);

    // Then
    CHECK(socket != TCS_SOCKET_INVALID);
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_packet bind")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;
    static struct TcsInterfaceAddress addrs[kAddressListCapacity];
    size_t addresses_found = 0;
    CHECK(tcs_address_list(0, TCS_FAMILY_PACKET, addrs, kAddressListCapacity, &addresses_found) == TCS_SUCCESS);
    CHECK(addresses_found > 0);

    struct TcsAddress bind_address = TCS_ADDRESS_NONE;
    bind_address.family = TCS_FAMILY_PACKET;
    bind_address.data.packet.interface_id = addrs[0].iface.id;
    bind_address.data.packet.protocol = 0x22F0;

    // When
    CHECK(tcs_socket_packet(&socket, &bind_address, TCS_SOCK_DGRAM) == TCS_SUCCESS);

    // Then
    CHECK(socket != TCS_SOCKET_INVALID);
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_packet sendto")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;
    static struct TcsInterfaceAddress addrs[kAddressListCapacity];
    size_t addresses_found = 0;
    CHECK(tcs_address_list(0, TCS_FAMILY_PACKET, addrs, kAddressListCapacity, &addresses_found) == TCS_SUCCESS);
    CHECK(addresses_found > 0);

    struct TcsAddress bind_address = TCS_ADDRESS_NONE;
    bind_address.family = TCS_FAMILY_PACKET;
    bind_address.data.packet.interface_id = addrs[0].iface.id;
    bind_address.data.packet.protocol = 0x22F0;

    CHECK(tcs_socket_packet(&socket, &bind_address, TCS_SOCK_DGRAM) == TCS_SUCCESS);

    // When - sendto with explicit destination
    struct TcsAddress dest_address = TCS_ADDRESS_NONE;
    dest_address.family = TCS_FAMILY_PACKET;
    dest_address.data.packet.interface_id = addrs[0].iface.id;
    dest_address.data.packet.protocol = 0x22F0;
    memcpy(dest_address.data.packet.mac, AVTP_DEST_ADDR, sizeof(AVTP_DEST_ADDR));

    uint8_t msg[] = "hello world\n";
    size_t bytes_sent = 0;
    CHECK(tcs_send_to(socket, msg, sizeof(msg), TCS_FLAG_NONE, &dest_address, &bytes_sent) == TCS_SUCCESS);
    CHECK(bytes_sent == sizeof(msg));

    // Then
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_packet_str bind")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;
    static struct TcsInterface interfaces[kInterfaceListCapacity];
    size_t iface_count = 0;
    CHECK(tcs_interface_list(interfaces, kInterfaceListCapacity, &iface_count) == TCS_SUCCESS);
    CHECK(iface_count > 0);

    const char* iface_name = NULL;
    for (size_t i = 0; i < iface_count; ++i)
    {
        if (strcmp(interfaces[i].name, "lo") != 0)
        {
            iface_name = interfaces[i].name;
            break;
        }
    }
    if (iface_name == NULL)
    {
        REQUIRE(tcs_lib_free() == TCS_SUCCESS);
        return;
    }

    // When
    CHECK(tcs_socket_packet_str(&socket, iface_name, 0x22F0, TCS_SOCK_DGRAM) == TCS_SUCCESS);

    // Then
    CHECK(socket != TCS_SOCKET_INVALID);
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}
#endif

TEST_CASE("tcs_socket_packet invalid arguments old")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    // When/Then - NULL address should fail
    CHECK(tcs_socket_packet(&socket, NULL, TCS_SOCK_DGRAM) == TCS_ERROR_INVALID_ARGUMENT);

    // When/Then - NULL socket should fail
    CHECK(tcs_socket_packet(NULL, NULL, TCS_SOCK_DGRAM) == TCS_ERROR_INVALID_ARGUMENT);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

#if defined(__linux__)
TEST_CASE("tcs_socket_packet DGRAM")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    struct TcsAddress bind = TCS_ADDRESS_NONE;
    bind.family = TCS_FAMILY_PACKET;
    bind.data.packet.interface_id = 1; // lo
    bind.data.packet.protocol = 0x22F0;

    TcsSocket socket = TCS_SOCKET_INVALID;

    // When
    CHECK(tcs_socket_packet(&socket, &bind, TCS_SOCK_DGRAM) == TCS_SUCCESS);

    // Then
    CHECK(socket != TCS_SOCKET_INVALID);
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_packet RAW")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    struct TcsAddress bind = TCS_ADDRESS_NONE;
    bind.family = TCS_FAMILY_PACKET;
    bind.data.packet.interface_id = 1; // lo
    bind.data.packet.protocol = 0x22F0;

    TcsSocket socket = TCS_SOCKET_INVALID;

    // When
    CHECK(tcs_socket_packet(&socket, &bind, TCS_SOCK_RAW) == TCS_SUCCESS);

    // Then
    CHECK(socket != TCS_SOCKET_INVALID);
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_packet_str DGRAM")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    // When
    CHECK(tcs_socket_packet_str(&socket, "lo", 0x22F0, TCS_SOCK_DGRAM) == TCS_SUCCESS);

    // Then
    CHECK(socket != TCS_SOCKET_INVALID);
    CHECK(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_packet multicast add and drop str")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given - create and bind a DGRAM packet socket on lo
    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket_packet_str(&socket, "lo", 0x22F0, TCS_SOCK_DGRAM) == TCS_SUCCESS);

    // When/Then - join and leave a multicast MAC group via str
    CHECK(tcs_opt_membership_add_str(socket, "91:e0:f0:00:fe:00") == TCS_SUCCESS);
    CHECK(tcs_opt_membership_drop_str(socket, "91:e0:f0:00:fe:00") == TCS_SUCCESS);

    // Clean up
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}
#endif

TEST_CASE("tcs_socket_packet invalid arguments")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // NULL bind_address
    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket_packet(&socket, NULL, TCS_SOCK_DGRAM) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(socket == TCS_SOCKET_INVALID);

    // Wrong family
    struct TcsAddress bad = TCS_ADDRESS_NONE;
    bad.family = TCS_FAMILY_IP4;
    socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket_packet(&socket, &bad, TCS_SOCK_DGRAM) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(socket == TCS_SOCKET_INVALID);

    // Invalid type
    struct TcsAddress bind = TCS_ADDRESS_NONE;
    bind.family = TCS_FAMILY_PACKET;
    bind.data.packet.interface_id = 1;
    bind.data.packet.protocol = 0x22F0;
    socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket_packet(&socket, &bind, TcsSockType{9999}) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(socket == TCS_SOCKET_INVALID);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_socket_packet_str invalid interface")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    // When/Then
    CHECK(tcs_socket_packet_str(&socket, "nonexistent99", 0x22F0, TCS_SOCK_DGRAM) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(socket == TCS_SOCKET_INVALID);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Sentinel handling: TCS_FAMILY_PACKET on unsupported platform")
{
    // On platforms without AF_PACKET (Windows, macOS, BSDs), TCS_FAMILY_PACKET is a sentinel.
    // All entry points that take a family must reject it consistently with TCS_ERROR_NOT_SUPPORTED
    // instead of leaking the sentinel into OS calls.
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Probe whether this platform supports AF_PACKET by trying to create one.
    TcsSocket probe = TCS_SOCKET_INVALID;
    TcsResult probe_res = tcs_socket(&probe, TCS_FAMILY_PACKET, TCS_SOCK_RAW, TCS_PROTOCOL_ETH_ALL);

    if (probe_res == TCS_ERROR_NOT_SUPPORTED)
    {
        // Unsupported platform: verify all paths agree on the contract.
        CHECK(probe == TCS_SOCKET_INVALID);

        // tcs_address_resolve() must reject the sentinel filter before calling getaddrinfo()
        struct TcsAddress resolved = TCS_ADDRESS_NONE;
        size_t resolved_count = 0;
        CHECK(tcs_address_resolve("127.0.0.1", TCS_FAMILY_PACKET, &resolved, 1, &resolved_count) ==
              TCS_ERROR_NOT_SUPPORTED);

        // tcs_address_list() must skip the unsupported filter (no entries, no error)
        static struct TcsInterfaceAddress ifaddrs[kAddressListCapacity];
        size_t ifaddr_count = 0;
        CHECK(tcs_address_list(0, TCS_FAMILY_PACKET, ifaddrs, kAddressListCapacity, &ifaddr_count) == TCS_SUCCESS);
        CHECK(ifaddr_count == 0);

        // sockaddr2native (via tcs_bind) must reject a sentinel address
        TcsSocket udp_socket = TCS_SOCKET_INVALID;
        REQUIRE(tcs_socket(&udp_socket, TCS_FAMILY_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
        struct TcsAddress packet_addr = TCS_ADDRESS_NONE;
        packet_addr.family = TCS_FAMILY_PACKET;
        CHECK(tcs_bind(udp_socket, &packet_addr) == TCS_ERROR_NOT_SUPPORTED);
        CHECK(tcs_close(&udp_socket) == TCS_SUCCESS);
    }
    else if (probe_res == TCS_SUCCESS)
    {
        // Supported (Linux with CAP_NET_RAW): clean up and skip the rest.
        CHECK(tcs_close(&probe) == TCS_SUCCESS);
    }
    // Other results (e.g. TCS_ERROR_PERMISSION_DENIED on Linux without CAP_NET_RAW) are valid
    // platform-specific outcomes and not exercised by this test.

    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_address_socket_local on TCP")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    REQUIRE(tcs_socket(&listen_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    struct TcsAddress bind_address = TCS_ADDRESS_NONE;
    bind_address.family = TCS_FAMILY_IP4;
    bind_address.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    bind_address.data.ip4.port = 1260;
    CHECK(tcs_bind(listen_socket, &bind_address) == TCS_SUCCESS);
    REQUIRE(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    REQUIRE(tcs_connect_str(client_socket, "127.0.0.1", 1260) == TCS_SUCCESS);

    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);

    // When
    struct TcsAddress local_addr = TCS_ADDRESS_NONE;
    CHECK_POSIX(tcs_address_socket_local(client_socket, &local_addr) == TCS_SUCCESS);

    // Then
    CHECK_POSIX(local_addr.family.native == TCS_FAMILY_IP4.native);
    CHECK_POSIX(local_addr.data.ip4.address == TCS_ADDRESS_LOOPBACK_IP4);
    CHECK_POSIX(local_addr.data.ip4.port != 0);

    // Clean up
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_address_socket_remote on TCP")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    REQUIRE(tcs_socket(&listen_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    struct TcsAddress bind_address = TCS_ADDRESS_NONE;
    bind_address.family = TCS_FAMILY_IP4;
    bind_address.data.ip4.address = TCS_ADDRESS_LOOPBACK_IP4;
    bind_address.data.ip4.port = 1261;
    CHECK(tcs_bind(listen_socket, &bind_address) == TCS_SUCCESS);
    REQUIRE(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    REQUIRE(tcs_connect_str(client_socket, "127.0.0.1", 1261) == TCS_SUCCESS);

    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);

    // When
    struct TcsAddress remote_addr = TCS_ADDRESS_NONE;
    CHECK_POSIX(tcs_address_socket_remote(client_socket, &remote_addr) == TCS_SUCCESS);

    // Then
    CHECK_POSIX(remote_addr.family.native == TCS_FAMILY_IP4.native);
    CHECK_POSIX(remote_addr.data.ip4.address == TCS_ADDRESS_LOOPBACK_IP4);
    CHECK_POSIX(remote_addr.data.ip4.port == 1261);

    // Clean up
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_address_socket_local and remote with invalid args")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    struct TcsAddress addr = TCS_ADDRESS_NONE;
    CHECK(tcs_address_socket_local(TCS_SOCKET_INVALID, &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_socket_remote(TCS_SOCKET_INVALID, &addr) == TCS_ERROR_INVALID_ARGUMENT);

    TcsSocket socket = TCS_SOCKET_INVALID;
    REQUIRE(tcs_socket(&socket, TCS_FAMILY_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_address_socket_local(socket, NULL) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_socket_remote(socket, NULL) == TCS_ERROR_INVALID_ARGUMENT);

    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

// ######## IPv6 Tests ########

TEST_CASE("IPv6 socket preset TCP")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket, TCS_FAMILY_IP6, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(socket != TCS_SOCKET_INVALID);

    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("IPv6 socket preset UDP")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket, TCS_FAMILY_IP6, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(socket != TCS_SOCKET_INVALID);

    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("IPv6 Simple TCP Test")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    REQUIRE(tcs_socket(&listen_socket, TCS_FAMILY_IP6, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    REQUIRE(tcs_socket(&client_socket, TCS_FAMILY_IP6, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP6;
    local_address.data.ip6.address = TCS_ADDRESS_LOOPBACK_IP6;
    local_address.data.ip6.port = 1214;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    REQUIRE(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);

    struct TcsAddress connect_address = TCS_ADDRESS_NONE;
    connect_address.family = TCS_FAMILY_IP6;
    connect_address.data.ip6.address = TCS_ADDRESS_LOOPBACK_IP6;
    connect_address.data.ip6.port = 1214;
    REQUIRE(tcs_connect(client_socket, &connect_address) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    uint8_t recv_buffer[8] = {0};
    const uint8_t* send_buffer = (const uint8_t*)"IPv6test";
    CHECK(tcs_send(client_socket, send_buffer, 8, TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);
    CHECK(tcs_receive(accept_socket, recv_buffer, 8, TCS_MSG_WAITALL, NULL) == TCS_SUCCESS);

    CHECK(memcmp(recv_buffer, send_buffer, 8) == 0);

    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);
    CHECK(tcs_close(&accept_socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("IPv6 UDP Test")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket socket_recv = TCS_SOCKET_INVALID;
    TcsSocket socket_send = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket_recv, TCS_FAMILY_IP6, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_send, TCS_FAMILY_IP6, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_opt_receive_timeout_set(socket_recv, 5000) == TCS_SUCCESS);
    CHECK(tcs_opt_reuse_address_set(socket_recv, true) == TCS_SUCCESS);

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_FAMILY_IP6;
    local_address.data.ip6.address = TCS_ADDRESS_ANY_IP6;
    local_address.data.ip6.port = 1433;
    CHECK(tcs_bind(socket_recv, &local_address) == TCS_SUCCESS);

    uint8_t msg[] = "hello ipv6\n";
    size_t sent = 0;
    uint8_t recv_buffer[1024] = {0};
    size_t bytes_received = 0;

    struct TcsAddress dest = TCS_ADDRESS_NONE;
    dest.family = TCS_FAMILY_IP6;
    dest.data.ip6.address = TCS_ADDRESS_LOOPBACK_IP6;
    dest.data.ip6.port = 1433;

    CHECK(tcs_send_to(socket_send, msg, sizeof(msg), TCS_FLAG_NONE, &dest, &sent) == TCS_SUCCESS);
    CHECK(tcs_receive(socket_recv, recv_buffer, sizeof(recv_buffer), TCS_FLAG_NONE, &bytes_received) == TCS_SUCCESS);
    recv_buffer[bytes_received] = '\0';

    CHECK(sent > 0);
    CHECK(strcmp((const char*)recv_buffer, (const char*)msg) == 0);

    CHECK(tcs_close(&socket_recv) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_send) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("IPv6 address parse loopback")
{
    TcsAddress addr;
    CHECK(tcs_address_parse("::1", &addr) == TCS_SUCCESS);
    CHECK(addr.family.native == TCS_FAMILY_IP6.native);
    CHECK(addr.data.ip6.port == 0);
    CHECK(tcs_address_is_loopback(&addr));
}

TEST_CASE("IPv6 address parse all-zeros")
{
    TcsAddress addr;
    CHECK(tcs_address_parse("::", &addr) == TCS_SUCCESS);
    CHECK(addr.family.native == TCS_FAMILY_IP6.native);
    CHECK(tcs_address_is_any(&addr));
}

TEST_CASE("IPv6 address parse full form")
{
    TcsAddress addr;
    // RFC 4291 Form 1: x:x:x:x:x:x:x:x
    CHECK(tcs_address_parse("2001:0db8:85a3:0000:0000:8a2e:0370:7334", &addr) == TCS_SUCCESS);
    CHECK(addr.family.native == TCS_FAMILY_IP6.native);
    CHECK(addr.data.ip6.address.bytes[0] == 0x20);
    CHECK(addr.data.ip6.address.bytes[1] == 0x01);
    CHECK(addr.data.ip6.address.bytes[2] == 0x0d);
    CHECK(addr.data.ip6.address.bytes[3] == 0xb8);
    CHECK(addr.data.ip6.address.bytes[14] == 0x73);
    CHECK(addr.data.ip6.address.bytes[15] == 0x34);

    CHECK(tcs_address_parse("1:2:3:4:5:6:7:8", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.address.bytes[1] == 0x01);
    CHECK(addr.data.ip6.address.bytes[15] == 0x08);

    CHECK(tcs_address_parse("0:0:0:0:0:0:0:0", &addr) == TCS_SUCCESS);
    CHECK(tcs_address_is_any(&addr));
}

TEST_CASE("IPv6 address parse compressed")
{
    TcsAddress addr;
    CHECK(tcs_address_parse("fe80::1", &addr) == TCS_SUCCESS);
    CHECK(addr.family.native == TCS_FAMILY_IP6.native);
    CHECK(addr.data.ip6.address.bytes[0] == 0xFE);
    CHECK(addr.data.ip6.address.bytes[1] == 0x80);
    CHECK(addr.data.ip6.address.bytes[15] == 0x01);
    CHECK(tcs_address_is_link_local(&addr));

    CHECK(tcs_address_parse("2001:db8::1", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.address.bytes[0] == 0x20);
    CHECK(addr.data.ip6.address.bytes[1] == 0x01);
    CHECK(addr.data.ip6.address.bytes[15] == 0x01);

    CHECK(tcs_address_parse("1::", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.address.bytes[0] == 0x00);
    CHECK(addr.data.ip6.address.bytes[1] == 0x01);
    CHECK(addr.data.ip6.address.bytes[15] == 0x00);

    CHECK(tcs_address_parse("1::2", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.address.bytes[1] == 0x01);
    CHECK(addr.data.ip6.address.bytes[15] == 0x02);

    CHECK(tcs_address_parse("FFFF::1", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.address.bytes[0] == 0xFF);
    CHECK(addr.data.ip6.address.bytes[1] == 0xFF);
}

TEST_CASE("IPv6 address parse mixed IPv4 notation")
{
    // RFC 4291 Form 3: x:x:x:x:x:x:d.d.d.d
    TcsAddress addr;
    CHECK(tcs_address_parse("::ffff:192.168.1.1", &addr) == TCS_SUCCESS);
    CHECK(addr.family.native == TCS_FAMILY_IP6.native);
    CHECK(addr.data.ip6.address.bytes[10] == 0xFF);
    CHECK(addr.data.ip6.address.bytes[11] == 0xFF);
    CHECK(addr.data.ip6.address.bytes[12] == 192);
    CHECK(addr.data.ip6.address.bytes[13] == 168);
    CHECK(addr.data.ip6.address.bytes[14] == 1);
    CHECK(addr.data.ip6.address.bytes[15] == 1);

    CHECK(tcs_address_parse("::13.1.68.3", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.address.bytes[12] == 13);
    CHECK(addr.data.ip6.address.bytes[13] == 1);
    CHECK(addr.data.ip6.address.bytes[14] == 68);
    CHECK(addr.data.ip6.address.bytes[15] == 3);

    CHECK(tcs_address_parse("0:0:0:0:0:ffff:129.144.52.38", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.address.bytes[10] == 0xFF);
    CHECK(addr.data.ip6.address.bytes[11] == 0xFF);
    CHECK(addr.data.ip6.address.bytes[12] == 129);
    CHECK(addr.data.ip6.address.bytes[13] == 144);
    CHECK(addr.data.ip6.address.bytes[14] == 52);
    CHECK(addr.data.ip6.address.bytes[15] == 38);

    CHECK(tcs_address_parse("64:ff9b::192.168.1.1", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.address.bytes[0] == 0x00);
    CHECK(addr.data.ip6.address.bytes[1] == 0x64);
    CHECK(addr.data.ip6.address.bytes[2] == 0xFF);
    CHECK(addr.data.ip6.address.bytes[3] == 0x9B);
    CHECK(addr.data.ip6.address.bytes[12] == 192);
    CHECK(addr.data.ip6.address.bytes[15] == 1);

    // Invalid mixed notation
    CHECK(tcs_address_parse("::1.2.3.256", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("::1.2.3", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("::1.2.3.4.5", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("0:0:0:0:0:0:0:192.168.1.1", &addr) == TCS_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("IPv6 address parse with port")
{
    TcsAddress addr;
    CHECK(tcs_address_parse("[::1]:8080", &addr) == TCS_SUCCESS);
    CHECK(addr.family.native == TCS_FAMILY_IP6.native);
    CHECK(addr.data.ip6.port == 8080);
    CHECK(tcs_address_is_loopback(&addr));

    CHECK(tcs_address_parse("[::1]", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.port == 0);
    CHECK(tcs_address_is_loopback(&addr));

    CHECK(tcs_address_parse("[fe80::1%3]:8080", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.port == 8080);
    CHECK(addr.data.ip6.scope_id == 3);
    CHECK(tcs_address_is_link_local(&addr));

    CHECK(tcs_address_parse("[::1]:65535", &addr) == TCS_SUCCESS);
    CHECK(addr.data.ip6.port == 65535);

    CHECK(tcs_address_parse("[::1]:65536", &addr) == TCS_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("IPv6 address parse with scope id")
{
    TcsAddress addr;
    CHECK(tcs_address_parse("fe80::1%3", &addr) == TCS_SUCCESS);
    CHECK(addr.family.native == TCS_FAMILY_IP6.native);
    CHECK(addr.data.ip6.scope_id == 3);
    CHECK(tcs_address_is_link_local(&addr));
}

TEST_CASE("IPv6 address parse invalid")
{
    TcsAddress addr;
    CHECK(tcs_address_parse(":::", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("1:2:3:4:5:6:7:8:9", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("1::2::3", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("::1:2:3:4:5:6:7:8", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("1:2:3:4:5:6:7::8", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("::1:", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("fe80::1:", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("[::1]:", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("[::1]:99999", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("[::1]:8080abc", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("[::1]:999999", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("fe80::1%3abc", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("fe80::1%99999999999", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("[::1", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("[]", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("[::1]garbage", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("fe80::1%", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("10000::1", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("g::1", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    // RFC 3986: port = *DIGIT, strictly decimal
    CHECK(tcs_address_parse("[::1]:0x50", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("[::1]:+80", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("[::1]:-1", &addr) == TCS_ERROR_INVALID_ARGUMENT);
    CHECK(tcs_address_parse("[::1]: 80", &addr) == TCS_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("IPv6 address to string loopback")
{
    TcsAddress addr = TCS_ADDRESS_NONE;
    addr.family = TCS_FAMILY_IP6;
    addr.data.ip6.address = TCS_ADDRESS_LOOPBACK_IP6;
    char str[70];
    CHECK(tcs_address_to_str(&addr, str) == TCS_SUCCESS);
    CHECK_EQ(str, "::1");
}

TEST_CASE("IPv6 address to string all-zeros")
{
    TcsAddress addr = TCS_ADDRESS_NONE;
    addr.family = TCS_FAMILY_IP6;
    addr.data.ip6.address = TCS_ADDRESS_ANY_IP6;
    char str[70];
    CHECK(tcs_address_to_str(&addr, str) == TCS_SUCCESS);
    CHECK_EQ(str, "::");
}

TEST_CASE("IPv6 address to string with port")
{
    TcsAddress addr = TCS_ADDRESS_NONE;
    addr.family = TCS_FAMILY_IP6;
    addr.data.ip6.address = TCS_ADDRESS_LOOPBACK_IP6;
    addr.data.ip6.port = 8080;
    char str[70];
    CHECK(tcs_address_to_str(&addr, str) == TCS_SUCCESS);
    CHECK_EQ(str, "[::1]:8080");
}

TEST_CASE("IPv6 address to string RFC 5952 compliance")
{
    TcsAddress addr;
    char str[70];

    // Leading zeros suppressed
    CHECK(tcs_address_parse("2001:0db8::0001", &addr) == TCS_SUCCESS);
    CHECK(tcs_address_to_str(&addr, str) == TCS_SUCCESS);
    CHECK_EQ(str, "2001:db8::1");

    // Single zero group NOT compressed to ::
    CHECK(tcs_address_parse("2001:db8:0:1:1:1:1:1", &addr) == TCS_SUCCESS);
    CHECK(tcs_address_to_str(&addr, str) == TCS_SUCCESS);
    CHECK_EQ(str, "2001:db8:0:1:1:1:1:1");

    // Longest zero run compressed, first run wins on tie
    CHECK(tcs_address_parse("1:0:0:2:0:0:0:3", &addr) == TCS_SUCCESS);
    CHECK(tcs_address_to_str(&addr, str) == TCS_SUCCESS);
    CHECK_EQ(str, "1:0:0:2::3");

    // Equal length runs: first wins
    CHECK(tcs_address_parse("1:0:0:2:3:0:0:4", &addr) == TCS_SUCCESS);
    CHECK(tcs_address_to_str(&addr, str) == TCS_SUCCESS);
    CHECK_EQ(str, "1::2:3:0:0:4");

    // Lowercase hex
    CHECK(tcs_address_parse("ABCD:EF01::1", &addr) == TCS_SUCCESS);
    CHECK(tcs_address_to_str(&addr, str) == TCS_SUCCESS);
    CHECK_EQ(str, "abcd:ef01::1");
}

TEST_CASE("IPv6 address roundtrip")
{
    const char* inputs[] = {"::1", "fe80::1", "2001:db8::1", "::", "ff02::1", "1:2:3:4:5:6:7:8", "1::2"};
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++)
    {
        TcsAddress addr;
        CHECK(tcs_address_parse(inputs[i], &addr) == TCS_SUCCESS);
        char str[70];
        CHECK(tcs_address_to_str(&addr, str) == TCS_SUCCESS);
        CHECK_EQ(str, inputs[i]);
    }
}

TEST_CASE("IPv6 address utility functions")
{
    TcsAddress loopback = TCS_ADDRESS_NONE;
    loopback.family = TCS_FAMILY_IP6;
    loopback.data.ip6.address = TCS_ADDRESS_LOOPBACK_IP6;
    CHECK(tcs_address_is_supported(&loopback));
    CHECK(tcs_address_is_loopback(&loopback));
    CHECK_FALSE(tcs_address_is_any(&loopback));
    CHECK_FALSE(tcs_address_is_multicast(&loopback));

    TcsAddress any = TCS_ADDRESS_NONE;
    any.family = TCS_FAMILY_IP6;
    any.data.ip6.address = TCS_ADDRESS_ANY_IP6;
    CHECK(tcs_address_is_supported(&any));
    CHECK(tcs_address_is_any(&any));
    CHECK_FALSE(tcs_address_is_loopback(&any));

    TcsAddress multicast;
    tcs_address_parse("ff02::1", &multicast);
    CHECK(tcs_address_is_supported(&multicast));
    CHECK(tcs_address_is_multicast(&multicast));

    TcsAddress link_local;
    tcs_address_parse("fe80::1", &link_local);
    CHECK(tcs_address_is_supported(&link_local));
    CHECK(tcs_address_is_link_local(&link_local));

    TcsAddress unsupported = TCS_ADDRESS_NONE;
    unsupported.family.native = -12345;
    CHECK_FALSE(tcs_address_is_supported(&unsupported));
    CHECK_FALSE(tcs_address_is_loopback(&unsupported));
    CHECK_FALSE(tcs_address_is_supported(NULL));
}

TEST_CASE("IPv6 address resolve loopback")
{
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    struct TcsAddress addresses[4];
    size_t count = 0;
    CHECK(tcs_address_resolve("::1", TCS_FAMILY_IP6, addresses, 4, &count) == TCS_SUCCESS);
    CHECK(count >= 1);
    CHECK(addresses[0].family.native == TCS_FAMILY_IP6.native);
    CHECK(tcs_address_is_loopback(&addresses[0]));

    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("IPv6 address is_equal")
{
    TcsAddress a = TCS_ADDRESS_NONE;
    a.family = TCS_FAMILY_IP6;
    a.data.ip6.address = TCS_ADDRESS_LOOPBACK_IP6;
    TcsAddress b = TCS_ADDRESS_NONE;
    b.family = TCS_FAMILY_IP6;
    b.data.ip6.address = TCS_ADDRESS_LOOPBACK_IP6;
    CHECK(tcs_address_is_equal(&a, &b));

    b.data.ip6.port = 80;
    CHECK_FALSE(tcs_address_is_equal(&a, &b));
}
