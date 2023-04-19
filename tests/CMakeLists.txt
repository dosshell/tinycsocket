# Tests
add_executable(tests "tests.cpp" "mock.cpp" "mock.h")
if (NOT MSVC)
  target_compile_options(tests PUBLIC -std=c++11)
endif ()

target_compile_options(tests PUBLIC "-DDO_WRAP")

if (MINGW)
  target_link_libraries(tests PRIVATE -std=c++11 -fPIC -static -static-libgcc -static-libstdc++)
endif ()
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    find_package(Threads REQUIRED)
    target_link_libraries(tests PRIVATE Threads::Threads)
endif ()
target_link_libraries(tests PRIVATE tinycsocket_wrapped)

set_target_properties(tests PROPERTIES FOLDER TCS/tests)
add_definitions(-DDOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING)

# Header only
# Tests
add_executable(test_header_inline "header_only/a.c" "header_only/b.c")
target_link_libraries(test_header_inline PRIVATE tinycsocket)
# target_include_directories(test_header_inline PRIVATE ../include)

if (NOT MSVC)
  target_compile_options(test_header_inline PUBLIC -std=c99)
endif ()

if (MINGW)
  target_link_libraries(test_header_inline PRIVATE -std=c99 -fPIC -static -static-libgcc -static-libstdc++)
endif ()

set_target_properties(test_header_inline PROPERTIES FOLDER TCS/tests/header_only)
