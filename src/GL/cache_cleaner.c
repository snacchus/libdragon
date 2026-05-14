#include "cache_cleaner.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

void cache_cleaner_init(cache_cleaner_t *cleaner, cache_duration_t evict_after)
{
    cleaner->head = NULL;
    cleaner->evict_after = evict_after;
}

void cache_cleaner_destroy(cache_cleaner_t *cleaner)
{
    cached_item_t *item = cleaner->head;
    while (item != NULL) {
        cached_item_t *next = item->next;
        free(item);
        item = next;
    }
}

static cache_timestamp_t get_current_timestamp()
{
    // TODO
}

cache_id_t cache_cleaner_register(cache_cleaner_t *cleaner, void *context, clean_cached_item_fn_t callback)
{
    cached_item_t *new_item = malloc(sizeof(cached_item_t));
    new_item->context = context;
    new_item->clean_callback = callback;
    new_item->last_used_timestamp = get_current_timestamp();
    new_item->next = cleaner->head;

    cleaner->head = new_item;
    return (cache_id_t)new_item;
}

void cache_cleaner_unregister(cache_cleaner_t *cleaner, cache_id_t id)
{
    cached_item_t **item = &cleaner->head;
    while (*item != NULL && (cache_id_t)*item != id) {
        item = &(*item)->next;
    }

    if (*item == NULL) return;

    cached_item_t *current = *item;
    *item = current->next;
    free(current);
}

void cache_cleaner_touch(cache_cleaner_t *cleaner, cache_id_t id)
{
    cached_item_t *item = (cached_item_t*)id;
    item->last_used_timestamp = get_current_timestamp();
}

static bool is_expired(cache_cleaner_t *cleaner, cached_item_t *item, cache_timestamp_t current_timestamp)
{
    return (current_timestamp - item->last_used_timestamp) > cleaner->evict_after;
}

static void invoke_callback(cached_item_t *item)
{
    item->clean_callback(item->context);
}

void cache_cleaner_cleanup(cache_cleaner_t *cleaner)
{
    cache_timestamp_t current_timestamp = get_current_timestamp();

    cached_item_t **item = &cleaner->head;
    while (*item != NULL) {
        cached_item_t *current = *item;
        if (is_expired(cleaner, current, current_timestamp)) {
            invoke_callback(current);
            *item = current->next;
            item = &current->next;
            free(current);
        } else {
            item = &current->next;
        }
    }
}
