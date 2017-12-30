#include <tinycsocket.h>

#include <stdio.h>
#include <stdbool.h>

int show_error(const char* error_text)
{
    fprintf(stderr, "%s", error_text);
    return -1;
}

int main(int argc, const char* argv[])
{
    if (tcs_init() != TINYCSOCKET_SUCCESS)
        return show_error("Could not init tinycsockets");

    tcs_socket socket = TINYCSOCKET_NULLSOCKET;

    struct tcs_addrinfo* address_info = NULL;

    struct tcs_addrinfo hints = { 0 };
    hints.ai_family = TINYCSOCKET_AF_INET;
    hints.ai_socktype = TINYCSOCKET_SOCK_DGRAM;
    hints.ai_flags = TINYCSOCKET_AI_PASSIVE;

    tcs_getaddrinfo("localhost", "1212", &hints, &address_info);

    bool didBind = false;
    for (struct tcs_addrinfo* address_iterator = address_info; address_iterator != NULL; address_iterator = address_iterator->ai_next)
    {
        if (tcs_new(&socket, address_iterator->ai_family, address_iterator->ai_socktype, address_iterator->ai_protocol) != TINYCSOCKET_SUCCESS)
            continue;

        if (tcs_bind(socket, address_iterator->ai_addr, address_iterator->ai_addrlen) != TINYCSOCKET_SUCCESS)
            continue;

        didBind = true;
        break;
    }

    if (!didBind)
        return show_error("Could not bind socket");

    struct TinyCSocketAddress remote_address = { 0 };
    size_t remote_address_size = sizeof(remote_address);
    uint8_t recv_buffer[1024];
    size_t bytes_recieved = 0;
    if (tcs_recvfrom(socket, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, &remote_address, &remote_address_size, &bytes_recieved) != TINYCSOCKET_SUCCESS)
        return show_error("Could not recieve data");
    
    // Makes sure it is a NULL terminated string, this is why we only accept 1023 bytes in recieve
    recv_buffer[bytes_recieved] = '\0';
    printf("recieved: %s\n", recv_buffer);

    char msg[] = "I here you loud and clear\n";
    if (tcs_sendto(socket, msg, sizeof(msg), 0, &remote_address, sizeof(remote_address), NULL) != TINYCSOCKET_SUCCESS)
        return show_error("Could not send message");

    if (tcs_delete(&socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not close socket");

    if (tcs_free() != TINYCSOCKET_SUCCESS)
        return show_error("Could not free tinycsockets");
}
