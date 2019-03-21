add_library(tinycsockets STATIC
            tinycsocket.h
            tinycsocket_simple.c
            tinycsocket_win32.c
            tinycsocket_posix.c)

add_library(tinycsocket ALIAS tinycsockets)

target_include_directories(tinycsockets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
	target_link_libraries(tinycsockets PRIVATE wsock32 ws2_32)
endif()

install(TARGETS tinycsockets DESTINATION lib)
install(FILES tinycsocket.h DESTINATION include)

