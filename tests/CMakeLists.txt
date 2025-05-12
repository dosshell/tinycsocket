# Tests
include(CTest)

add_executable(tests "tests.cpp" "mock.cpp" "mock.h")
target_compile_options(tests PUBLIC "-DDO_WRAP")
target_link_libraries(tests PRIVATE tinycsocket_wrapped)
set_target_properties(tests PROPERTIES FOLDER tinycsocket/tests)
add_definitions(-DDOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING)
if(MINGW)
    target_link_libraries(
        tests
        PRIVATE -std=c++11 -fPIC -static -static-libgcc -static-libstdc++
    )
endif()
if(NOT MSVC)
    target_compile_options(tests PUBLIC -std=c++11)
endif()
# Android does not need to link to any threads. It does not follow posix.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    find_package(Threads REQUIRED)
    target_link_libraries(tests PRIVATE Threads::Threads)
endif()

# Header only
# translation unit tests
add_executable(test_translation_units "header_only/a.c" "header_only/b.c")
target_link_libraries(test_translation_units PRIVATE tinycsocket_header)
if(NOT MSVC)
    target_compile_options(test_translation_units PUBLIC -std=c99)
endif()

if(MINGW)
    target_link_libraries(
        test_translation_units
        PRIVATE -std=c99 -fPIC -static -static-libgcc -static-libstdc++
    )
endif()

set_target_properties(
    test_translation_units
    PROPERTIES FOLDER tinycsocket/tests/header_only
)

# Tests but as header only implementation
add_executable(
    tests_header_only
    "mock.cpp"
    "mock.h"
    "header_only/tests_header_only.cpp"
    "../src/dbg_wrap.h"
)
target_sources(
    tests_header_only
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tests.cpp
)
target_include_directories(tests_header_only PRIVATE "../src/")
if(NOT MSVC)
    target_compile_options(tests_header_only PUBLIC -std=c++11)
endif()
if(MINGW)
    target_link_libraries(
        tests_header_only
        PRIVATE -std=c++11 -fPIC -static -static-libgcc -static-libstdc++
    )
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    find_package(Threads REQUIRED)
    target_link_libraries(tests_header_only PRIVATE Threads::Threads)
endif()
target_compile_options(tests_header_only PUBLIC "-DDO_WRAP")
target_link_libraries(tests_header_only PRIVATE tinycsocket_header)
target_include_directories(
    tests_header_only
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(tests_header_only PROPERTIES FOLDER tinycsocket/tests)
add_definitions(-DDOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING)

add_test(NAME tests COMMAND tests)
add_test(NAME tests_header_only COMMAND tests_header_only)
add_test(NAME test_translation_units COMMAND test_translation_units)
