// This file will detect and include the correct implementation
#if defined(__linux__) || defined(__sun) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__APPLE__)
#define __POSIX__
#endif

// Common code goes here

// Platform specific code is included here
#if defined(WIN32)
#include "tinycsocket_win32.c"
#elif defined(__POSIX__)
#include "tinycsocket_posix.c"
#else
#pragma message("Warning: Unknown OS, trying POSIX")
#include "tinycsocket_posix.c"
#endif