#include "grt.h"
#define SEG_CAPACITY UINT64_MAX

GRT *init_grt(GRT *grt, uint64_t max_lpn)
{
	grt = (GRT *)malloc(sizeof(GRT));
	grt->tree = (RedBlackTree *)malloc(sizeof(RedBlackTree));
	if (grt->tree == NULL)
	{
		ftl_err("malloc failed\n");
	}

	grt->tree = (RedBlackTree *)malloc(sizeof(RedBlackTree));
	grt->tree->nil = (rbtree_node *)malloc(sizeof(rbtree_node));
	grt->tree->nil->color = BLACK;
	grt->tree->nil->is_RA = 0;
	grt->tree->root = grt->tree->nil;

	grt->max_tree_index = 0;
	return grt;
}

void insert_grt(GRT *grt, uint64_t key, bool is_RA, void *addr)
{

	rbtree_node *node = NULL;
	node = (rbtree_node *)malloc(sizeof(rbtree_node));
	node->key = key;
	node->in_flash = false;
	node->is_RA = is_RA;
	node->addr = addr;

	// printf("Insert %lu in tree %lu ,max_tree %lu\n", key, grt_index,grt->max_tree_index);
	rbtree_insert(grt->tree, node);
}

rbtree_node *search_grt(GRT *grt, uint64_t key)
{
	rbtree_node *node = NULL;
	node = rbtree_search(grt->tree, key);

	return node;
}

void delete_grt(GRT *grt, uint64_t key)
{
	rbtree_node *nodeToDelete = rbtree_search(grt->tree, key);

	if (nodeToDelete != grt->tree->nil)
	{
		rbtree_delete(grt->tree, nodeToDelete);
	}
}

void set_in_flash(GRT *grt, uint64_t key)
{
	rbtree_node *node = search_grt(grt, key);
	node->in_flash = true;
	return;
}

void set_in_cache(GRT *grt, uint64_t key)
{
	rbtree_node *node = search_grt(grt, key);
	node->in_flash = false;
	return;
}

void check_in_flash(GRT *grt, uint64_t key)
{
	rbtree_node *node = search_grt(grt, key);
	if (node->in_flash)
	{
		ftl_err("%ld is in flash\n", key);
	}
	else
	{
		ftl_err("%ld is in cache\n", key);
	}
	return;
}

uint64_t get_max_key(GRT *grt)
{
	rbtree_node *node = rbtree_maxi(grt->tree, grt->tree->root);
	return node->key;
}