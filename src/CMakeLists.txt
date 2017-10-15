add_library (tinycsocket
			 tinycsocket.h
			 tinycsocket_win32.c
			 tinycsocket_posix.c)

target_include_directories (tinycsocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
  target_link_libraries(tinycsocket wsock32 ws2_32)
endif()