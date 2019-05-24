# Tests
add_executable (tests tests.cpp)
find_package (Threads REQUIRED)
target_link_libraries (tests LINK_PUBLIC tinycsocket Threads::Threads)
