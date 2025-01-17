#include "rb_tree.h"

typedef struct global_range_table
{
    RedBlackTree *tree;
    uint64_t max_tree_index;
} GRT;

GRT *init_grt(GRT *grt, uint64_t max_lpn);

void insert_grt(GRT *grt, uint64_t key, bool is_RA, void *addr);

rbtree_node *search_grt(GRT *grt, uint64_t key);

void delete_grt(GRT *grt, uint64_t key);

void set_in_cache(GRT *grt, uint64_t key);

void set_in_flash(GRT *grt, uint64_t key);

void check_in_flash(GRT *grt, uint64_t key);

uint64_t get_max_key(GRT *grt);