// MIT

#ifndef TINY_C_SOCKETS_H_
#define TINY_C_SOCKETS_H_

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

typedef void TinyCSocketCtx;

int tinycsocket_init();
int tinycsocket_free();
int tinycsocket_create_socket(TinyCSocketCtx** outSocketCtx);
int tinycsocket_close_socket(TinyCSocketCtx** inoutSocketCtx);
int tinycsocket_connect(TinyCSocketCtx* inoutSocketCtx, const char* address, const char* port);

#ifdef __cplusplus
}
#endif

#endif