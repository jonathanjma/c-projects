#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"
#include "hashtable.h"

struct hashtable
{
    // Define hashtable struct to use linkedlist as buckets
    linkedlist_t **buckets;
    unsigned int num_buckets;
    unsigned int size;
};

/**
 * Hash function to hash a key into the range [0, max_range)
 */
static int hash(int key, int max_range)
{
    key = (key > 0) ? key : -key;
    return key % max_range;
}

hashtable_t *ht_init(int num_buckets)
{
    // Create a new hashtable
    hashtable_t *table = malloc(sizeof(hashtable_t));

    table->buckets = malloc(num_buckets * sizeof(linkedlist_t *));
    table->num_buckets = num_buckets;
    table->size = 0;

    // Initialize the buckets
    for (size_t i = 0; i < num_buckets; i++)
    {
        table->buckets[i] = ll_init();
    }

    return table;
}

void ht_free(hashtable_t *table)
{
    // Free a hashtable from memory
    for (size_t i = 0; i < table->num_buckets; i++)
    {
        ll_free(table->buckets[i]);
    }
    free(table->buckets);
    free(table);
}

void ht_add(hashtable_t *table, int key, int value)
{
    // Create a new mapping from key -> value.
    // If the key already exists, replace the value.
    int hash_code = hash(key, table->num_buckets);
    linkedlist_t *bucket = table->buckets[hash_code];
    int before_size = ll_size(bucket);
    ll_add(bucket, key, value);
    table->size += ll_size(bucket) - before_size;
}

int ht_get(hashtable_t *table, int key)
{
    // Retrieve the value mapped to the given key.
    // If it does not exist, return 0
    int hash_code = hash(key, table->num_buckets);
    return ll_get(table->buckets[hash_code], key);
}

int ht_size(hashtable_t *table)
{
    // Return the number of mappings in this hashtable
    return table->size;
}
