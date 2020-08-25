# Tests
add_executable(tests tests.cpp)
find_package(Threads REQUIRED)
if (NOT MSVC)
  target_compile_options(tests PRIVATE -std=c++11)
endif ()
target_link_libraries(tests LINK_PUBLIC tinycsocket Threads::Threads)

set_target_properties(tests PROPERTIES FOLDER TCS/tests)
