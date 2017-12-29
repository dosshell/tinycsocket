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
    if (tinycsocket_init() != TINYCSOCKET_SUCCESS)
        return show_error("Could not init tinycsockets");

    TinyCSocketCtx listen_socket = TINYCSOCKET_NULLSOCKET;
    TinyCSocketCtx child_socket = TINYCSOCKET_NULLSOCKET;

    struct TinyCSocketAddressInfo hints = { 0 };

    hints.ai_family = TINYCSOCKET_AF_INET;
    hints.ai_protocol = TINYCSOCKET_IPPROTO_TCP;
    hints.ai_socktype = TINYCSOCKET_SOCK_STREAM;
    hints.ai_flags = TINYCSOCKET_AI_PASSIVE;

    struct TinyCSocketAddressInfo* listen_addressinfo = NULL;
    if (tinycsocket_getaddrinfo(NULL, "1212", &hints, &listen_addressinfo) != TINYCSOCKET_SUCCESS)
        return show_error("Could not resolve listen address");

    if (tinycsocket_socket(&listen_socket, listen_addressinfo->ai_family, listen_addressinfo->ai_socktype, listen_addressinfo->ai_protocol) != TINYCSOCKET_SUCCESS)
        return show_error("Could not create a listen socket");

    if (tinycsocket_bind(listen_socket, listen_addressinfo->ai_addr, listen_addressinfo->ai_addrlen) != TINYCSOCKET_SUCCESS)
        return show_error("Could not bind to listen address");

    if (tinycsocket_freeaddrinfo(&listen_addressinfo) != TINYCSOCKET_SUCCESS)
        return show_error("Could not free address info");

    if (tinycsocket_listen(listen_socket, TINYCSOCKET_BACKLOG_SOMAXCONN) != TINYCSOCKET_SUCCESS)
        return show_error("Could not listen");

    if (tinycsocket_accept(listen_socket, &child_socket, NULL, NULL) != TINYCSOCKET_SUCCESS)
        return show_error("Could not accept socket");

    if (tinycsocket_closesocket(&listen_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not close listen socket");

    uint8_t recv_buffer[1024];
    size_t bytes_recieved = 0;
    if (tinycsocket_recv(child_socket, recv_buffer, sizeof(recv_buffer) - sizeof('\0'), 0, &bytes_recieved) != TINYCSOCKET_SUCCESS)
        return show_error("Could not recieve data from client");

    recv_buffer[bytes_recieved] = '\0';
    printf("recieved: %s\n", recv_buffer);

    char msg[] = "I here you loud and clear\n";
    if (tinycsocket_send(child_socket, msg, sizeof(msg), 0, NULL) != TINYCSOCKET_SUCCESS)
        return show_error("Could not send reply message");

    if (tinycsocket_shutdown(child_socket, TINYCSOCKET_BOTH) != TINYCSOCKET_SUCCESS)
        return show_error("Could not shutdown socket");

    if (tinycsocket_closesocket(&child_socket) != TINYCSOCKET_SUCCESS)
        return show_error("Could not close socket");

    if (tinycsocket_free() != TINYCSOCKET_SUCCESS)
        return show_error("Could not free tinycsockets");
}
