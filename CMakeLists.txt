cmake_minimum_required(VERSION 3.10)

# CMake fix for crosscompile (MinGW-w64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(tinycsocket VERSION 0.4)

include(CheckSymbolExists)
include(CheckIncludeFile)

# Add warning levels for gcc and msvc
if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(
            REGEX REPLACE
            "/W[0-4]"
            "/W4"
            CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS}"
        )
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
    if(CMAKE_C_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    endif()
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # set(COMMON_GXX_FLAGS
    #     "-Wno-unused-parameter -Wall -Wextra -Wpedantic -Walloca -Wcast-align -Wcast-qual -Wdouble-promotion -Wduplicated-cond -Wenum-conversion -Wfloat-equal -Wformat-overflow=2 -Wformat-signedness -Wformat=2 -Wframe-larger-than=2048 -Wlogical-op -Wmissing-braces -Wmultichar -Wpointer-arith -Wrestrict -Wshadow -Wsuggest-attribute=format -Wsuggest-attribute=malloc -Wuninitialized -Wvla -Wwrite-strings -Wsign-conversion"
    # )
    # set(CMAKE_C_FLAGS
    #     "${CMAKE_C_FLAGS} ${COMMON_GXX_FLAGS} -Wno-unused-parameter -Wall -Wextra -Wpedantic -Walloca -Wcast-align -Wcast-qual -Wdouble-promotion -Wduplicated-cond -Wenum-conversion -Wfloat-equal -Wformat-overflow=2 -Wformat-signedness -Wformat=2 -Wframe-larger-than=2048 -Wlogical-op -Wmissing-braces -Wmultichar -Wpointer-arith -Wrestrict -Wshadow -Wsuggest-attribute=format -Wsuggest-attribute=malloc -Wuninitialized -Wvla -Wwrite-strings -Wsign-conversion -std=c99"
    # )
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_GXX_FLAGS}")
    add_definitions(
        -D_POSIX_C_SOURCE=200112L
        -D_DEFAULT_SOURCE
        -D_ISOC99_SOURCE
    )
endif()

option(TCS_ENABLE_TESTS "Enable tests" OFF)
option(TCS_ENABLE_EXAMPLES "Enable examples" OFF)
option(TCS_WARNINGS_AS_ERRORS "Enable treat warnings as errors" OFF)
option(TCS_GENERATE_COVERAGE "Enable for test coverage generation" OFF)

# ifaddrs crossplatform define
if (NOT MSVC)
    check_symbol_exists(getifaddrs "sys/types.h;ifaddrs.h" TCS_IFADDRS_FOUND)
    if(NOT TCS_IFADDRS_FOUND)
        MESSAGE("TCS warning: Symbol getifaddrs not found. Workaround will be applied.")
        add_definitions("-DTCS_MISSING_IFADDRS")
    endif()

    check_include_file("linux/if_packet.h" TCS_AF_PACKET_FOUND)
    if(NOT TCS_AF_PACKET_FOUND)
        MESSAGE("TCS warning: Header sys/types.h does not exist. AF_PACKET will not be available.")
        add_definitions("-DTCS_MISSING_AF_PACKET")
    endif()
endif()

set(TINYCSOCKET_SRC
    "src/tinycsocket_internal.h"
    "src/tinydatastructures.h"
    "src/tinycsocket_common.c"
    "src/tinycsocket_win32.c"
    "src/tinycsocket_posix.c"
)

# Header generation
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/tinycsocket.h
    COMMAND
        m4 ${CMAKE_CURRENT_SOURCE_DIR}/src/tinycsocket.h.m4 >
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tinycsocket.h
    DEPENDS ${TINYCSOCKET_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/src/tinycsocket.h.m4
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_custom_target(
    generate_header
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include/tinycsocket.h
)
set_target_properties(generate_header PROPERTIES FOLDER tinycsocket)

# Header interface
add_library(tinycsocket_header INTERFACE)
add_dependencies(tinycsocket_header generate_header)
target_include_directories(
    tinycsocket_header
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
if(WIN32)
    target_link_libraries(tinycsocket_header INTERFACE wsock32 ws2_32 iphlpapi)
endif()

# Tinycsocket static library
add_library(tinycsocket STATIC ${TINYCSOCKET_SRC})
add_library(tinycsockets ALIAS tinycsocket)
target_include_directories(tinycsocket PUBLIC include PRIVATE src)
target_link_libraries(tinycsocket PRIVATE tinycsocket_header)
set_target_properties(tinycsocket PROPERTIES FOLDER tinycsocket)

if(TCS_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(tinycsocket PRIVATE /W4 /WX)
    else()
        target_compile_options(tinycsocket PRIVATE -Werror)
    endif()
endif()

install(TARGETS tinycsocket DESTINATION lib)
install(FILES include/tinycsocket.h DESTINATION include)

# Extra targets for development
if(TCS_ENABLE_TESTS)
    enable_testing()
    add_library(tinycsocket_wrapped STATIC ${TINYCSOCKET_SRC} "src/dbg_wrap.h")
    target_include_directories(tinycsocket_wrapped PUBLIC include src)
    target_link_libraries(tinycsocket_wrapped INTERFACE tinycsocket_header)
    target_compile_options(tinycsocket_wrapped PUBLIC "-DDO_WRAP")
endif()

if(TCS_GENERATE_COVERAGE)
    if(MSVC)
        message(SEND_ERROR "MSVC with code coverage is not supported.")
    endif()
    target_compile_options(tinycsocket_wrapped PRIVATE --coverage)
    target_link_libraries(tinycsocket_wrapped PRIVATE --coverage)
endif()

if(TCS_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

if(TCS_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    # Define output directory for Doxygen documentation
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/docs")
    
    # Add custom target to build documentation
    add_custom_target(
        docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../docs
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    
    # Set the target folder for organization in IDEs
    set_target_properties(docs PROPERTIES FOLDER tinycsocket/docs)
    
    message(STATUS "Doxygen found: documentation target 'docs' is available")
else()
    message(STATUS "Doxygen not found: documentation target will not be available")
endif()
