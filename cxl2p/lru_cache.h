#include "range_group.h"

#define RB_IN_NAND_CAPACITY 1000000 // in flash range bucket num
#define CXL_READ_LATENCY 300
#define CXL_WRITE_LATENCY 300

// lru_node structure for each cache entry
typedef struct lru_node
{
    void *range_block; // RB/RA
    bool is_RA;
    uint64_t max;
    uint64_t min;
    uint16_t load;
    struct lru_node *next;
    struct lru_node *prev;
    struct lru_node *hash_next;
} lru_node;

// Structure for the LRU cache
typedef struct
{
    int capacity;
    int count;
    lru_node **hashTable;
    lru_node *head;
    lru_node *tail;
} LRUCache;

lru_node *rb_createNode(uint64_t min, uint64_t max, bool is_RA);

uint64_t rb_addNodeToCache(LRUCache *cache, lru_node *node);

uint64_t rb_addNodeToEndOfCache(LRUCache *cache, lru_node *node);

uint64_t rb_deleteNode(LRUCache *cache, lru_node *node);

uint64_t evict(LRUCache *cache);

uint64_t rb_addNodeToNandCache(LRUCache *nand_cache, lru_node *node);

void rb_moveToEnd(LRUCache *cache, lru_node *node);

uint64_t rb_hash(uint64_t key, int capacity);

LRUCache *rb_createLRUCache(int capacity);

void delete_LRU_cache(LRUCache *cache);

uint64_t rb_get(LRUCache *cache, uint64_t key, uint64_t lpn);

uint64_t rb_get_debug(LRUCache *cache, uint64_t key, uint64_t lpn);

int rb_put(LRUCache *cache, GRT *grt, seg_meta *seg_meta, uint64_t key, uint64_t lpn, uint64_t ppn, uint64_t grt_max_key);

int rb_update(LRUCache *cache, GRT *grt, seg_meta *seg_meta, uint64_t key, uint64_t lpn, uint64_t ppn, uint64_t grt_max_key, bool is_pma0_flush);

uint64_t rb_delete(LRUCache *cache, uint64_t key, uint64_t lpn);

void rb_printLRUCache(LRUCache *cache);

lru_node *find_range_block(LRUCache *cache, uint64_t key);

bool rb_check_block(LRUCache *cache, uint64_t key);

void rb_addToFront(LRUCache *cache, lru_node *node);

void rb_removeByKey(LRUCache *cache, uint64_t key);

void rb_moveToFront(LRUCache *cache, lru_node *node);

void rb_move_to_new_hash_list(LRUCache *cache, lru_node *node, uint64_t old_key);
