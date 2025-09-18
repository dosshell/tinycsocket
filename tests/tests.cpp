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

#include "tinycsocket.h"
#include "tinydatastructures.h"

#include <cstring>
#include <thread>

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

    TcsSocket client_socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket_preset(&client_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_connect_str(client_socket, "example.com", 80) == TCS_SUCCESS);

    uint8_t send_buffer[] = "GET / HTTP/1.1\nHost: example.com\n\n";
    CHECK(tcs_send(client_socket, send_buffer, sizeof(send_buffer), TCS_MSG_SENDALL, NULL) == TCS_SUCCESS);

    static uint8_t recv_buffer[8192] = {0};
    size_t bytes_received = 0;
    CHECK(tcs_receive(client_socket, recv_buffer, 8192, TCS_FLAG_NONE, &bytes_received) == TCS_SUCCESS);
    CHECK(tcs_shutdown(client_socket, TCS_SD_BOTH) == TCS_SUCCESS);
    CHECK(tcs_close(&client_socket) == TCS_SUCCESS);

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
    TcsSocket socket = TCS_SOCKET_INVALID;
    int pre_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // When
    TcsResult sts = tcs_socket_preset(&socket, TCS_PRESET_UDP_IP4);
    int post_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // Then
    CHECK(sts == TCS_SUCCESS);
    CHECK(socket != TCS_SOCKET_INVALID);
    CHECK(post_mem_diff == pre_mem_diff);

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
    CHECK(tcs_socket(&socket_recv, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_send, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_opt_receive_timeout_set(socket_recv, 5000) == TCS_SUCCESS);
    CHECK(tcs_opt_reuse_address_set(socket_recv, true) == TCS_SUCCESS);

    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_AF_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1432;

    CHECK(tcs_bind(socket_recv, &local_address) == TCS_SUCCESS);

    uint8_t msg[] = "hello world\n";
    size_t sent = 0;
    uint8_t recv_buffer[1024] = {0};
    size_t recv_size = sizeof(recv_buffer) - sizeof('\0');
    size_t bytes_received = 0;

    TcsAddress address;
    address.family = TCS_AF_IP4;
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
    CHECK(tcs_socket_preset(&socket, TCS_PRESET_UDP_IP4) == TCS_SUCCESS);

    // When
    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_AF_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1465;

    TcsResult sts = tcs_bind(socket, &local_address);

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
    TcsSocket listen_socket = TCS_SOCKET_INVALID;
    TcsSocket accept_socket = TCS_SOCKET_INVALID;
    TcsSocket client_socket = TCS_SOCKET_INVALID;

    CHECK(tcs_socket_preset(&listen_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_socket_preset(&client_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_AF_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1212;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    CHECK(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    CHECK(tcs_connect_str(client_socket, "localhost", 1212) == TCS_SUCCESS);

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

    CHECK(tcs_socket_preset(&listen_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_socket_preset(&client_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_AF_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1212;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    CHECK(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    CHECK(tcs_connect_str(client_socket, "localhost", 1212) == TCS_SUCCESS);

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

    CHECK(tcs_opt_receive_timeout_set(server_socket, 10) == TCS_SUCCESS);
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

    CHECK(tcs_socket_preset(&listen_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_socket_preset(&client_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_AF_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1212;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    CHECK(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    CHECK(tcs_connect_str(client_socket, "localhost", 1212) == TCS_SUCCESS);

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

    CHECK(tcs_socket_preset(&listen_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_socket_preset(&client_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_AF_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1212;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    CHECK(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    CHECK(tcs_connect_str(client_socket, "localhost", 1212) == TCS_SUCCESS);

    CHECK(tcs_accept(listen_socket, &accept_socket, NULL) == TCS_SUCCESS);
    CHECK(tcs_close(&listen_socket) == TCS_SUCCESS);

    // When
    TcsBuffer send_buffers[3];
    send_buffers[0].data = (uint8_t*)"12345678";
    send_buffers[0].length = 8;
    send_buffers[1].data = (uint8_t*)"ABCDEFGH";
    send_buffers[1].length = 8;
    send_buffers[2].data = (uint8_t*)"abcdefgh";
    send_buffers[2].length = 8;
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

    CHECK(tcs_socket_preset(&listen_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);
    CHECK(tcs_socket_preset(&client_socket, TCS_PRESET_TCP_IP4) == TCS_SUCCESS);

    CHECK(tcs_opt_reuse_address_set(listen_socket, true) == TCS_SUCCESS);
    TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_AF_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1212;
    CHECK(tcs_bind(listen_socket, &local_address) == TCS_SUCCESS);
    CHECK(tcs_listen(listen_socket, TCS_BACKLOG_MAX) == TCS_SUCCESS);
    CHECK(tcs_connect_str(client_socket, "localhost", 1212) == TCS_SUCCESS);

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

// TODO(markusl): Broken on Windows (use nonblocking behind the curton?)
#ifdef CROSS_ISSUE
TEST_CASE("shutdown")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket peer1 = TCS_NULLSOCKET;
    TcsSocket peer2 = TCS_NULLSOCKET;

    CHECK(tcs_create(&peer1, TCS_PRESET_UDP_IP4) == TCS_SUCCESS);
    CHECK(tcs_create(&peer2, TCS_PRESET_UDP_IP4) == TCS_SUCCESS);

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
        tcs_close(&peer2);
    });

    uint8_t buffer1[1024];
    tcs_receive(peer1, buffer1, 1024, TCS_NO_FLAGS, NULL);
    tcs_shutdown(peer2, TCS_SD_RECEIVE);
    tcs_close(&peer1);

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
    CHECK(tcs_pool_destroy(&pool) == TCS_SUCCESS);
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
    TcsSocket socket = TCS_SOCKET_INVALID;

    CHECK(tcs_socket_preset(&socket, TCS_PRESET_UDP_IP4) == TCS_SUCCESS);

    TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_AF_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 1212;
    CHECK(tcs_bind(socket, &local_address) == TCS_SUCCESS);

    int user_data = 1337;
    struct TcsPool* pool = NULL;
    CHECK(tcs_pool_create(&pool) == TCS_SUCCESS);
    CHECK(tcs_pool_add(pool, socket, (void*)&user_data, false, true, false) == TCS_SUCCESS);

    // When
    size_t populated = 0;
    TcsPollEvent ev = TCS_POOL_EVENT_EMPTY;
    CHECK(tcs_pool_poll(pool, &ev, 1, &populated, 5000) == TCS_SUCCESS);
    CHECK(tcs_pool_destroy(&pool) == TCS_SUCCESS);
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
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    tcs_lib_free();
}

TEST_CASE("tcs_pool_poll simple read")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    CHECK(tcs_socket_preset(&socket, TCS_PRESET_UDP_IP4) == TCS_SUCCESS);
    int allocation_diff_before = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;
    struct TcsAddress local_address = TCS_ADDRESS_NONE;
    local_address.family = TCS_AF_IP4;
    local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
    local_address.data.ip4.port = 5679;
    CHECK(tcs_bind(socket, &local_address) == TCS_SUCCESS);
    int user_data = 1337;
    struct TcsPool* pool = NULL;
    CHECK(tcs_pool_create(&pool) == TCS_SUCCESS);
    CHECK(tcs_pool_add(pool, socket, (void*)&user_data, true, false, false) == TCS_SUCCESS);

    // When
    size_t populated = 0;
    TcsPollEvent ev = TCS_POOL_EVENT_EMPTY;
    CHECK(tcs_pool_poll(pool, &ev, 1, &populated, 0) == TCS_ERROR_TIMED_OUT);

    // Then
    CHECK(populated == 0);

    // When
    TcsAddress receiver = TCS_ADDRESS_NONE;
    CHECK(tcs_address_parse("127.0.0.1:5679", &receiver) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket, (const uint8_t*)"hej", 4, TCS_FLAG_NONE, &receiver, NULL) == TCS_SUCCESS);
    CHECK(tcs_pool_poll(pool, &ev, 1, &populated, 10) == TCS_SUCCESS);
    CHECK(tcs_pool_destroy(&pool) == TCS_SUCCESS);
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
    CHECK(tcs_close(&socket) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("tcs_pool_poll partial")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    struct TcsPool* pool = NULL;
    CHECK(tcs_pool_create(&pool) == TCS_SUCCESS);

    const int SOCKET_COUNT = 3;
    TcsSocket socket[SOCKET_COUNT];
    int user_data[SOCKET_COUNT];

    for (int i = 0; i < SOCKET_COUNT; ++i)
    {
        socket[i] = TCS_SOCKET_INVALID;
        CHECK(tcs_socket_preset(&socket[i], TCS_PRESET_UDP_IP4) == TCS_SUCCESS);

        struct TcsAddress local_address = TCS_ADDRESS_NONE;
        local_address.family = TCS_AF_IP4;
        local_address.data.ip4.address = TCS_ADDRESS_ANY_IP4;
        local_address.data.ip4.port = (uint16_t)(5000 + i);
        CHECK(tcs_bind(socket[i], &local_address) == TCS_SUCCESS);
        user_data[i] = 5000 + i;
        CHECK(tcs_pool_add(pool, socket[i], (void*)&user_data[i], true, false, true) == TCS_SUCCESS);
    }

    // When
    size_t populated = 0;
    TcsPollEvent ev = TCS_POOL_EVENT_EMPTY;
    CHECK(tcs_pool_poll(pool, &ev, 1, &populated, 0) == TCS_ERROR_TIMED_OUT);

    // Then
    CHECK(populated == 0);

    // When
    TcsAddress destinaion_address = TCS_ADDRESS_NONE;
    CHECK(tcs_address_parse("127.0.0.1:5001", &destinaion_address) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket[0], (const uint8_t*)"hej", 4, TCS_FLAG_NONE, &destinaion_address, NULL) == TCS_SUCCESS);
    CHECK(tcs_pool_poll(pool, &ev, 1, &populated, 10) == TCS_SUCCESS);
    CHECK(populated == 1);
    CHECK(*(int*)ev.user_data == 5001);
    CHECK(ev.can_read == true);

    // Then

    // Check that the event and socket is still there in the pool, even after poll
    CHECK(tcs_pool_poll(pool, &ev, 1, &populated, 10) == TCS_SUCCESS);
    CHECK(populated == 1);
    CHECK(*(int*)ev.user_data == 5001);
    CHECK(ev.can_read == true);

    uint8_t receive_buffer[1024];
    size_t received_bytes = 0;
    CHECK(tcs_receive(socket[1], receive_buffer, 1024, TCS_FLAG_NONE, &received_bytes) == TCS_SUCCESS);

    // Check that the event is removed after received data
    CHECK(tcs_pool_poll(pool, &ev, 1, &populated, 10) == TCS_ERROR_TIMED_OUT);

    CHECK(populated == 0);

    // Clean up
    CHECK(tcs_pool_destroy(&pool) == TCS_SUCCESS);
    for (int i = 0; i < SOCKET_COUNT; ++i)
    {
        CHECK(tcs_close(&socket[i]) == TCS_SUCCESS);
    }
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Address information count")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    size_t no_of_found_addresses = 0;

    // When
    CHECK(tcs_address_resolve("localhost", TCS_AF_IP4, NULL, 0, &no_of_found_addresses) == TCS_SUCCESS);

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
    struct TcsInterface interfaces[8];
    size_t no_of_found_interfaces = 0;

    // When
    CHECK(tcs_interface_list(interfaces, 8, &no_of_found_interfaces) == TCS_SUCCESS);

    // Then
    CHECK(no_of_found_interfaces > 0);
    for (size_t i = 0; i < no_of_found_interfaces; ++i)
    {
        printf("Interface %u: %s\n", interfaces[i].id, interfaces[i].name);

        struct TcsInterfaceAddress addresses[8];
        size_t found_addresses = 0;
        tcs_address_list(interfaces[i].id, TCS_AF_ANY, addresses, 8, &found_addresses);
        for (size_t j = 0; j < found_addresses; ++j)
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
    struct TcsInterfaceAddress ifaddrs[8];
    bool found_loopback = false;
    int pre_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // When
    WARN(tcs_address_list(0, TCS_AF_ANY, ifaddrs, 8, &ifaddrs_count) == TCS_SUCCESS);
    // find IPv4 loopback
    for (size_t i = 0; i < ifaddrs_count; ++i)
    {
        if (ifaddrs[i].address.family == TCS_AF_IP4 && ifaddrs[i].address.data.ip4.address == TCS_ADDRESS_LOOPBACK_IP4)
        {
            char out_str[40];
            tcs_address_to_str(&ifaddrs[i].address, out_str);
            printf("Interface loopback: %u; %s; %s\n", ifaddrs[i].iface.id, ifaddrs[i].iface.name, out_str);
            found_loopback = true;
        }
    }

    int post_mem_diff = MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER;

    // Then
    WARN(ifaddrs_count > 0);
    WARN(found_loopback);
    CHECK(post_mem_diff == pre_mem_diff);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS); // We are in C++, we should use defer
}

TEST_CASE("tcs_util_address_to_string with only IPv4")
{
    // Given
    TcsAddress addr;
    addr.family = TCS_AF_IP4;
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
    addr.family = TCS_AF_IP4;
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
    CHECK(address.family == TCS_AF_IP4);
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
    CHECK(address.family == TCS_AF_IP4);
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
    CHECK(tcs_socket(&socket_false, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_true, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_fail, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);

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

    CHECK(tcs_socket(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_socket(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_socket(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_socket(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_socket(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_socket(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

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

    CHECK(tcs_socket(&socket_false, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_true, TCS_AF_IP4, TCS_SOCK_STREAM, TCS_PROTOCOL_IP_TCP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_fail, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);

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
    TcsAddress address_any = {TCS_AF_IP4, {{0, 0}}};
    address_any.data.ip4.port = 1901;

    TcsAddress multicast_address = TCS_ADDRESS_NONE;
    tcs_address_parse("239.255.255.251:1901", &multicast_address);

    TcsSocket socket = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket, TCS_AF_IP4, TCS_SOCK_DGRAM, 0) == TCS_SUCCESS);
    CHECK(tcs_opt_reuse_address_set(socket, true) == TCS_SUCCESS);
    CHECK(tcs_opt_receive_timeout_set(socket, 5000) == TCS_SUCCESS);

    CHECK(tcs_bind(socket, &address_any) == TCS_SUCCESS);
    uint8_t msg[] = "hello world\n";

    // When
    CHECK(tcs_opt_membership_add_to(socket, &address_any, &multicast_address) == TCS_SUCCESS);

    uint8_t c;
    size_t a = sizeof(c);
    CHECK(tcs_opt_get(socket, TCS_SOL_IP, TCS_SO_IP_MULTICAST_LOOP, &c, &a) == TCS_SUCCESS);

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

TEST_CASE("Multicast Add-Drop-Add Membership")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket_send = TCS_SOCKET_INVALID;
    TcsSocket socket_recv = TCS_SOCKET_INVALID;
    CHECK(tcs_socket(&socket_send, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    CHECK(tcs_socket(&socket_recv, TCS_AF_IP4, TCS_SOCK_DGRAM, TCS_PROTOCOL_IP_UDP) == TCS_SUCCESS);
    tcs_opt_receive_timeout_set(socket_recv, 1000);
    CHECK(tcs_opt_reuse_address_set(socket_recv, true) == TCS_SUCCESS);

    TcsAddress address_any = {TCS_AF_IP4, {{0, 0}}};
    address_any.data.ip4.port = 1901;

    TcsAddress multicast_address = TCS_ADDRESS_NONE;
    CHECK(tcs_address_parse("239.255.255.251:1901", &multicast_address) == TCS_SUCCESS);
    CHECK(tcs_bind(socket_recv, &address_any) == TCS_SUCCESS);
    uint8_t msg_1[] = "hello world1\n";
    uint8_t msg_2[] = "hello world2\n";
    uint8_t msg_missed[] = "you can not read me\n";

    // When
    CHECK(tcs_opt_membership_add_to(socket_recv, &address_any, &multicast_address) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket_send, msg_1, sizeof(msg_1), 0, &multicast_address, NULL) == TCS_SUCCESS);
    CHECK(tcs_opt_membership_drop_from(socket_recv, &address_any, &multicast_address) == TCS_SUCCESS);
    CHECK(tcs_send_to(socket_send, msg_missed, sizeof(msg_missed), 0, &multicast_address, NULL) == TCS_SUCCESS);
    CHECK(tcs_opt_membership_add_to(socket_recv, &address_any, &multicast_address) == TCS_SUCCESS);
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

    CHECK(strcmp(reinterpret_cast<const char*>(recv_buffer_1), reinterpret_cast<const char*>(msg_1)) == 0);
    CHECK(strcmp(reinterpret_cast<const char*>(recv_buffer_2), reinterpret_cast<const char*>(msg_2)) == 0);

    // Clean up
    CHECK(tcs_close(&socket_recv) == TCS_SUCCESS);
    CHECK(tcs_close(&socket_send) == TCS_SUCCESS);
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

TEST_CASE("Simple AVTP talker")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    TcsSocket socket = TCS_SOCKET_INVALID;
    WARN(tcs_socket_preset(&socket, TCS_PRESET_PACKET) == TCS_SUCCESS);
    TcsAddress address = TCS_ADDRESS_NONE;
    tcs_address_parse("", &address);

    WARN(tcs_close(&socket) == TCS_SUCCESS);

    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}

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

TDS_ULIST_IMPL(int, int);

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

const uint8_t AVTP_DEST_ADDR[6] = {0x91, 0xe0, 0xf0, 0x00, 0xfe, 0x00};

TEST_CASE("Create packet socket")
{
    // Setup
    REQUIRE(tcs_lib_init() == TCS_SUCCESS);

    // Given
    TcsSocket socket = TCS_SOCKET_INVALID;

    // When
    WARN(tcs_socket_preset(&socket, TCS_PRESET_PACKET) == TCS_SUCCESS);

    // Then
    WARN(socket != TCS_SOCKET_INVALID);
    WARN(tcs_close(&socket) == TCS_SUCCESS);

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
    WARN(tcs_socket(&socket, TCS_AF_PACKET, TCS_SOCK_DGRAM, 0x22F0) == TCS_SUCCESS);

    // Then
    WARN(socket != TCS_SOCKET_INVALID);
    WARN(tcs_close(&socket) == TCS_SUCCESS);

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
    WARN(tcs_socket(&socket, TCS_AF_PACKET, TCS_SOCK_DGRAM, 0x22F0) == TCS_SUCCESS);

    WARN(tcs_opt_priority_set(socket, 6) == TCS_SUCCESS); // Set priority to 6 (VLAN priority 6)
    struct TcsInterfaceAddress addrs[8];
    size_t addresses_found = 0;
    WARN(tcs_address_list(0, TCS_AF_PACKET, addrs, 8, &addresses_found) == TCS_SUCCESS);
    WARN(addresses_found > 0);

    TcsAddress address = TCS_ADDRESS_NONE;
    address.family = TCS_AF_PACKET;
    address.data.packet.interface_id = addrs[0].iface.id;
    address.data.packet.protocol = 0x22F0;
    memcpy(address.data.packet.mac, AVTP_DEST_ADDR, sizeof(AVTP_DEST_ADDR));

    uint8_t msg[] = "hello world\n";
    size_t bytes_sent = 0;
    WARN(tcs_send_to(socket, msg, sizeof(msg), TCS_MSG_SENDALL, &address, &bytes_sent) == TCS_SUCCESS);
    WARN(bytes_sent == sizeof(msg));
    WARN(tcs_close(&socket) == TCS_SUCCESS);

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
    WARN(tcs_socket_preset(&socket, TCS_PRESET_PACKET) == TCS_SUCCESS);

    WARN(tcs_opt_priority_set(socket, 6) == TCS_SUCCESS); // Set priority to 6 (VLAN priority 6)
    struct TcsInterfaceAddress addr[8];
    size_t addresses_found = 0;
    WARN(tcs_address_list(0, TCS_AF_PACKET, addr, 8, &addresses_found) == TCS_SUCCESS);
    WARN(addresses_found > 0);

    TcsAddress address = TCS_ADDRESS_NONE;
    address.family = TCS_AF_PACKET;
    address.data.packet.interface_id = addr[0].iface.id;
    address.data.packet.protocol = 0x22F0; // Ethertype for AVTP
    memcpy(address.data.packet.mac, AVTP_DEST_ADDR, sizeof(AVTP_DEST_ADDR));

    WARN(tcs_bind(socket, &address) == TCS_SUCCESS);

    uint8_t msg[] = "hello world\n";
    size_t bytes_sent = 0;
    WARN(tcs_send(socket, msg, sizeof(msg), TCS_FLAG_NONE, &bytes_sent) == TCS_SUCCESS);
    WARN(bytes_sent == sizeof(msg));
    WARN(tcs_close(&socket) == TCS_SUCCESS);

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
    WARN(tcs_socket_preset(&socket, TCS_PRESET_PACKET) == TCS_SUCCESS);

    struct TcsInterfaceAddress addr[8];
    size_t addresses_found = 0;
    WARN(tcs_address_list(0, TCS_AF_PACKET, addr, 8, &addresses_found) == TCS_SUCCESS);
    WARN(addresses_found > 0);

    TcsAddress address = TCS_ADDRESS_NONE;
    address.family = TCS_AF_PACKET;
    address.data.packet.interface_id = addr[0].iface.id;
    memcpy(address.data.packet.mac, AVTP_DEST_ADDR, sizeof(AVTP_DEST_ADDR));
    address.data.packet.protocol = 0x22F0; // Ethertype for TSN

    WARN(tcs_bind(socket, &address) == TCS_SUCCESS);

    WARN(tcs_opt_membership_add(socket, &address) == TCS_SUCCESS);

    // Then
    // tcs_receive...
    WARN(tcs_close(&socket) == TCS_SUCCESS);

    // Clean up
    REQUIRE(tcs_lib_free() == TCS_SUCCESS);
}
