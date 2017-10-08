#ifndef IMPLEMENTATION_DETECTOR_H_
#define IMPLEMENTATION_DETECTOR_H_

#if defined(WIN32)
#define USE_WIN32_IMPL
#elif defined(__linux__) || defined(__sun) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__APPLE__)
#define USE_POSIX_IMPL
#else
#pragma message("Warning: Unknown OS, trying POSIX")
#define USE_POSIX_IMPL
#endif

#endif