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
static int TINYCSOCKET_ERROR_KERNEL;

typedef void TinyCSocketCtx;

int tinycsocket_init();
int tinycsocket_free();
int tinycsocket_create_socket(TinyCSocketCtx** outSocketCtx);
int tinycsocket_close_socket(TinyCSocketCtx** inoutSocketCtx);

#ifdef __cplusplus
}
#endif

#endif