#ifndef TINYDATASTRUCTURES_H_
#define TINYDATASTRUCTURES_H_

#include <stdlib.h>
#include <string.h>

static inline int tds_ulist_create(void** data, size_t* count, size_t* capacity, size_t element_size);
static inline int tds_ulist_destroy(void** data, size_t* count, size_t* capacity);
static inline int tds_ulist_reserve(void** data, size_t* capacity, size_t element_size, size_t requested_capacity);
static inline int tds_ulist_add(void** data,
                                size_t* count,
                                size_t* capacity,
                                size_t element_size,
                                void* add_data,
                                size_t add_count);
static inline int tds_ulist_remove(void** data,
                                   size_t* count,
                                   size_t* capacity,
                                   size_t element_size,
                                   size_t remove_from,
                                   size_t remove_count);

static inline int tds_ulist_create(void** data, size_t* count, size_t* capacity, size_t element_size)
{
    *data = NULL;
    *count = 0;
    *capacity = 0;
    if (element_size == 0)
        return -1;
    return tds_ulist_reserve(data, capacity, element_size, 1);
}

static inline int tds_ulist_destroy(void** data, size_t* count, size_t* capacity)
{
    if (*data != NULL)
    {
        free(*data);
        *data = NULL;
    }
    *count = 0;
    *capacity = 0;
    return 0;
}

// TODO: move to reserve
static inline size_t tds_ulist_best_capacity_fit(size_t old_capacity, size_t new_capacity)
{
    const size_t MINIMUM_CAPACITY = 8;

    size_t c = MINIMUM_CAPACITY;
    while (c < new_capacity)
        c *= 2;

    // Hysteresis
    if (c * 2 == old_capacity)
        return old_capacity;

    return c;
}

static inline int tds_ulist_reserve(void** data, size_t* capacity, size_t element_size, size_t requested_capacity)
{
    size_t new_capacity = tds_ulist_best_capacity_fit(*capacity, requested_capacity);
    if (new_capacity == *capacity)
        return 0;

// UB protection for C23 and implemention defined protection before C23 (Should never happen)
#ifndef NDEBUG
    if (new_capacity == 0)
        return -1;
#endif

    void* new_data = realloc(*data, new_capacity * element_size);
    if (new_data == NULL)
        return -1;

    if (new_capacity > *capacity)
    {
        memset((char*)new_data + *capacity * element_size, 0, (new_capacity - *capacity) * element_size);
    }
    *data = new_data;
    *capacity = new_capacity;
    return 0;
}

static inline int tds_ulist_add(void** data,
                                size_t* count,
                                size_t* capacity,
                                size_t element_size,
                                void* add_data,
                                size_t add_count)
{
    if (*count + add_count > *capacity)
    {
        int reserve_sts = tds_ulist_reserve(data, capacity, element_size, *count + add_count);
        if (reserve_sts != 0)
            return reserve_sts;
    }

    memcpy((char*)*data + (*count * element_size), add_data, add_count * element_size);
    *count += add_count;
    return 0;
}

static inline int tds_ulist_remove(void** data,
                                   size_t* count,
                                   size_t* capacity,
                                   size_t element_size,
                                   size_t remove_from,
                                   size_t remove_count)
{
    if (remove_from >= *count || remove_count == 0 || remove_from + remove_count > *count)
        return -1;

    void* dst = (char*)(*data) + (remove_from * element_size);
    void* src = (char*)(*data) + (*count - remove_count) * element_size;
    memmove(dst, src, element_size * remove_count);
    *count -= remove_count;

    if (*count < *capacity / 2)
    {
        int reserve_sts = tds_ulist_reserve(data, capacity, element_size, *count);
        if (reserve_sts != 0)
            return reserve_sts;
    }

    return 0;
}

#define TDS_ULIST_IMPL(TYPE, NAME)                                                                                     \
    struct TdsUList_##NAME                                                                                             \
    {                                                                                                                  \
        TYPE* data;                                                                                                    \
        size_t count;                                                                                                  \
        size_t capacity;                                                                                               \
    };                                                                                                                 \
                                                                                                                       \
    static inline int tds_ulist_##NAME##_create(struct TdsUList_##NAME* ulist)                                         \
    {                                                                                                                  \
        return tds_ulist_create((void**)&ulist->data, &ulist->count, &ulist->capacity, sizeof(TYPE));                  \
    }                                                                                                                  \
    static inline int tds_ulist_##NAME##_destroy(struct TdsUList_##NAME* ulist)                                        \
    {                                                                                                                  \
        int sts = tds_ulist_destroy((void**)&ulist->data, &ulist->count, &ulist->capacity);                            \
        memset(ulist, 0, sizeof(*ulist));                                                                              \
        return sts;                                                                                                    \
    }                                                                                                                  \
    static inline int tds_ulist_##NAME##_add(struct TdsUList_##NAME* ulist, TYPE* data, size_t count)                  \
    {                                                                                                                  \
        return tds_ulist_add((void**)&ulist->data, &ulist->count, &ulist->capacity, sizeof(TYPE), (void*)data, count); \
    }                                                                                                                  \
    static inline int tds_ulist_##NAME##_remove(                                                                       \
        struct TdsUList_##NAME* ulist, size_t remove_from, size_t remove_count)                                        \
    {                                                                                                                  \
        return tds_ulist_remove(                                                                                       \
            (void**)&ulist->data, &ulist->count, &ulist->capacity, sizeof(TYPE), remove_from, remove_count);           \
    }                                                                                                                  \
    static inline int tds_ulist_##NAME##_reserve(struct TdsUList_##NAME* ulist, size_t new_capacity)                   \
    {                                                                                                                  \
        return tds_ulist_reserve((void**)&ulist->data, &ulist->capacity, sizeof(TYPE), new_capacity);                  \
    }

// Tiny Data Structures Map Implementation

static inline int tds_map_create(void** keys,
                                 void** values,
                                 size_t* count,
                                 size_t* capacity,
                                 size_t key_element_size,
                                 size_t value_element_size)
{
    size_t key_capacity = 0;
    size_t key_count = 0;

    size_t value_capacity = 0;
    size_t value_count = 0;

    int key_sts = tds_ulist_create(keys, &key_count, &key_capacity, key_element_size);
    int value_sts = tds_ulist_create(values, &value_count, &value_capacity, value_element_size);

    if (key_sts != 0 || value_sts != 0)
    {
        tds_ulist_destroy(keys, &value_count, &value_capacity);
        tds_ulist_destroy(values, &key_count, &key_capacity);
        if (key_sts != 0)
            return key_sts;
        if (value_sts != 0)
            return value_sts;
    }
    if (key_capacity != value_capacity)
    {
        tds_ulist_destroy(keys, &value_count, &value_capacity);
        tds_ulist_destroy(values, &key_count, &key_capacity);
        return -1;
    }
    *capacity = key_capacity;
    *count = key_count;

    return 0;
}

static inline int tds_map_destroy(void** keys, void** values, size_t* count, size_t* capacity)
{
    size_t key_capacity = 0;
    size_t key_count = 0;

    size_t value_capacity = 0;
    size_t value_count = 0;

    int key_sts = tds_ulist_destroy(keys, &key_count, &key_capacity);
    int value_sts = tds_ulist_destroy(values, &value_count, &value_capacity);
    *count = 0;
    *capacity = 0;
    if (key_sts != 0)
        return key_sts;
    if (value_sts != 0)
        return value_sts;
    return 0;
}

static inline int tds_map_add(void** keys,
                              void** values,
                              size_t* count,
                              size_t* capacity,
                              size_t key_element_size,
                              size_t value_element_size,
                              void* key_add,
                              void* value_add)
{
    size_t value_count = *count;
    size_t key_count = *count;
    size_t key_capacity = *capacity;
    size_t value_capacity = *capacity;
    int key_sts = tds_ulist_add(keys, &key_count, &key_capacity, key_element_size, key_add, 1);
    int value_sts = tds_ulist_add(values, &value_count, &value_capacity, value_element_size, value_add, 1);
    if (key_sts != 0 || value_sts != 0)
    {
        // TODO: fix invariant memory state. Restore memory capacity should work most of the time.
        // This is invariant is still non-fatal though, we may use more memory than needed.
        return -1;
    }
    *count += 1;
    *capacity = key_capacity <= value_capacity ? key_capacity : value_capacity; // min(key_capacity, value_capacity);

    if (key_capacity != value_capacity)
        return -1;

    return 0;
}

static inline int tds_map_remove(void** keys,
                                 void** values,
                                 size_t* count,
                                 size_t* capacity,
                                 size_t key_element_size,
                                 size_t value_element_size,
                                 size_t index)
{
    size_t value_count = *count;
    size_t key_count = *count;
    size_t key_capacity = *capacity;
    size_t value_capacity = *capacity;
    if (index >= *count)
        return -1;

    int key_sts = tds_ulist_remove(keys, &key_count, &key_capacity, key_element_size, index, 1);
    int value_sts = tds_ulist_remove(values, &value_count, &value_capacity, value_element_size, index, 1);
    if (key_sts != 0 || value_sts != 0)
    {
        // -2 indicates we are in a very bad situation and the data structure may be corrupted.
        // This should not be able to happen, with current implementation, but if it does, we need to be able to recover from it.
        return -2;
    }
    if (key_count != value_count)
        return -2;
    *count = key_count;
    *capacity = key_capacity;
    return 0;
}

#define TDS_MAP_IMPL(KEY_TYPE, VALUE_TYPE, NAME)                                                          \
                                                                                                          \
    struct TdsMap_##NAME                                                                                  \
    {                                                                                                     \
        KEY_TYPE* keys;                                                                                   \
        VALUE_TYPE* values;                                                                               \
        size_t count;                                                                                     \
        size_t capacity;                                                                                  \
    };                                                                                                    \
                                                                                                          \
    static inline int tds_map_##NAME##_create(struct TdsMap_##NAME* map)                                  \
    {                                                                                                     \
        memset(map, 0, sizeof(TdsMap_##NAME));                                                            \
        return tds_map_create((void**)&map->keys,                                                         \
                              (void**)&map->values,                                                       \
                              &map->count,                                                                \
                              &map->capacity,                                                             \
                              sizeof(KEY_TYPE),                                                           \
                              sizeof(VALUE_TYPE));                                                        \
    }                                                                                                     \
    static inline int tds_map_##NAME##_destroy(struct TdsMap_##NAME* map)                                 \
    {                                                                                                     \
        int sts = tds_map_destroy((void**)&map->keys, (void**)&map->values, &map->count, &map->capacity); \
        if (sts != 0)                                                                                     \
            return sts;                                                                                   \
        memset(map, 0, sizeof(TdsMap_##NAME));                                                            \
        return 0;                                                                                         \
    }                                                                                                     \
    static inline int tds_map_##NAME##_add(struct TdsMap_##NAME* map, KEY_TYPE key, VALUE_TYPE value)     \
    {                                                                                                     \
        return tds_map_add((void**)&map->keys,                                                            \
                           (void**)&map->values,                                                          \
                           &map->count,                                                                   \
                           &map->capacity,                                                                \
                           sizeof(KEY_TYPE),                                                              \
                           sizeof(VALUE_TYPE),                                                            \
                           &key,                                                                          \
                           &value);                                                                       \
    }                                                                                                     \
    static inline int tds_map_##NAME##_remove(struct TdsMap_##NAME* map, size_t index)                    \
    {                                                                                                     \
        return tds_map_remove((void**)&map->keys,                                                         \
                              (void**)&map->values,                                                       \
                              &map->count,                                                                \
                              &map->capacity,                                                             \
                              sizeof(KEY_TYPE),                                                           \
                              sizeof(VALUE_TYPE),                                                         \
                              index);                                                                     \
    }

#endif
