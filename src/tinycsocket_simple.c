#include "tinycsocket.h"
#include <stdbool.h>

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

int tcs_simple_send_netstring(tcs_socket* socket_ctx, uint8_t* buffer, size_t buffer_length)
{
  return TINYCSOCKET_ERROR_NOT_IMPLEMENTED;
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

int tcs_simple_recv_fixed(tcs_socket socket_ctx, uint8_t* buffer, size_t length)
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

int tcs_simple_recv_netstring(tcs_socket* socket_ctx, uint8_t buffer, size_t buffer_length)
{
  return TINYCSOCKET_ERROR_NOT_IMPLEMENTED;
}
