#include <tinycsocket.h>

#include <stdio.h>
#include <stdlib.h>

int show_error(const char* error_text)
{
  fprintf(stderr, error_text);
  getchar();
  return -1;
}

int main(int argc, const char* argv[])
{
  if (tinycsocket_init() != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not init tinycsocket");
  }

  TinyCSocketCtx* socketCtx = NULL;
  if (tinycsocket_create_socket(&socketCtx) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not create a socket");
  }

  if (tinycsocket_connect(socketCtx, "localhost", "1212") != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not connect to localhost at port 1212");
  }

  const char* msg = "hello world";
  if (tinycsocket_send_data(socketCtx, msg, strlen(msg)) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not send data");
  }

  if (tinycsocket_close_socket(&socketCtx) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not close the socket");
  }

  if (tinycsocket_free() != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not free tinycsocket");
  }
  printf("OK");
  getchar();
  return 0;
}