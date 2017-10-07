#include "tinycsocket.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>

// Internal structures
typedef struct TinyCSocketCtxInternal
{
  SOCKET soc;
} TinyCSocketCtxInternal;

// Internal states
static int gInits = 0;

// Internal functions
static int internal_init_ctx(TinyCSocketCtxInternal* inoutInternalCtx);

int internal_init_ctx(TinyCSocketCtxInternal* inoutInternalCtx)
{
  inoutInternalCtx->soc = INVALID_SOCKET;
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
  internal_init_ctx(pInternalCtx);
  pInternalCtx->soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (pInternalCtx->soc == INVALID_SOCKET)
  {
    tinycsocket_close_socket(&pInternalCtx);
    return TINYCSOCKET_ERROR_UNKNOWN;
  }

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_close_socket(TinyCSocketCtx** inoutSocketCtx)
{
  if (inoutSocketCtx == NULL)
    return TINYCSOCKET_SUCCESS;

  TinyCSocketCtxInternal* pInternalCtx = (TinyCSocketCtxInternal*)(*inoutSocketCtx);
  int shutdownErrorCode = shutdown(pInternalCtx->soc, SD_SEND);

  closesocket(pInternalCtx->soc);

  free(*inoutSocketCtx);
  *inoutSocketCtx = NULL;

  if (shutdownErrorCode == SOCKET_ERROR)
  {
    return TINYCSOCKET_ERROR_UNKNOWN;
  }

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_connect(TinyCSocketCtx* inoutSocketCtx, const char* address, const char* port)
{
  TinyCSocketCtxInternal* pInternalCtx = inoutSocketCtx;
  if (pInternalCtx == NULL || pInternalCtx->soc == INVALID_SOCKET)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  struct addrinfo *result = NULL, *ptr = NULL, hints;

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (getaddrinfo(address, port, &hints, &result) != 0)
  {
    return TINYCSOCKET_ERROR_ADDRESS_LOOKUP_FAILED;
  }

  // Try to connect
  int connectionErrorCode = 0;
  for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
  {
    if (connect(pInternalCtx->soc, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
    {
      closesocket(pInternalCtx->soc);
      connectionErrorCode = -1;
      continue;
    }
    else
      break;
  }

  freeaddrinfo(result);

  if (connectionErrorCode != 0)
  {
    return TINYCSOCKET_ERROR_CONNECTION_REFUSED;
  }

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_send_data(TinyCSocketCtx* inSocketCtx, const void* data, const size_t bytes)
{
  TinyCSocketCtxInternal* pInternalCtx = inSocketCtx;
  if (pInternalCtx->soc == NULL)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }

  if (send(pInternalCtx->soc, data, bytes, 0) == SOCKET_ERROR)
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
#endif // WIN32
