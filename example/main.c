#include <tinycsocket.h>

#include <stdio.h>
#include <stdlib.h>

int show_error(const char* error_text)
{
  fprintf(stderr, error_text);
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

  const char msg[] = "hello world";
  if (tinycsocket_send_data(socketCtx, msg, sizeof(msg)) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not send data");
  }

  char buffer[1024];
  int bytesRecieved = 0;
  if (tinycsocket_recieve_data(socketCtx, buffer, 1023, &bytesRecieved) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not recieve data");
  }
  // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in recieve
  buffer[bytesRecieved] = '\0';
  printf("recieved: %s\n", buffer);

  if (tinycsocket_destroy_socket(&socketCtx) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not close the socket");
  }

  if (tinycsocket_free() != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not free tinycsocket");
  }
  printf("OK\n");
  return 0;
}
