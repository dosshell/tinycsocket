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
#include <thread>

TEST_CASE("init test")
{
    CHECK(tcs_lib_init() == TINYCSOCKET_SUCCESS);
    CHECK(tcs_lib_free() == TINYCSOCKET_SUCCESS);
}

TEST_CASE("UDP test")
{
  CHECK(tcs_lib_init() == TINYCSOCKET_SUCCESS);

  struct tcs_addrinfo* address_info = NULL;

  struct tcs_addrinfo hints = { 0 };
  hints.ai_family = TINYCSOCKET_AF_INET;
  hints.ai_socktype = TINYCSOCKET_SOCK_DGRAM;

  tcs_getaddrinfo("localhost", "1212", &hints, &address_info);

  // Setup UDP reciever
  CHECK(tcs_lib_init() == TINYCSOCKET_SUCCESS);
  tcs_socket recv_soc = TINYCSOCKET_NULLSOCKET;

  bool didBind = false;
  for (struct tcs_addrinfo* address_iterator = address_info; address_iterator != NULL; address_iterator = address_iterator->ai_next)
  {
    if (tcs_create(&recv_soc, address_iterator->ai_family, address_iterator->ai_socktype, address_iterator->ai_protocol) != TINYCSOCKET_SUCCESS)
      continue;

    if (tcs_bind(recv_soc, address_iterator->ai_addr, address_iterator->ai_addrlen) != TINYCSOCKET_SUCCESS)
      continue;

    didBind = true;
    break;
  }

  CHECK(didBind);

  struct tcs_sockaddr remote_address = { 0 };
  size_t remote_address_size = sizeof(remote_address);
  uint8_t recv_buffer[1024] = { 0 };
  size_t bytes_recieved = 0;

  std::thread recv_thread([&]() {
    CHECK(tcs_recvfrom(recv_soc, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, &remote_address, &remote_address_size, &bytes_recieved) == TINYCSOCKET_SUCCESS);
    recv_buffer[bytes_recieved] = '\0';
  });

  // Setup UDP sender
  tcs_socket socket = TINYCSOCKET_NULLSOCKET;

  bool didConnect = false;
  for (struct tcs_addrinfo* address_iterator = address_info; address_iterator != NULL; address_iterator = address_iterator->ai_next)
  {
    if (tcs_create(&socket, address_iterator->ai_family, address_iterator->ai_socktype, address_iterator->ai_protocol) != TINYCSOCKET_SUCCESS)
      continue;

    if (tcs_connect(socket, address_iterator->ai_addr, address_iterator->ai_addrlen) != TINYCSOCKET_SUCCESS)
      continue;

    didConnect = true;
    break;
  }

  CHECK(didConnect);

  // Send message
  uint8_t msg[] = "hello world\n";
  CHECK(tcs_send(socket, msg, sizeof(msg), 0, NULL) == TINYCSOCKET_SUCCESS);

  // Join threads and check if we did send the message correctly
  recv_thread.join();
  CHECK(strcmp(reinterpret_cast<const char*>(recv_buffer), reinterpret_cast<const char*>(msg)) == 0);
  CHECK(tcs_freeaddrinfo(&address_info) == TINYCSOCKET_SUCCESS);
  CHECK(tcs_close(&recv_soc) == TINYCSOCKET_SUCCESS);
  CHECK(tcs_lib_free() == TINYCSOCKET_SUCCESS);
}
