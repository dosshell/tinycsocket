cmake_minimum_required(VERSION 3.10)

# CMake fix for crosscompile (MinGW-w64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(tinycsocket VERSION 0.3)

# Add warning levels for gcc and msvc
if (MSVC)
  if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif ()
  if (CMAKE_C_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  else ()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  endif ()
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
elseif (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(COMMON_GXX_FLAGS "-Wall -Wextra -pedantic -Winit-self -Wpointer-arith -Wcast-align -Wconversion -Wshadow -Wundef -Wwrite-strings -Wswitch-enum -Wswitch-default")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_GXX_FLAGS} -Wstrict-prototypes -Wmissing-prototypes -std=c99")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_GXX_FLAGS}")
  add_definitions(-D_POSIX_C_SOURCE=200112L -D_DEFAULT_SOURCE -D_ISOC99_SOURCE)
endif ()

option(TCS_ENABLE_TESTS "Enable tests" OFF)
option(TCS_ENABLE_EXAMPLES "Enable examples" OFF)
option(TCS_WARNINGS_AS_ERRORS "Enable treat warnings as errors" OFF)
option(TCS_GENERATE_COVERAGE "Enable for test coverage generation" OFF)

# ifaddrs crossplatform define
include(CheckSymbolExists)
check_symbol_exists(getifaddrs "sys/types.h;ifaddrs.h" TCS_HAVE_IFADDRS)
if (TCS_HAVE_IFADDRS)
  add_definitions("-DTCS_HAVE_IFADDRS")
endif ()

SET(TINYCSOCKET_SRC
    "src/tinycsocket_internal.h"
    "src/tinydatastructures.h"
    "src/tinycsocket_common.c"
    "src/tinycsocket_win32.c"
    "src/tinycsocket_posix.c")

# Header generation
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/tinycsocket.h COMMAND m4 ${CMAKE_CURRENT_SOURCE_DIR}/src/tinycsocket.h.m4 > ${CMAKE_CURRENT_SOURCE_DIR}/include/tinycsocket.h
    DEPENDS ${TINYCSOCKET_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/src/tinycsocket.h.m4
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_custom_target(generate_header SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include/tinycsocket.h)
set_target_properties(generate_header PROPERTIES FOLDER tinycsocket)

# Header interface
add_library(tinycsocket_header INTERFACE)
add_dependencies(tinycsocket_header generate_header)
target_include_directories(tinycsocket_header INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if (WIN32)
  target_link_libraries(tinycsocket_header INTERFACE wsock32 ws2_32 iphlpapi)
endif ()

# Tinycsocket static library
add_library(tinycsocket STATIC ${TINYCSOCKET_SRC})
add_library(tinycsockets ALIAS tinycsocket)
target_include_directories(tinycsocket PUBLIC include PRIVATE src)
target_link_libraries(tinycsocket PRIVATE tinycsocket_header)
set_target_properties(tinycsocket PROPERTIES FOLDER tinycsocket)

if (TCS_WARNINGS_AS_ERRORS)
    if (MSVC)
        target_compile_options(tinycsocket PRIVATE /W4 /WX)
    else ()
        target_compile_options(tinycsocket PRIVATE -Werror)
    endif ()
endif ()

install(TARGETS tinycsocket DESTINATION lib)
install(FILES include/tinycsocket.h DESTINATION include)

# Extra targets for development
if (TCS_ENABLE_TESTS)
    add_library(tinycsocket_wrapped STATIC
                ${TINYCSOCKET_SRC}
                "src/dbg_wrap.h")
    target_include_directories(tinycsocket_wrapped PUBLIC include src)
    target_link_libraries(tinycsocket_wrapped INTERFACE tinycsocket_header)
    target_compile_options(tinycsocket_wrapped PUBLIC "-DDO_WRAP")
endif ()

if (TCS_GENERATE_COVERAGE)
  if (MSVC)
    message(SEND_ERROR "MSVC with code coverage is not supported.")
  endif ()
  target_compile_options(tinycsocket_wrapped PRIVATE --coverage)
  target_link_libraries(tinycsocket_wrapped PRIVATE --coverage)
endif ()

if (TCS_ENABLE_TESTS)
  add_subdirectory(tests)
endif ()

if (TCS_ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif ()
