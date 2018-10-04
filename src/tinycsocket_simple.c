#include "tinycsocket.h"
#include <stdbool.h>

int tcs_simple_connect(tcs_socket* socket_ctx, const char* hostname, const char* port, int domain, int protocol)
{
  struct tcs_addrinfo hints = { 0 };
  hints.ai_family = domain;
  hints.ai_protocol = protocol;

  struct tcs_addrinfo* address_info = NULL;
  int sts = tcs_getaddrinfo(hostname, port, &hints, &address_info);
  if (sts != TINYCSOCKET_SUCCESS)
    return sts;

  bool is_connected = false;
  for (struct tcs_addrinfo* address_iterator = address_info; address_iterator != NULL; address_iterator = address_iterator->ai_next)
  {
    if (tcs_create(socket_ctx, address_iterator->ai_family, address_iterator->ai_socktype, address_iterator->ai_protocol) != TINYCSOCKET_SUCCESS)
      continue;

    if (tcs_connect(*socket_ctx, address_iterator->ai_addr, address_iterator->ai_addrlen) != TINYCSOCKET_SUCCESS)
    {
      tcs_close(socket_ctx);
      continue;
    }

    is_connected = true;
    break;
  }

  tcs_freeaddrinfo(&address_info);

  if (!is_connected)
    return TINYCSOCKET_ERROR_CONNECTION_REFUSED;

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

int tcs_simple_send_netstring(tcs_socket* socket_ctx, uint8_t buffer, size_t buffer_length)
{
  return TINYCSOCKET_ERROR_NOT_IMPLEMENTED;
}

int tcs_simple_recv_netstring(tcs_socket* socket_ctx, uint8_t buffer, size_t buffer_length)
{
  return TINYCSOCKET_ERROR_NOT_IMPLEMENTED;
}
