/**
* COMP30023 Computer Systems 2019
* Project 1: Image Tagger
*
* Created by Ziqi Jia on 25/04/19.
* Copyright © 2019 Ziqi Jia. All rights reserved.
*
* References:
* Dictionary Create, insert, search and delete
* 	http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)HashTables.html?highlight=%28CategoryAlgorithmNotes%29
* hash_function
* 	http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)HashTables.html?highlight=%28CategoryAlgorithmNotes%29
*
*/


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "dict.h"

#define INITIAL_SIZE (1024)
#define GROWTH_FACTOR (2)
#define MAX_LOAD_FACTOR (1)
#define MULTIPLIER (97)

/* Make a duplicate of str. */
char *strdup(const char *str)
{
    int n = strlen(str) + 1;  /* +1 for ’\0’ */
    char *dup = malloc(n * sizeof(char));
    if(dup != NULL)
    {
        strcpy(dup, str);
    }
    return dup;
}

/* Initialize the dictionary, that used in both Dictionary_Create and Dictionary_Grow. */
Dict
internal_Dictionary_Create(int size)
{
    Dict dict;
    int i;

    dict = malloc(sizeof(*dict));
    assert(dict != 0);

    //initialize the dictionary
    dict->size = size;
    dict->num = 0;
    dict->table = malloc(sizeof(struct elt *) * dict->size);
    dict->keys = malloc(sizeof(int) * dict->size);

    assert(dict->table != 0);

    for(i = 0; i < dict->size; i++) dict->table[i] = 0;
    for(i = 0; i < dict->size; i++) dict->keys[i] = 0;

    return dict;
}

/* Create and return a new Dictionary object. */
Dict
Dictionary_Create(void)
{
    return internal_Dictionary_Create(INITIAL_SIZE);
}

/* Destroy the dictionary. */
void
Dictionary_Destroy(Dict d)
{
    assert(d);
    int i;
    struct dNode *n, *next;
    //free each node's key and value
    for(i = 0; i < d->size; i++) {
        for(n = d->table[i]; n != 0; n = next) {
            next = n->next;

            free(n->key);
            free(n->value);
            free(n);
        }
    }
    //free the pointer table in the dictionary
    free(d->table);
    //free the whole dictionary
    free(d);
}

/* Clear the keys and values in the dictionary. */
void
Dictionary_Clear(Dict d)
{
    int i;
    struct dNode *n, *next;
    //free each node's key and value
    for(i = 0; i < d->size; i++) {
        for(n = d->table[i]; n != 0; n = next) {
            next = n->next;
            free(n->key);
            free(n->value);
            free(n);
        }
    }
    for(i = 0; i < d->size; i++) d->table[i] = 0;
    for(i = 0; i < d->size; i++) d->keys[i] = 0;
    d->num = 0;
}

/* Hash function for hashing a string to an integer value */
static unsigned long hash_function(const char *);

static unsigned long
hash_function(const char *s)
{
    unsigned const char *us;
    unsigned long h;

    h = 0;
    for(us = (unsigned const char *) s; *us; us++) {
        h = h * MULTIPLIER + *us;
    }
    return h;
}

/* Grow the dictionary size if there is not enough room */
static void Dictionary_Grow(Dict);

static void
Dictionary_Grow(Dict d)
{
    Dict d2;            /* new dictionary to create */
    struct dictionary swap;   /* temporary structure for brain transplant */
    int i;
    struct dNode *n;

    d2 = internal_Dictionary_Create(d->size * GROWTH_FACTOR);

    for(i = 0; i < d->size; i++) {
        for(n = d->table[i]; n != 0; n = n->next) {
            /* recopy everything */
            Dictionary_Insert(d2, n->key, n->value);
        }
    }

    /* swap d and d2 then call Dictionary_Destroy on d2 */
    swap = *d;
    *d = *d2;
    *d2 = swap;

    Dictionary_Destroy(d2);
}

/* Insert a new key-value pair into an existing dictionary. */
void
Dictionary_Insert(Dict d, const char *key, const char *value)
{
    struct dNode *n;
    unsigned long h;

    assert(key);
    assert(value);
    h = hash_function(key) % d->size;
    if(Dictionary_Search(d, key))
        Dictionary_Delete(d, key);

    n = malloc(sizeof(*n));
    assert(n);
    n->key = strdup(key);
    n->value = strdup(value);
    n->next = d->table[h];
    d->table[h] = n;
    d->keys[h] += 1;
    d->num++;

    /* grow table if there is not enough room */
    if(d->num >= d->size * MAX_LOAD_FACTOR) {
        Dictionary_Grow(d);
    }
}

/* Count the size of dictionary. */
const int
Dictionary_Size(Dict d)
{
    int i;
    int count = 0;
    for(i = 0; i<d->size; i++)
    {
        if (d->keys[i] > 0)
        {
            count += d->keys[i];
        }
    }
    return count;
}

/* Return the string value associated with a given key or 0 if no matching key is present. */
const char *
Dictionary_Search(Dict d, const char *key)
{
    struct dNode *n;

    for(n = d->table[hash_function(key) % d->size]; n != 0; n = n->next) {
        if(!strcmp(n->key, key)) {
            return n->value;
        }
    }

    return 0;
}

/* Return all nodes in the dictionary. */
const struct elt **
Dictionary_AllItems(Dict d, int *count_num)
{
    int i;
    int count = 0;
    // count the existing keys in dictionary
    for(i = 0; i<d->size; i++)
    {
        if (d->keys[i] > 0)
        {
            count += d->keys[i];
        }
    }
    // use a new stuct dNode to store all the nodes in dictionary
    struct dNode **ns = malloc(sizeof(struct dNode *)*count);
    *count_num = count;
    int count_idx = 0;
    struct dNode *n;
    for(i=0; i<d->size; i++)
    {
        if (d->keys[i] > 0)
        {
            for(n = d->table[i]; n != 0; n = n->next)
            {
                ns[count_idx] = d->table[i];
                count_idx += 1;
            }
        }
    }
    return ns;
}

/* Delete the node with the given key, and free all memory
 * associated with it. If there is no such record, has no effect. */
void
Dictionary_Delete(Dict d, const char *key)
{
    struct dNode **prev;          /* what to change when node is deleted */
    struct dNode *n;              /* what to delete */
    unsigned long h;
    h = hash_function(key) % d->size;

    for(prev = &(d->table[h]); *prev != 0; prev = &((*prev)->next)) {
        if(!strcmp((*prev)->key, key)) {
            /* got it */
            n = *prev;
            *prev = n->next;

            free(n->key);
            free(n->value);
            free(n);
            break;
        }
    }
    if (d->keys[h]>0)
        d->keys[h] -= 1;
    d->num--;
}
