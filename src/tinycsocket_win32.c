#include "implementation_detector.h"
#ifdef USE_WIN32_IMPL

#include "tinycsocket.h"

// Close to POSIX but we may want to handle some things different

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>

// Internal structures
typedef struct TinyCSocketCtxInternal
{
  SOCKET socket;
} TinyCSocketCtxInternal;

// Internal states
static int gInits = 0;

// Internal functions
static int internal_init_ctx(TinyCSocketCtxInternal* inoutInternalCtx);

int internal_init_ctx(TinyCSocketCtxInternal* inoutInternalCtx)
{
  inoutInternalCtx->socket = INVALID_SOCKET;
  return TINYCSOCKET_SUCCESS;
}

// Implementation of API
int tinycsocket_init()
{
  if (gInits <= 0)
  {
    WSADATA wsaData;
    int wsaStartupErrorCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaStartupErrorCode != 0)
    {
      return TINYCSOCKET_ERROR_KERNEL;
    }
  }
  ++gInits;
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_free()
{
  gInits--;
  if (gInits <= 0)
  {
    WSACleanup();
  }
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_create_socket(TinyCSocketCtx** outSocketCtx)
{
  // Must be a pointer to a null value, sent as a pointer argument
  if (outSocketCtx == NULL || *outSocketCtx != NULL)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  // Allocate socket data
  TinyCSocketCtxInternal** ppInternalCtx = (TinyCSocketCtxInternal**)outSocketCtx;
  *ppInternalCtx = (TinyCSocketCtxInternal*)malloc(sizeof(TinyCSocketCtxInternal));
  if (*ppInternalCtx == NULL)
  {
    return TINYCSOCKET_ERROR_MEMORY;
  }
  TinyCSocketCtxInternal* pInternalCtx = (*ppInternalCtx);

  // Init data
  tinycsocket_init();
  internal_init_ctx(pInternalCtx);

  // Do not create the win-socket now, wait for it in the listen or connect part

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_destroy_socket(TinyCSocketCtx** inoutSocketCtx)
{
  if (inoutSocketCtx == NULL)
    return TINYCSOCKET_SUCCESS;

  TinyCSocketCtxInternal* pInternalCtx = (TinyCSocketCtxInternal*)(*inoutSocketCtx);

  closesocket(pInternalCtx->socket);

  free(*inoutSocketCtx);
  *inoutSocketCtx = NULL;
  tinycsocket_free();

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_connect(TinyCSocketCtx* inoutSocketCtx, const char* address, const char* port)
{
  TinyCSocketCtxInternal* pInternalCtx = inoutSocketCtx;
  if (pInternalCtx == NULL ||
      pInternalCtx->socket != INVALID_SOCKET) // We create the win-socket here
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  struct addrinfo hints;
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  struct addrinfo* result = NULL;
  if (getaddrinfo(address, port, &hints, &result) != 0)
  {
    return TINYCSOCKET_ERROR_ADDRESS_LOOKUP_FAILED;
  }

  // Try to connect
  BOOL didConnect = FALSE;
  for (struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next)
  {
    pInternalCtx->socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (pInternalCtx->socket == INVALID_SOCKET)
    {
      continue;
    }

    if (connect(pInternalCtx->socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
    {
      closesocket(pInternalCtx->socket);
      continue;
    }

    didConnect = TRUE;
  }

  freeaddrinfo(result);

  if (!didConnect)
  {
    return TINYCSOCKET_ERROR_CONNECTION_REFUSED;
  }

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_send_data(TinyCSocketCtx* inSocketCtx, const void* data, const size_t bytes)
{
  TinyCSocketCtxInternal* pInternalCtx = inSocketCtx;
  if (pInternalCtx->socket == NULL)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  if (send(pInternalCtx->socket, data, bytes, 0) == SOCKET_ERROR)
  {
    int sendError = WSAGetLastError();
    switch (sendError)
    {
      case WSANOTINITIALISED:
        return TINYCSOCKET_ERROR_NOT_INITED;
      case WSAETIMEDOUT:
        return TINYCSOCKET_ERROR_TIMED_OUT;
      default:
        return TINYCSOCKET_ERROR_UNKNOWN;
    }
  }
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_recieve_data(TinyCSocketCtx* inSocketCtx,
                             const void* buffer,
                             const size_t bufferByteSize,
                             int* outBytesRecieved)
{
  if (inSocketCtx == NULL || buffer == NULL || bufferByteSize == NULL)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  TinyCSocketCtxInternal* pInternalCtx = inSocketCtx;
  int recvResult = recv(pInternalCtx->socket, buffer, bufferByteSize, 0);
  if (recvResult < 0)
  {
    return TINYCSOCKET_ERROR_UNKNOWN;
  }
  if (outBytesRecieved != NULL)
  {
    *outBytesRecieved = recvResult;
  }
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_bind(TinyCSocketCtx* inSocketCtx, const char* address, const char* port)
{
  TinyCSocketCtxInternal* pInternalCtx = inSocketCtx;

  if (pInternalCtx == NULL || pInternalCtx->socket != INVALID_SOCKET)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  struct addrinfo* result = NULL;
  struct addrinfo hints;
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  int addressInfo = getaddrinfo(NULL, port, &hints, &result);
  if (addressInfo != 0)
  {
    return TINYCSOCKET_ERROR_KERNEL;
  }

  BOOL didConnect = FALSE;
  pInternalCtx->socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (pInternalCtx->socket == INVALID_SOCKET)
  {
    return TINYCSOCKET_ERROR_KERNEL;
  }

  if (bind(pInternalCtx->socket, result->ai_addr, (int)result->ai_addrlen) != SOCKET_ERROR)
  {
    didConnect = TRUE;
  }

  freeaddrinfo(result);

  if (!didConnect)
  {
    return TINYCSOCKET_ERROR_UNKNOWN;
  }

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_listen(TinyCSocketCtx* inoutSocketCtx)
{
  TinyCSocketCtxInternal* pInternalCtx = inoutSocketCtx;

  if (pInternalCtx == NULL || pInternalCtx->socket == INVALID_SOCKET)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  if (listen(pInternalCtx->socket, SOMAXCONN) == SOCKET_ERROR)
  {
    return TINYCSOCKET_ERROR_UNKNOWN;
  }

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_accept(TinyCSocketCtx* inListenSocketCtx, TinyCSocketCtx* inoutBindSocketCtx)
{
  TinyCSocketCtxInternal* pInternalCtx = inListenSocketCtx;
  TinyCSocketCtxInternal* pInternalBindCtx = inoutBindSocketCtx;

  pInternalBindCtx->socket = accept(pInternalCtx->socket, NULL, NULL);

  return TINYCSOCKET_SUCCESS;
}

#endif