// MIT

#ifndef TINY_C_SOCKETS_H_
#define TINY_C_SOCKETS_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static int TINYCSOCKET_SUCCESS = 0;
static int TINYCSOCKET_ERROR_UNKNOWN = -1;
static int TINYCSOCKET_ERROR_MEMORY = -2;
static int TINYCSOCKET_ERROR_INVALID_ARGUMENT = -3;
static int TINYCSOCKET_ERROR_KERNEL = -4;
static int TINYCSOCKET_ERROR_ADDRESS_LOOKUP_FAILED = -5;
static int TINYCSOCKET_ERROR_CONNECTION_REFUSED = -6;
static int TINYCSOCKET_ERROR_NOT_INITED = -7;
static int TINYCSOCKET_ERROR_TIMED_OUT = -8;
static int TINYCSOCKET_ERROR_NOT_IMPLEMENTED = -9;
static int TINYCSOCKET_ERROR_NOT_CONNECTED = -10;

typedef void TinyCSocketCtx;

int tinycsocket_init();
int tinycsocket_free();
int tinycsocket_create_socket(TinyCSocketCtx** outSocketCtx);
int tinycsocket_destroy_socket(TinyCSocketCtx** inoutSocketCtx);
int tinycsocket_connect(TinyCSocketCtx* inoutSocketCtx, const char* address, const char* port);
int tinycsocket_send_data(TinyCSocketCtx* inSocketCtx, const void* data, const size_t bytes);
int tinycsocket_send_netstring(TinyCSocketCtx* inSocketCtx, const char* text);
int tinycsocket_recieve_netstring(TinyCSocketCtx* inSocketCtx, char** outText);
int tinycsocket_recieve_data(TinyCSocketCtx* inSocketCtx,
                             const void* buffer,
                             const size_t bufferByteSize,
                             int* outBytesRecieved);

#ifdef __cplusplus
}
#endif

#endif