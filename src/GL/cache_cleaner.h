#ifndef __CACHE_CLEANER_H
#define __CACHE_CLEANER_H

#include <stdint.h>

typedef intptr_t cache_id_t;
typedef uint32_t cache_timestamp_t;
typedef uint32_t cache_duration_t;

typedef void (*clean_cached_item_fn_t)(void*);

typedef struct cached_item_s cached_item_t;

typedef struct cached_item_s {
    void *context;
    clean_cached_item_fn_t clean_callback;
    cache_timestamp_t last_used_timestamp;
    cached_item_t *next;
} cached_item_t;

typedef struct cache_cleaner_s {
    cached_item_t *head;
    cache_duration_t evict_after;
} cache_cleaner_t;

#ifdef __cplusplus
extern "C" {
#endif

void cache_cleaner_init(cache_cleaner_t *cleaner, cache_duration_t evict_after);
void cache_cleaner_destroy(cache_cleaner_t *cleaner);

cache_id_t cache_cleaner_register(cache_cleaner_t *cleaner, void *context, clean_cached_item_fn_t callback);
void cache_cleaner_unregister(cache_cleaner_t *cleaner, cache_id_t id);

void cache_cleaner_touch(cache_cleaner_t *cleaner, cache_id_t id);

void cache_cleaner_cleanup(cache_cleaner_t *cleaner);

#ifdef __cplusplus
}
#endif

#endif
