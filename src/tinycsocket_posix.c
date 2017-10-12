#include "implementation_detector.h"
#ifdef USE_POSIX_IMPL

#include "tinycsocket.h"

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <stdlib.h>
#include <netdb.h>

#include <errno.h>
#include <string.h>

#include <stdbool.h>

static const int NO_SOCKET = -1;
static const int FAILURE = -1;

// Internal structures
typedef struct TinyCSocketCtxInternal
{
  int socket;
} TinyCSocketCtxInternal;

int tinycsocket_init()
{
  // Not needed for posix
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_free()
{
  // Not needed for posix
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_create_socket(TinyCSocketCtx** socket_ctx)
{
  if (socket_ctx == NULL || *socket_ctx != NULL)
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;

  // Allocate socket data
  TinyCSocketCtxInternal** internal_socket_ctx_ptr = (TinyCSocketCtxInternal**)socket_ctx;
  *internal_socket_ctx_ptr = (TinyCSocketCtxInternal*)malloc(sizeof(TinyCSocketCtxInternal));
  if (*internal_socket_ctx_ptr == NULL)
  {
    return TINYCSOCKET_ERROR_MEMORY;
  }
  TinyCSocketCtxInternal* internal_ctx = (*internal_socket_ctx_ptr);
  
  internal_ctx->socket = NO_SOCKET;

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_destroy_socket(TinyCSocketCtx** socket_ctx)
{
  if (socket_ctx == NULL)
  return TINYCSOCKET_SUCCESS;

  TinyCSocketCtxInternal* internal_socket_ctx = (TinyCSocketCtxInternal*)(*socket_ctx);
  close(internal_socket_ctx->socket);

  free(*socket_ctx);
  *socket_ctx = NULL;

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_connect(TinyCSocketCtx* socket_ctx, const char* address, const char* port)
{
  TinyCSocketCtxInternal* internal_socket_ctx = socket_ctx;
  if (internal_socket_ctx == NULL || internal_socket_ctx->socket != NO_SOCKET)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }
  u_short port_number = atoi(port);
  if (port_number <= 0)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }
  
  struct addrinfo *serverinfo = NULL, *ptr = NULL, hints;
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (getaddrinfo(address, port, &hints, &serverinfo) != 0)
  {
    return TINYCSOCKET_ERROR_ADDRESS_LOOKUP_FAILED;
  }

  // Try to connect
  bool did_connect = false;
  for (ptr = serverinfo; ptr != NULL; ptr = ptr->ai_next)
  {
    internal_socket_ctx->socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (internal_socket_ctx->socket == NO_SOCKET)
    {
      continue;
    }

    if (connect(internal_socket_ctx->socket, ptr->ai_addr, ptr->ai_addrlen) == FAILURE)
    {
      close(internal_socket_ctx->socket);
      continue;
    }

    did_connect = true;
  }

  if (!did_connect)
  {
    return TINYCSOCKET_ERROR_CONNECTION_REFUSED;
  }
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_send_data(TinyCSocketCtx* socket_ctx, const void* data, const size_t bytes)
{
  TinyCSocketCtxInternal* internal_socket_ctx = socket_ctx;
  if (internal_socket_ctx->socket == NO_SOCKET)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  if (send(internal_socket_ctx->socket, data, bytes, 0) == FAILURE)
  {
    int send_error_code = errno;
    switch (send_error_code)
    {
      case ENOTCONN:
        return TINYCSOCKET_ERROR_NOT_CONNECTED;
      default:
        return TINYCSOCKET_ERROR_UNKNOWN;
    }
  }
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_recieve_data(TinyCSocketCtx* socket_ctx,
                             const void* buffer,
                             const size_t buffer_byte_size,
                             int* bytes_recieved)
{
  if (socket_ctx == NULL || buffer == NULL || buffer_byte_size == NULL)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  TinyCSocketCtxInternal* internal_socket_ctx = socket_ctx;
  int recv_status_code = recv(internal_socket_ctx->socket, buffer, buffer_byte_size, 0);
  if (recv_status_code == -1)
  {
    return TINYCSOCKET_ERROR_UNKNOWN;
  }
  if (bytes_recieved != NULL)
  {
    *bytes_recieved = recv_status_code;
  }
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_bind(TinyCSocketCtx* socket_ctx, const char* address, const char* port)
{
  TinyCSocketCtxInternal* internal_socket_ctx = socket_ctx;

  if (internal_socket_ctx == NULL || internal_socket_ctx->socket != NO_SOCKET)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  struct addrinfo* result = NULL;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  int addressInfo = getaddrinfo(NULL, port, &hints, &result);
  if (addressInfo != 0)
  {
    return TINYCSOCKET_ERROR_KERNEL;
  }

  bool did_connect = false;
  internal_socket_ctx->socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (internal_socket_ctx->socket == NO_SOCKET)
  {
    return TINYCSOCKET_ERROR_KERNEL;
  }

  if (bind(internal_socket_ctx->socket, result->ai_addr, (int)result->ai_addrlen) != FAILURE)
  {
    did_connect = true;
  }

  freeaddrinfo(result);

  if (!did_connect)
  {
    return TINYCSOCKET_ERROR_UNKNOWN;
  }

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_listen(TinyCSocketCtx* socket_ctx)
{
  TinyCSocketCtxInternal* internal_socket_ctx = socket_ctx;

  if (internal_socket_ctx == NULL || internal_socket_ctx->socket == NO_SOCKET)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  if (listen(internal_socket_ctx->socket, SOMAXCONN) == FAILURE)
  {
    return TINYCSOCKET_ERROR_UNKNOWN;
  }

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_accept(TinyCSocketCtx* listen_socket_ctx, TinyCSocketCtx* bind_socket_ctx)
{
  TinyCSocketCtxInternal* internal_listen_socket_ctx = listen_socket_ctx;
  TinyCSocketCtxInternal* internal_bind_socket_ctx = bind_socket_ctx;

  internal_bind_socket_ctx->socket = accept(internal_listen_socket_ctx->socket, NULL, NULL);

  return TINYCSOCKET_SUCCESS;
}

#endif
