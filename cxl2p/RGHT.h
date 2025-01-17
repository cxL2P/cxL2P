#include "lru_cache.h"
#include <time.h>

typedef struct range_group_hash_table
{
    LRUCache *lru; // range bucket locate
    GRT *grt;      // global range table
} RGHT;

// Initialize ftl cache
RGHT *init_RGHT(RGHT *rght, int capacity, uint64_t tt_page);

// Insert ftl cache
void insert_RGHT(RGHT *rght, seg_meta *seg_meta, uint64_t key, uint64_t value, uint64_t *swap);

int update_RGHT(RGHT *rght, seg_meta *seg_meta, uint64_t key, uint64_t value, bool is_pma0_flush, uint64_t *swap);

uint64_t search_RGHT(RGHT *rght, uint64_t key, uint64_t *swap);

uint64_t search_RGHT_debug(RGHT *rght, uint64_t key, uint64_t *swap);

uint64_t delete_RGHT(RGHT *rght, uint64_t key, uint64_t *cxl_lat);

void destory_RGHT(RGHT *rght);

/*
Deconstruct Pma0 seg and move value to coresponding RB
*/
bool deconstruct_pma0_seg(RGHT *rght, seg_meta *seg_meta_arr, uint64_t seg_idx);

void compact_RGHT(RGHT *rght);
