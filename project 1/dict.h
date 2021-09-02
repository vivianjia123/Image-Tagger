#ifndef _DICT_
#define _DICT_

typedef struct dictionary *Dict;

struct dNode {
    struct dNode *next;  /* ptr to next node */
    char *key;           /* ptr to stored key */
    char *value;         /* ptr to stored value */
};

struct dictionary {
    int size;            /* size of the pointer table */
    int num;             /* number of node stored */
    struct dNode **table;/* pointer table */
    int *keys;           /* number of keys of the pointer table */
};

/* make a duplicate of str */
char *strdup(const char *str);

/* create a new empty dictionary */
Dict Dictionary_Create(void);

/* destroy a dictionary */
void Dictionary_Destroy(Dict);

/* clear a dictionary */
void Dictionary_Clear(Dict);

/* insert a new node into dictionary */
void Dictionary_Insert(Dict, const char *key, const char *value);

/* count the size of dictionary */
const int Dictionary_Size(Dict);

/* return the value associated with a matching key */
const char *Dictionary_Search(Dict, const char *key);

/* return all nodes in the dictionary */
const struct elt **Dictionary_AllItems(Dict, int *);

/* delete the value with the given key */
void Dictionary_Delete(Dict, const char *key);

#endif
