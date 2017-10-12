# Client example
add_executable (client_example client_example.c)
target_link_libraries (client_example LINK_PUBLIC tinycsocket)

# Server example
add_executable (server_example server_example.c)
target_link_libraries (server_example LINK_PUBLIC tinycsocket)
