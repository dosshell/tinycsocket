#include <tinycsocket.h>

#include <stdio.h>
#include <stdlib.h>

int show_error(const char* error_text)
{
    fprintf(stderr, "%s", error_text);
    return -1;
}

int main()
{
    TinyCSocketCtx listen_socket;
    TinyCSocketCtx binding_socket;

    if (tinycsocket_create_socket(&listen_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not recreate the socket");

    if (tinycsocket_create_socket(&binding_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not create the binding socket");

    if (tinycsocket_bind_and_listen(&listen_socket, "localhost", "1212") != TINYCSOCKET_SUCCESS)
        return show_error("Could not bind and listen to localhost at port 1212");

    if (tinycsocket_accept(&listen_socket, &binding_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not accept socket");

    char buffer[1024];
    size_t bytes_recieved = 0;
    if (tinycsocket_recieve_netstring(&binding_socket, buffer, 1023, &bytes_recieved) !=
        TINYCSOCKET_SUCCESS)
        return show_error("Could not recieve data");

    // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in recieve
    buffer[bytes_recieved] = '\0';
    printf("recieved: %s\n", buffer);

    char msg[] = "hello there";
    if (tinycsocket_send_data(&binding_socket, msg, sizeof(msg)) != TINYCSOCKET_SUCCESS)
        return show_error("Could not send message to client");

    if (tinycsocket_destroy_socket(&listen_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not destroy listening socket");

    if (tinycsocket_destroy_socket(&binding_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not destroy binding socket");
}
