file(READ "${SRC_DIR}/src/tinycsocket_internal.h" TINYCSOCKET_INTERNAL_H)
file(READ "${SRC_DIR}/src/tinydatastructures.h" TINYDATASTRUCTURES_H)
file(READ "${SRC_DIR}/src/tinycsocket_posix.c" TINYCSOCKET_POSIX_C)
file(READ "${SRC_DIR}/src/tinycsocket_win32.c" TINYCSOCKET_WIN32_C)
file(READ "${SRC_DIR}/src/tinycsocket_common.c" TINYCSOCKET_COMMON_C)

if(NOT OUTPUT)
    set(OUTPUT "${SRC_DIR}/include/tinycsocket.h")
endif()

configure_file("${SRC_DIR}/src/tinycsocket.h.in" "${OUTPUT}" @ONLY)
