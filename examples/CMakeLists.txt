# TCP Server example
add_executable(tcp_server tcp_server.c)
target_link_libraries(tcp_server PRIVATE tinycsocket_header)

# TCP Client example
add_executable(tcp_client tcp_client.c)
target_link_libraries(tcp_client PRIVATE tinycsocket_header)

# UDP Client example
add_executable(udp_client udp_client.c)
target_link_libraries(udp_client PRIVATE tinycsocket_header)

# UDP Server example
add_executable(udp_server udp_server.c)
target_link_libraries(udp_server PRIVATE tinycsocket_header)

set_target_properties(
    tcp_server
    tcp_client
    udp_client
    udp_server
    PROPERTIES FOLDER TCS/examples
)
