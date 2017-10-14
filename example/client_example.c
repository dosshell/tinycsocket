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
  // Init library
  if (tinycsocket_init() != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not init tinycsocket");
  }

  TinyCSocketCtx* client_socket = NULL;

  // Client example
  if (tinycsocket_create_socket(&client_socket) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not create a socket");
  }

  if (tinycsocket_connect(client_socket, "localhost", "1212") != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not connect to localhost at port 1212");
  }

  const char msg[] = "hello world";
  if (tinycsocket_send_data(client_socket, msg, sizeof(msg)) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not send data");
  }

  char recv_buffer[1024];
  int bytes_recieved = 0;
  if (tinycsocket_recieve_data(client_socket, recv_buffer, 1023, &bytes_recieved) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not recieve data");
  }
  // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in recieve
  recv_buffer[bytes_recieved] = '\0';
  printf("recieved: %s\n", recv_buffer);

  if (tinycsocket_destroy_socket(&client_socket) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not close the socket");
  }

  // Free resources
  if (tinycsocket_free() != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not free tinycsocket");
  }
  printf("OK\n");
  return 0;
}
