#ifndef TINYDATASTRUCTURES_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Goals:
// Threadsafe
// Secure, no data leaks
// Tiny compile times
// Tiny generated file size (light on instruction cache)
// Optimized for "is there one, there might be many"
// Tiny code pollution
// Easy to debug

static int ulist_create(void** data, size_t init_capacity, size_t* capacity, size_t element_size);
static int ulist_reserve(void** data, size_t requested_capacity, size_t* capacity, size_t element_size);
static int ulist_create_copy(void* from_data,
                             size_t from_count,
                             void** to_data,
                             size_t* to_count,
                             size_t* to_capacity,
                             size_t element_size);
static int ulist_destroy(void** data);
static size_t ulist_best_capacity_fit(size_t n);
static int ulist_reserve(void** data, size_t requested_capacity, size_t* capacity, size_t element_size);
static int ulist_resize(void** data, size_t new_size, size_t* capacity, size_t* count, size_t element_size);
static int ulist_add(void** data,
                     void* add_data,
                     size_t add_count,
                     size_t* count,
                     size_t* capacity,
                     size_t element_size);
static int ulist_remove(void* data, size_t index, size_t remove_count, size_t* count, size_t element_size);
static int ulist_relax(void** data, size_t count, size_t* capacity);
static int ulist_pop(void* data, size_t* count, void* out_element, size_t element_size);

static int ulist_create(void** data, size_t init_capacity, size_t* capacity, size_t element_size)
{
    return ulist_reserve(data, init_capacity, capacity, element_size);
}

static int ulist_create_copy(void* from_data,
                             size_t from_count,
                             void** to_data,
                             size_t* to_count,
                             size_t* to_capacity,
                             size_t element_size)
{
    int sts = ulist_create(to_data, from_count, to_capacity, element_size);
    if (sts != 0)
        return sts;
    memcpy(from_data, *to_data, from_count * element_size);
    *to_count = from_count;
    return 0;
}

static int ulist_destroy(void** data)
{
    if (*data != NULL)
    {
        free(*data);
        *data = NULL;
    }
    return 0;
}

static size_t ulist_best_capacity_fit(size_t n)
{
    size_t best_fit = 8;
    size_t size_list[] = {
        8, 32, 128, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576};
    if (n >= size_list[14])
    {
        best_fit = ((n + size_list[14] - 1) / size_list[14]) * size_list[14];
    }
    else
    {
        for (size_t i = 0; i < 15; ++i)
        {
            if (n <= size_list[i])
            {
                best_fit = size_list[i];
                break;
            }
        }
    }
    return best_fit;
}

static int ulist_reserve(void** data, size_t requested_capacity, size_t* capacity, size_t element_size)
{
    if (requested_capacity <= *capacity)
        return 0;

    size_t new_capacity = ulist_best_capacity_fit(requested_capacity);

    void* new_data = realloc(*data, new_capacity * element_size);
    if (new_data == NULL)
        return -1;
    memset((uint8_t*)new_data + *capacity * element_size, 0, (new_capacity - *capacity) * element_size);
    *data = new_data;
    *capacity = new_capacity;
    return 0;
}

static int ulist_resize(void** data, size_t new_size, size_t* capacity, size_t* count, size_t element_size)
{
    ulist_reserve(data, new_size, capacity, element_size);
    if (new_size < *count)
        memset((uint8_t*)*data + new_size * element_size, 0, *count - new_size);
    *count = new_size;
    return 0;
}

static int ulist_add(void** data,
                     void* add_data,
                     size_t add_count,
                     size_t* count,
                     size_t* capacity,
                     size_t element_size)
{
    ulist_reserve(data, add_count * element_size, capacity, element_size);
    memcpy((uint8_t*)*data + *count * element_size, add_data, add_count * element_size);
    *count += add_count;
    return 0;
}

static int ulist_remove(void* data, size_t index, size_t remove_count, size_t* count, size_t element_size)
{
    if (index + remove_count > *count)
        return -1;
    void* dst = (uint8_t*)data + index * element_size;
    void* src = (uint8_t*)data + (*count - 1) * element_size * remove_count;
    memmove(dst, src, element_size * remove_count);
    *count -= remove_count;
    return 0;
}

static int ulist_relax(void** data, size_t count, size_t* capacity)
{
    size_t suggested_capacity = ulist_best_capacity_fit(count);
    if (suggested_capacity < *capacity)
    {
        void* new_data = realloc(*data, suggested_capacity);
        if (new_data == NULL)
            return -1;
        *data = new_data;
        *capacity = suggested_capacity;
    }
    return 0;
}

static int ulist_pop(void* data, size_t* count, void* out_element, size_t element_size)
{
    if (*count < 1)
        return -1;
    if (out_element != NULL)
        memmove(out_element, (uint8_t*)data + *count * element_size, element_size); // allow to pop to another element
    memset((uint8_t*)data + *count * element_size, 0, element_size);
    (*count)--;
    return 0;
}

#define TDS_ULIST_IMPL(TYPE, NAME)                                                                          \
                                                                                                            \
    typedef struct                                                                                          \
    {                                                                                                       \
        TYPE* data;                                                                                         \
        size_t count;                                                                                       \
        size_t capacity;                                                                                    \
    } UList_##NAME;                                                                                         \
                                                                                                            \
    static inline int ulist_##NAME##_create(UList_##NAME* ulist, size_t init_count)                         \
    {                                                                                                       \
        memset(ulist, 0, sizeof(*ulist));                                                                   \
        return ulist_create((void**)&ulist->data, init_count, &ulist->capacity, sizeof(TYPE));              \
    }                                                                                                       \
    static inline int ulist_##NAME##_create_copy(UList_##NAME* from, UList_##NAME* to)                      \
    {                                                                                                       \
        return ulist_create_copy(                                                                           \
            &from->data, from->count, (void**)&to->data, &to->count, &to->capacity, sizeof(TYPE));          \
    }                                                                                                       \
    static inline int ulist_##NAME##_destroy(UList_##NAME* ulist)                                           \
    {                                                                                                       \
        ulist_destroy((void**)&ulist->data);                                                                \
        memset(ulist, 0, sizeof(*ulist));                                                                   \
        return 0;                                                                                           \
    }                                                                                                       \
    static inline int ulist_##NAME##_reserve(UList_##NAME* ulist, size_t new_capacity)                      \
    {                                                                                                       \
        return ulist_reserve((void**)&ulist->data, new_capacity, &ulist->capacity, sizeof(TYPE));           \
    }                                                                                                       \
    static inline int ulist_##NAME##_resize(UList_##NAME* ulist, size_t new_size)                           \
    {                                                                                                       \
        return ulist_resize((void**)&ulist->data, new_size, &ulist->capacity, &ulist->count, sizeof(TYPE)); \
    }                                                                                                       \
    static inline int ulist_##NAME##_add_one(UList_##NAME* ulist, TYPE data)                                \
    {                                                                                                       \
        return ulist_add((void**)&ulist->data, &data, 1, &ulist->count, &ulist->capacity, sizeof(TYPE));    \
    }                                                                                                       \
    static inline int ulist_##NAME##_add(UList_##NAME* ulist, TYPE* data, size_t count)                     \
    {                                                                                                       \
        return ulist_add((void**)&ulist->data, data, count, &ulist->count, &ulist->capacity, sizeof(TYPE)); \
    }                                                                                                       \
    static inline int ulist_##NAME##_remove_one(UList_##NAME* ulist, size_t index)                          \
    {                                                                                                       \
        return ulist_remove(&ulist->data, index, 1, &ulist->count, sizeof(TYPE));                           \
    }                                                                                                       \
    static inline int ulist_##NAME##_remove(UList_##NAME* ulist, size_t index, size_t remove_count)         \
    {                                                                                                       \
        return ulist_remove(&ulist->data, index, remove_count, &ulist->count, sizeof(TYPE));                \
    }                                                                                                       \
    static inline int ulist_##NAME##_pop(UList_##NAME* ulist, TYPE* popped_element)                         \
    {                                                                                                       \
        return ulist_pop(&ulist->data, &ulist->count, popped_element, sizeof(TYPE));                        \
    }                                                                                                       \
    static inline int ulist_##NAME##_relax(UList_##NAME* ulist)                                             \
    {                                                                                                       \
        return ulist_relax((void**)&ulist->data, ulist->count, &ulist->capacity);                           \
    }
#endif
