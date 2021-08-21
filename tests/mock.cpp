#include "mock.h"

#include <stdint.h>
#include <stdio.h>
#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define MAYBE_UNUSED
#else
#define _malloc_dbg(s, b, f, l) malloc(s)
#define _free_dbg(p, b) free(p)
#define _realloc_dbg(p, s, t, f, l) realloc(p, s)
#define MAYBE_UNUSED __attribute__((unused))
#endif
#include <stdlib.h>

extern "C" {

int MOCK_ALLOC_COUNTER = 0;
int MOCK_FREE_COUNTER = 0;
int MOCK_ALLOC_FAIL_AFTER = MOCK_FAIL_OFF; // -1 for off

void* __wrap_malloc(size_t size, char const* filename MAYBE_UNUSED, int line_number MAYBE_UNUSED)
{
    MOCK_ALLOC_COUNTER++;

    if (MOCK_ALLOC_FAIL_AFTER == 0)
    {
        MOCK_ALLOC_FAIL_AFTER = MOCK_FAIL_OFF;
        return NULL;
    }
    if (MOCK_ALLOC_FAIL_AFTER > 0)
        MOCK_ALLOC_FAIL_AFTER--;

    return _malloc_dbg(size, 1, filename, line_number);
}

void* __wrap_realloc(void* ptr, size_t size, char const* filename MAYBE_UNUSED, int line_number MAYBE_UNUSED)
{
    if (size == 0)
    {
#ifdef _MSC_VER
        __debugbreak(); // UB-isch, or worse
#endif
    }
    if (ptr == nullptr)
    {
        MOCK_ALLOC_COUNTER++;
    }

    return _realloc_dbg(ptr, size, 1, filename, line_number);
}

void __wrap_free(void* ptr)
{
    if (ptr)
    {
        MOCK_FREE_COUNTER++;
        if (MOCK_ALLOC_COUNTER - MOCK_FREE_COUNTER < 0)
        {
#ifdef _MSC_VER
            __debugbreak(); // Called free more than what was allocated
#endif
        }
    }
    _free_dbg(ptr, 1);
}
}
