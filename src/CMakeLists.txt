add_library(tinycsocket STATIC
            "tinycsocket.h"
            "tinycsocket_common.c"
            "tinycsocket_win32.c"
            "tinycsocket_posix.c")

add_library(tinycsockets ALIAS tinycsocket)

target_include_directories(tinycsocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(tinycsocket PROPERTIES FOLDER TCS)

if (WIN32)
	target_link_libraries(tinycsocket PRIVATE wsock32 ws2_32 iphlpapi)
endif ()

if (TCS_WARNINGS_AS_ERRORS)
    if (MSVC)
        target_compile_options(tinycsocket PRIVATE /W4 /WX)
    else ()
        target_compile_options(tinycsocket PRIVATE -Wall -Wextra -pedantic -Werror)
    endif ()
endif ()

install(TARGETS tinycsocket DESTINATION lib)
install(FILES tinycsocket.h DESTINATION include)
