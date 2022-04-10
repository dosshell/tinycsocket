#ifndef WRAP_H_
#define WRAP_H_

#ifdef malloc
#error You need to include this file before <corecrt_malloc.h> and not include <crtdbg.h>.
#endif

#ifdef __cplusplus
extern "C" {
#endif
void* __wrap_malloc(size_t size, char const* file_name, int line_number);
void* __wrap_realloc(void* ptr, size_t size, char const* file_name, int line_number);
void __wrap_free(void* ptr);
#ifdef __cplusplus
}
#endif

#ifdef _MSC_VER
#pragma push_macro("free")
#pragma push_macro("malloc")
#pragma push_macro("realloc")
#undef free
#undef malloc
#undef realloc
#include <malloc.h>
#pragma pop_macro("realloc")
#pragma pop_macro("malloc")
#pragma pop_macro("free")
#else
#include <stdlib.h>
#endif

#define malloc(s) __wrap_malloc(s, __FILE__, __LINE__)
#define free(p) __wrap_free(p)
#define realloc(p, s) __wrap_realloc(p, s, __FILE__, __LINE__)

#endif
