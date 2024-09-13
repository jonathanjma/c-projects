#include "linkedlist.h"
#include <stdio.h>
#include <stdlib.h>

struct linkedlist {
    struct linkedlist_node *first;
    unsigned int size;
};

struct linkedlist_node {
    int key;
    int value;
    struct linkedlist_node *next;
};
typedef struct linkedlist_node linkedlist_node_t;

linkedlist_t *ll_init() {
    // Use malloc to set aside that amount of space in memory.
    linkedlist_t *list = malloc(sizeof(linkedlist_t));

    // Set metadata for your new list and return the new list
    list->first = NULL;
    list->size = 0;

    return list;
}

void ll_free(linkedlist_t *list) {
    // Free a linked list from memory
    linkedlist_node_t *cur_node = list->first;
    while (cur_node != NULL) {
        linkedlist_node_t *next_node = cur_node->next;
        free(cur_node);
        cur_node = next_node;
    }
    free(list);
}

void ll_add(linkedlist_t *list, int key, int value) {
    // Create a new node and add to the front of the linked list if a
    // node with the key does not already exist.
    // Otherwise, replace the existing value with the new value.
    linkedlist_node_t *cur_node = list->first;
    while (cur_node != NULL) {
        if (cur_node->key == key) {
            cur_node->value = value;
            return;
        }
        cur_node = cur_node->next;
    }

    // Node with key does not exist in list
    linkedlist_node_t *new_node = malloc(sizeof(linkedlist_node_t));
    new_node->key = key;
    new_node->value = value;

    linkedlist_node_t *old_first = list->first;
    list->first = new_node;
    new_node->next = old_first;
    list->size += 1;
}

int ll_get(linkedlist_t *list, int key) {
    // Go through each node in the linked list and return the value of
    // the node with a matching key. If it does not exist, return 0.
    linkedlist_node_t *cur_node = list->first;
    while (cur_node != NULL) {
        if (cur_node->key == key) {
            return cur_node->value;
        }
        cur_node = cur_node->next;
    }
    return 0;
}

int ll_size(linkedlist_t *list) {
    // Return the number of nodes in this linked list
    return list->size;
}
