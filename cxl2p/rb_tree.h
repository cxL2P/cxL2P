#ifndef _FLY_RED_BLACK_TREE_H_
#define _FLY_RED_BLACK_TREE_H_
#include <stdlib.h>
#include <stdbool.h>
#include "cArray.h"
#define KEY_TYPE uint64_t

enum RB_COLOR
{
    RED = 0,
    BLACK
};

typedef struct _rbtree_node
{
    /* data */
    KEY_TYPE key;
    void *addr;
    bool in_flash; // correspoding range bucket is in flash
    bool is_RA;
    struct _rbtree_node *left;
    struct _rbtree_node *right;
    struct _rbtree_node *parent;
    unsigned char color;

} rbtree_node;

typedef struct _RedBlackTree
{
    /* data */
    rbtree_node *root;
    rbtree_node *nil;
} RedBlackTree;

typedef struct _KeyArray
{
    uint64_t *keys;
    bool *in_flash;
    int len;
} KeyArray;

void rbtree_deleteByKey(RedBlackTree *T, KEY_TYPE key);
extern void rbtree_insert(RedBlackTree *T, rbtree_node *z);
extern rbtree_node *rbtree_delete(RedBlackTree *T, rbtree_node *z);
extern rbtree_node *rbtree_search(RedBlackTree *T, KEY_TYPE key);
void rbtree_traversal(RedBlackTree *T, rbtree_node *node);
void rbtree_get_keys(RedBlackTree *T, rbtree_node *node, KeyArray *result);
rbtree_node *rbtree_mini(RedBlackTree *T, rbtree_node *x);
rbtree_node *rbtree_maxi(RedBlackTree *T, rbtree_node *x);
void rbtree_delete_fixup(RedBlackTree *T, rbtree_node *x);
rbtree_node *rbtree_successor(RedBlackTree *T, rbtree_node *x);
#endif