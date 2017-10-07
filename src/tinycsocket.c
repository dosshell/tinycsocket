#include "tinycsocket.h"

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>

typedef struct TinyCSocketCtxInternal
{
  SOCKET soc;
} TinyCSocketCtxInternal;

int init_internal_ctx(TinyCSocketCtxInternal* inoutInternalCtx)
{
  inoutInternalCtx->soc = INVALID_SOCKET;
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_create_socket(TinyCSocketCtx** outSocketCtx)
{
  // Must be a pointer to a null value, sent as a pointer argument
  if (outSocketCtx == NULL || *outSocketCtx != NULL)
  {
    return TINYCSOCKET_ERROR_INVALID_ARGUMENT;
  }
  
  TinyCSocketCtxInternal **ppInternalCtx = (TinyCSocketCtxInternal**)outSocketCtx;
  *ppInternalCtx = (TinyCSocketCtxInternal*)malloc(sizeof(TinyCSocketCtxInternal));
  if (*ppInternalCtx == NULL)
  {
    return TINYCSOCKET_ERROR_MEMORY;
  }
  TinyCSocketCtxInternal &internalCtx = (**ppInternalCtx);

  init_internal_ctx(&internalCtx);
  internalCtx.soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (internalCtx.soc != 0)
  {
    return TINYCSOCKET_ERROR_UNKNOWN;
  }

  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_connect_socket(TinyCSocketCtx* inoutSocketCtx, const int inPort)
{
  return TINYCSOCKET_SUCCESS;
}

int tinycsocket_close_socket(TinyCSocketCtx** inoutSocketCtx)
{
  if (inoutSocketCtx == nullptr)
    return TINYCSOCKET_SUCCESS;

  free(*inoutSocketCtx);
  *inoutSocketCtx = NULL;

  return TINYCSOCKET_SUCCESS;
}

#endif // WIN32
