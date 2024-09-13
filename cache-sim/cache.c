#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cache.h"
#include "print_helpers.h"

cache_t *make_cache(int capacity, int block_size, int assoc, enum protocol_t protocol, bool lru_on_invalidate_f) {
    cache_t *cache = malloc(sizeof(cache_t));
    cache->stats = make_cache_stats();

    cache->capacity = capacity;     // in Bytes
    cache->block_size = block_size; // in Bytes
    cache->assoc = assoc;           // 1, 2, 3... etc.
    cache->n_cache_line = capacity / block_size;
    cache->n_set = capacity / (assoc * block_size);
    cache->n_offset_bit = log2(block_size);
    cache->n_index_bit = log2(capacity) - log2(assoc) - log2(block_size);
    cache->n_tag_bit = ADDRESS_SIZE - cache->n_offset_bit - cache->n_index_bit;

    // next create the cache lines and the array of LRU bits
    // - malloc an array with n_rows
    // - for each element in the array, malloc another array with n_col

    cache->lines = malloc(cache->n_set * sizeof(cache_line_t *));
    for (int i = 0; i < cache->n_set; i++) {
        cache->lines[i] = malloc(cache->assoc * sizeof(cache_line_t));
    }

    cache->lru_way = malloc(cache->n_set * sizeof(int));

    // initializes cache tags to 0, dirty bits to false,
    // state to INVALID, and LRU bits to 0
    for (int i = 0; i < cache->n_set; i++) {
        for (int j = 0; j < cache->assoc; j++) {
            cache->lines[i][j].tag = 0;
            cache->lines[i][j].dirty_f = false;
            cache->lines[i][j].state = INVALID;
            cache->lru_way[i] = 0;
        }
    }

    cache->protocol = protocol;
    cache->lru_on_invalidate_f = lru_on_invalidate_f;

    return cache;
}

/* Given a configured cache, returns the tag portion of the given address.
 *
 * Example: a cache with 4 bits each in tag, index, offset
 * in binary -- get_cache_tag(0b111101010001) returns 0b1111
 * in decimal -- get_cache_tag(3921) returns 15
 */
unsigned long get_cache_tag(cache_t *cache, unsigned long addr) {
    return addr >> (cache->n_offset_bit + cache->n_index_bit);
}

/* Given a configured cache, returns the index portion of the given address.
 *
 * Example: a cache with 4 bits each in tag, index, offset
 * in binary -- get_cache_index(0b111101010001) returns 0b0101
 * in decimal -- get_cache_index(3921) returns 5
 */
unsigned long get_cache_index(cache_t *cache, unsigned long addr) {
    unsigned long index_bits = addr >> cache->n_offset_bit;
    unsigned long index_mask = (1 << cache->n_index_bit) - 1;
    return index_bits & index_mask;
}

/* Given a configured cache, returns the given address with the offset bits zeroed out.
 *
 * Example: a cache with 4 bits each in tag, index, offset
 * in binary -- get_cache_block_addr(0b111101010001) returns 0b111101010000
 * in decimal -- get_cache_block_addr(3921) returns 3920
 */
unsigned long get_cache_block_addr(cache_t *cache, unsigned long addr) {
    unsigned long block_mask = ~((1 << cache->n_offset_bit) - 1);
    return addr & block_mask;
}

bool access_cache_msi(cache_t *cache, int tag, int index, enum action_t action) {

    for (int i = 0; i < cache->assoc; i++) {     // search cache
        if (cache->lines[index][i].tag == tag) { // exists in cache
            log_way(i);

            if (action == LOAD || action == STORE) { // ignore snoops
                cache->lru_way[index] = (i + 1) % cache->assoc;
                if (action == STORE && !cache->lines[index][i].dirty_f) {
                    cache->lines[index][i].dirty_f = true;
                }
            }

            switch (cache->lines[index][i].state) {
            case MODIFIED:
                update_stats(cache->stats, true, cache->lines[index][i].dirty_f && (action == LD_MISS || action == ST_MISS), false, action);
                if (action == LD_MISS) {
                    cache->lines[index][i].state = SHARED;
                } else if (action == ST_MISS) {
                    cache->lines[index][i].state = INVALID;
                }
                return true;
            case SHARED:
                update_stats(cache->stats, true, false, action == STORE, action); // upgrade miss
                if (action == STORE) {                                            // store hit
                    cache->lines[index][i].state = MODIFIED;
                } else if (action == ST_MISS) {
                    cache->lines[index][i].state = INVALID;
                }
                return true;
            case INVALID:
                update_stats(cache->stats, false, false, false, action);
                if (action == LOAD) { // load hit
                    cache->lines[index][i].state = SHARED;
                } else if (action == STORE) { // store hit
                    cache->lines[index][i].state = MODIFIED;
                }
                return false;
            case VALID:
                break;
            }
        }
    }

    // cache miss
    if (action == LD_MISS || action == ST_MISS) { // ignore snoops
        update_stats(cache->stats, false, false, false, action);
        return false;
    }
    int lru_way = cache->lru_way[index];
    update_stats(cache->stats, false, cache->lines[index][lru_way].dirty_f, false, action);
    cache->lines[index][lru_way].tag = tag;
    cache->lines[index][lru_way].dirty_f = action == STORE;
    cache->lines[index][lru_way].state = (action == STORE) ? MODIFIED : ((action == LOAD) ? SHARED : INVALID);
    cache->lru_way[index] = (lru_way + 1) % cache->assoc;

    return false;
}

/* This method takes a cache, an address, and an action
 * it proceses the cache access. functionality in no particular order:
 *   - look up the address in the cache, determine if hit or miss
 *   - update the LRU_way, cacheTags, state, dirty flags if necessary
 *   - update the cache statistics (call update_stats)
 * return true if there was a hit, false if there was a miss
 * Use the "get" helper functions above. They make your life easier.
 */
bool access_cache(cache_t *cache, unsigned long addr, enum action_t action) {

    unsigned long tag = get_cache_tag(cache, addr);
    unsigned long index = get_cache_index(cache, addr);

    log_set(index);

    if (cache->protocol == MSI) {
        return access_cache_msi(cache, tag, index, action);
    }

    // cache hit
    for (int i = 0; i < cache->assoc; i++) {
        if (cache->lines[index][i].tag == tag) {
            if (cache->lines[index][i].state == VALID) {
                log_way(i);

                // ignore snoops
                if (action == LOAD || action == STORE) {
                    cache->lru_way[index] = (i + 1) % cache->assoc;
                    update_stats(cache->stats, true, false, false, action);

                    if (action == STORE && !cache->lines[index][i].dirty_f) {
                        cache->lines[index][i].dirty_f = true;
                    }
                }

                // snoops
                if (cache->protocol == VI && (action == LD_MISS || action == ST_MISS)) {
                    cache->lines[index][i].state = INVALID;
                    if (cache->lines[index][i].dirty_f) { // writeback
                        cache->lines[index][i].dirty_f = false;
                        update_stats(cache->stats, false, true, false, action);
                    }
                }

                return true;
            } else {
                if (cache->protocol == VI && (action == LOAD || action == STORE)) {
                    cache->lines[index][i].state = VALID;
                }
            }
        }
    }

    // cache miss
    if (action == LD_MISS || action == ST_MISS) { // ignore snoops
        update_stats(cache->stats, false, false, false, action);
        return false;
    }

    // writeback if line is dirty
    int lru_way = cache->lru_way[index];
    update_stats(cache->stats, false, cache->lines[index][lru_way].dirty_f, false, action);

    cache->lines[index][lru_way].tag = tag;
    cache->lines[index][lru_way].dirty_f = action == STORE;
    cache->lines[index][lru_way].state = VALID;
    cache->lru_way[index] = (lru_way + 1) % cache->assoc;

    return false;
}