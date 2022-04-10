# Tests
add_executable(tests "tests.cpp" "mock.cpp" "mock.h")
find_package(Threads REQUIRED)
if (NOT MSVC)
  target_compile_options(tests PUBLIC -std=c++11)
endif ()

target_compile_options(tests PUBLIC "-DDO_WRAP")

if (MINGW)
  target_link_libraries(tests PRIVATE -std=c++11 -fPIC -static -static-libgcc -static-libstdc++)
endif ()

target_link_libraries(tests PUBLIC tinycsocket_wrapped Threads::Threads)

set_target_properties(tests PROPERTIES FOLDER TCS/tests)
add_definitions(-DDOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING)
