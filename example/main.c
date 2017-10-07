#include <tinycsocket.h>

#include <stdlib.h>
#include <stdio.h>

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