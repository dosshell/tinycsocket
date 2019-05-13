#include "tinycsocket.h"
#include <stdbool.h>
#include <stdio.h> //sprintf



int tcs_simple_connect(tcs_socket* socket_ctx, const char* hostname, const char* port, int domain, int type)
{
  if (socket_ctx == NULL || *socket_ctx != TINYCSOCKET_NULLSOCKET)
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

  int protocol = -1;
  if (type == TINYCSOCKET_SOCK_STREAM)
  {
    protocol = TINYCSOCKET_IPPROTO_TCP;
  }
  else if (type == TINYCSOCKET_SOCK_DGRAM)
  {
    protocol = TINYCSOCKET_IPPROTO_UDP;
  }
  else
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  int sts = TINYCSOCKET_SUCCESS;

  sts = tcs_create(socket_ctx, domain, type, protocol);
  if (sts != TINYCSOCKET_SUCCESS)
  {
    return sts;
  }

  struct tcs_addrinfo* address_info = NULL;
  sts = tcs_getaddrinfo(hostname, port, NULL, &address_info);
  if (sts != TINYCSOCKET_SUCCESS)
  {
    return sts;
  }

  bool is_connected = false;
  for (struct tcs_addrinfo* address_iterator = address_info; address_iterator != NULL; address_iterator = address_iterator->ai_next)
  {
    if (tcs_connect(*socket_ctx, address_iterator->ai_addr, address_iterator->ai_addrlen) == TINYCSOCKET_SUCCESS)
    {
      is_connected = true;
      break;
    }
  }

  tcs_freeaddrinfo(&address_info);

  if (!is_connected)
  {
    return TINYCSOCKET_ERROR_CONNECTION_REFUSED;
  }

  return TINYCSOCKET_SUCCESS;
}

int tcs_simple_bind(tcs_socket* socket_ctx, const char* hostname, const char* port, int domain, int protocol)
{
  struct tcs_addrinfo hints = { 0 };
  hints.ai_family = domain;
  hints.ai_protocol = protocol;
  hints.ai_flags = TINYCSOCKET_AI_PASSIVE;

  struct tcs_addrinfo* address_info = NULL;
  tcs_getaddrinfo(hostname, port, &hints, &address_info);

  bool is_bounded = false;
  for (struct tcs_addrinfo* address_iterator = address_info; address_iterator != NULL; address_iterator = address_iterator->ai_next)
  {
    if (tcs_create(socket_ctx, address_iterator->ai_family, address_iterator->ai_socktype, address_iterator->ai_protocol) != TINYCSOCKET_SUCCESS)
      continue;

    if (tcs_bind(*socket_ctx, address_iterator->ai_addr, address_iterator->ai_addrlen) != TINYCSOCKET_SUCCESS)
    {
      tcs_close(socket_ctx);
      continue;
    }

    is_bounded = true;
    break;
  }

  if (!is_bounded)
  {
    return TINYCSOCKET_ERROR_UNKNOWN;
  }

  return TINYCSOCKET_SUCCESS;
}

int tcs_simple_listen(tcs_socket* socket_ctx, const char* hostname, const char* port, int domain)
{
  if (socket_ctx == NULL || *socket_ctx != TINYCSOCKET_NULLSOCKET)
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

  struct tcs_addrinfo hints = { 0 };

  hints.ai_family = domain;
  hints.ai_protocol = TINYCSOCKET_IPPROTO_TCP;
  hints.ai_socktype = TINYCSOCKET_SOCK_STREAM;
  hints.ai_flags = TINYCSOCKET_AI_PASSIVE;

  struct tcs_addrinfo* listen_addressinfo = NULL;

  int sts = 0;
  sts = tcs_getaddrinfo(NULL, port, &hints, &listen_addressinfo);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  sts = tcs_create(socket_ctx, listen_addressinfo->ai_family, listen_addressinfo->ai_socktype, listen_addressinfo->ai_protocol);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  sts = tcs_bind(*socket_ctx, listen_addressinfo->ai_addr, listen_addressinfo->ai_addrlen);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  sts = tcs_freeaddrinfo(&listen_addressinfo);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  sts = tcs_listen(*socket_ctx, TINYCSOCKET_BACKLOG_SOMAXCONN);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  return TINYCSOCKET_SUCCESS;
}

int tcs_simple_recv_all(tcs_socket socket_ctx, uint8_t* buffer, size_t length)
{
  if (socket_ctx == TINYCSOCKET_NULLSOCKET)
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

  size_t bytes_left = length;
  while (bytes_left > 0)
  {
    size_t bytes_recieved = 0;
    int sts = tcs_recv(socket_ctx, buffer + bytes_recieved, bytes_left, 0, &bytes_recieved);
    if (sts != TINYCSOCKET_SUCCESS)
      return sts;

    bytes_left -= bytes_recieved;
  }
  return TINYCSOCKET_SUCCESS;
}

int tcs_simple_send_all(tcs_socket socket_ctx, uint8_t* buffer, size_t length, uint32_t flags)
{
  size_t left = length;
  size_t sent = 0;

  while (left > 0)
  {

    int sts = tcs_send(socket_ctx, buffer, length, flags, &sent);
    if (sts != TINYCSOCKET_SUCCESS)
      return sts;

    left -= sent;
  }
  return TINYCSOCKET_SUCCESS;
}

int tcs_simple_recv_netstring(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_length, size_t* bytes_recieved)
{
  if (socket_ctx == TINYCSOCKET_NULLSOCKET || buffer == NULL || buffer_length <= 0)
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

  size_t expected_length = 0;
  int parsed = 0;
  int sts = 0;
  char t = '\0';
  const int max_header = 21;
  while (t != ':' && parsed < max_header)
  {
    sts = tcs_simple_recv_all(socket_ctx, (uint8_t*)&t, 1);
    if (sts != TINYCSOCKET_SUCCESS)
      return sts;

    parsed += 1;

    bool is_num = t >= '0' && t <= '9';
    bool is_end = t == ':';
    if (!is_num && !is_end)
      return TINYCSOCKET_ERROR_ILL_FORMED_MESSAGE;

    if (is_end)
      break;

    expected_length += t - '0';
  }

  if (parsed >= max_header)
    return TINYCSOCKET_ERROR_ILL_FORMED_MESSAGE;

  if (buffer_length < expected_length)
    return TINYCSOCKET_ERROR_MEMORY;


  sts = tcs_simple_recv_all(socket_ctx, buffer, expected_length);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  sts = tcs_simple_recv_all(socket_ctx, (uint8_t*)&t, 1);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  if (t != ',')
    return TINYCSOCKET_ERROR_ILL_FORMED_MESSAGE;

  return TINYCSOCKET_SUCCESS;
}


int tcs_simple_send_netstring(tcs_socket socket_ctx, uint8_t* buffer, size_t buffer_length)
{
  if (socket_ctx == TINYCSOCKET_NULLSOCKET)
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

  if (buffer == NULL || buffer_length == 0)
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

  // buffer_length bigger than 64 bits? (size_t can be bigger on some systems)
  if (buffer_length > 0xffffffffffffffffULL)
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

  int header_length = 0;
  char netstring_header[21] = { 0 };

  // %zu is not supported by all compilers, therefor we cast it to llu
  header_length = sprintf(netstring_header, "%llu:", (unsigned long long)buffer_length);

  if (header_length < 0)
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

  int sts = 0;
  sts = tcs_simple_send_all(socket_ctx, (uint8_t*)netstring_header, header_length, 0);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  sts = tcs_simple_send_all(socket_ctx, buffer, buffer_length, 0);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  sts = tcs_simple_send_all(socket_ctx, (uint8_t*)",", 1, 0);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  return TINYCSOCKET_SUCCESS;
}
