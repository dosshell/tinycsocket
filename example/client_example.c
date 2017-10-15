#include <tinycsocket.h>

#include <stdio.h>
#include <stdlib.h>

int show_error(const char* error_text)
{
    fprintf(stderr, "%s", error_text);
    return -1;
}

int main(int argc, const char* argv[])
{
    TinyCSocketCtx client_socket;

    // Client example
    if (tinycsocket_create_socket(&client_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not create a socket");

    if (tinycsocket_connect(&client_socket, "localhost", "1212") != TINYCSOCKET_SUCCESS)
        return show_error("Could not connect to localhost at port 1212");

    char msg[] = "hellow world";
    if (tinycsocket_send_netstring(&client_socket, "hello world", sizeof(msg)) !=
        TINYCSOCKET_SUCCESS)
        return show_error("Could not send data");

    char recv_buffer[1024];
    size_t bytes_recieved = 0;
    if (tinycsocket_recieve_data(&client_socket, recv_buffer, 1023, &bytes_recieved) !=
        TINYCSOCKET_SUCCESS)
    {
        return show_error("Could not recieve data");
    }
    // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in recieve
    recv_buffer[bytes_recieved] = '\0';
    printf("recieved: %s\n", recv_buffer);

    if (tinycsocket_destroy_socket(&client_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not destroy the socket");
}
