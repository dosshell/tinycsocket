cmake_minimum_required (VERSION 2.8)
project (tinycsocket)

# Add warning levels for gcc and msvc
if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W3" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Winit-self -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wconversion -Wshadow -Wundef -Wwrite-strings -Wswitch-enum -Wswitch-default")
endif()

add_subdirectory (src)
add_subdirectory (tests)
add_subdirectory (example)
