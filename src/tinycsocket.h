// MIT

#ifndef TINY_C_SOCKETS_H_
#define TINY_C_SOCKETS_H_

static int TINYCSOCKET_SUCCESS = 0;
static int TINYCSOCKET_ERROR_UNKNOWN = -1;
static int TINYCSOCKET_ERROR_MEMORY = -2;
static int TINYCSOCKET_ERROR_INVALID_ARGUMENT = -3;

typedef void TinyCSocketCtx;

int tinycsocket_create_socket(TinyCSocketCtx** outSocketCtx);
int tinycsocket_close_socket(TinyCSocketCtx** inoutSocketCtx);

#endif