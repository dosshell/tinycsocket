#include <tinycsocket.h>

#include <stdio.h>
#include <stdlib.h>

int show_error(const char* error_text)
{
  fprintf(stderr, error_text);
  return -1;
}

int main()
{
  TinyCSocketCtx* listenSocket = NULL;
  if (tinycsocket_create_socket(&listenSocket) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not recreate the socket");
  }

  if (tinycsocket_bind(listenSocket, "localhost", "1212") != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not bind to localhost at port 1212");
  }

  if (tinycsocket_listen(listenSocket) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not listen on localhost at port 1212");
  }

  TinyCSocketCtx* bindingSocket = NULL;
  if (tinycsocket_create_socket(&bindingSocket) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not create the binding socket");
  }

  if (tinycsocket_accept(listenSocket, bindingSocket) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not accept socket");
  }

  char buffer[1024];
  int bytesRecieved = 0;
  if (tinycsocket_recieve_data(bindingSocket, buffer, 1023, &bytesRecieved) != TINYCSOCKET_SUCCESS)
  {
    return show_error("Could not recieve data");
  }
  // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in recieve
  buffer[bytesRecieved] = '\0';
  printf("recieved: %s\n", buffer);

  char msg[] = "hello there";
  if (tinycsocket_send_data(bindingSocket, msg, sizeof(msg)) != TINYCSOCKET_SUCCESS)
  {
    return show_error("asd :/");
  }

  printf("OK\n");
  return 0;
}