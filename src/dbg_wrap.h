/*
 * Copyright 2018 Markus Lindelöw
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
// This file is used to test for memory leaks in our tests.
#ifndef TCS_WRAP_H_
#define TCS_WRAP_H_

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
