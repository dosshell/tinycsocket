#include <tinycsocket.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int show_error(const char* error_text)
{
    fprintf(stderr, "%s", error_text);
    return -1;
}

int main(int argc, const char* argv[])
{
    if (tcs_init() != TINYCSOCKET_SUCCESS)
        return show_error("Could not init tinycsockets");

    tcs_socket client_socket = TINYCSOCKET_NULLSOCKET;

    if (tcs_new(&client_socket, TINYCSOCKET_AF_INET, TINYCSOCKET_SOCK_STREAM, TINYCSOCKET_IPPROTO_TCP) != TINYCSOCKET_SUCCESS)
        return show_error("Could not create a socket");

    struct tcs_addrinfo* address_info = NULL;
    if (tcs_getaddrinfo("localhost", "1212", NULL, &address_info) != TINYCSOCKET_SUCCESS)
        return show_error("Could not resolve host");

    bool isConnected = false;
    for (struct tcs_addrinfo* address_iterator = address_info; address_iterator != NULL; address_iterator = address_iterator->ai_next)
    {
        if (tcs_connect(client_socket, address_iterator->ai_addr, address_iterator->ai_addrlen) == TINYCSOCKET_SUCCESS)
        {
            isConnected = true;
            break;
        }
    }

    if (!isConnected)
        return show_error("Could not connect to server");

    char msg[] = "hello world\n";
    tcs_send(client_socket, msg, sizeof(msg), 0, NULL);

    uint8_t recv_buffer[1024];
    size_t bytes_recieved = 0;
    if (tcs_recv(client_socket, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, &bytes_recieved) != TINYCSOCKET_SUCCESS)
        return show_error("Could not recieve data");

    // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in recieve
    recv_buffer[bytes_recieved] = '\0';
    printf("recieved: %s\n", recv_buffer);

    if (tcs_shutdown(client_socket, TINYCSOCKET_SD_BOTH) != TINYCSOCKET_SUCCESS)
        return show_error("Could not shutdown socket");

    if (tcs_delete(&client_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not close the socket");

    if (tcs_free() != TINYCSOCKET_SUCCESS)
        return show_error("Could not free tinycsockets");
}
