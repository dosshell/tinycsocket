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
  return TINYCSOCKET_ERROR_KERNEL;
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
  TinyCSocketCtxInternal **ppInternalCtx = (TinyCSocketCtxInternal**)outSocketCtx;
  *ppInternalCtx = (TinyCSocketCtxInternal*)malloc(sizeof(TinyCSocketCtxInternal));
  if (*ppInternalCtx == NULL)
  {
    return TINYCSOCKET_ERROR_MEMORY;
  }
  TinyCSocketCtxInternal *pInternalCtx = (*ppInternalCtx);

  // Init data
  internal_init_ctx(pInternalCtx);
  pInternalCtx->soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

  TinyCSocketCtxInternal *pInternalCtx = (TinyCSocketCtxInternal*)(*inoutSocketCtx);
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

#endif // WIN32
