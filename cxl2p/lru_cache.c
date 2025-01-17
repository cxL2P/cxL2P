#include "lru_cache.h"
extern uint64_t split_num;
extern uint64_t used_split_slot;

// Function to create a new node
lru_node *rb_createNode(uint64_t min, uint64_t max, bool is_RA)
{
    // ftl_log("create node\n");
    lru_node *node = (lru_node *)malloc(sizeof(lru_node));
    if (!is_RA)
    {
        node->range_block = (void *)range_group_init(min, max);
    }
    else
    {
        node->range_block = (void *)range_array_init(min, max);
    }
    node->is_RA = is_RA;
    node->load = 0;
    node->min = min;
    node->max = max;
    node->next = NULL;
    node->prev = NULL;
    node->hash_next = NULL;
    return node;
}

// Move to another hash list
void rb_move_to_new_hash_list(LRUCache *cache, lru_node *node, uint64_t old_key)
{
    // find the node in  previous hash table and update the linked list pointers
    uint64_t index = rb_hash(old_key, cache->capacity);
    lru_node *prev = NULL;
    lru_node *current = cache->hashTable[index];

    // delete frome hash list
    while (current != NULL)
    {
        if (current == node)
        {
            break;
        }
        prev = current;
        current = current->hash_next;
    }

    if (current == NULL)
    {
        // Node not found
        ftl_log("Err,Node Not found while change hash\n");
        exit(0);
        return;
    }

    // Update the linked list pointers
    if (prev == NULL)
    {
        cache->hashTable[index] = current->hash_next;
    }
    else
    {
        prev->hash_next = current->hash_next;
    }

    ftl_assert(node->min == current->min);

    // Add to new hash list
    uint64_t new_index = rb_hash(node->min, cache->capacity);
    node->hash_next = cache->hashTable[new_index];
    cache->hashTable[new_index] = node;
}

// Function to delete a node from the cache
uint64_t rb_deleteNode(LRUCache *cache, lru_node *node)
{
    if (node == NULL)
    {
        return UNMAPPED_PPA;
    }
    uint32_t result = node->min;
    // find the node in hash table and update the linked list pointers
    int index = rb_hash(node->min, cache->capacity);
    lru_node *prev = NULL;
    lru_node *current = cache->hashTable[index];

    while (current != NULL)
    {
        if (current == node)
        {
            break;
        }
        prev = current;
        current = current->hash_next;
    }

    if (current == NULL)
    {
        // Node not found
        return UNMAPPED_PPA;
    }

    // Update the linked list pointers
    if (prev == NULL)
    {
        cache->hashTable[index] = current->hash_next;
    }
    else
    {
        prev->hash_next = current->hash_next;
    }

    // Update the LRUCache pointers
    if (cache->head == current)
    {
        cache->head = current->next;
    }
    if (cache->tail == current)
    {
        cache->tail = current->prev;
    }

    if (current->prev != NULL)
    {
        current->prev->next = current->next;
    }
    if (current->next != NULL)
    {
        current->next->prev = current->prev;
    }

    // Free the memory allocated for the node's range bucket and the node itself
    // ftl_log("delete node:%lu\n", current->min);
    if (!(current->is_RA))
    {
        range_group_destory((range_group *)(current->range_block));
    }
    else
    {
        range_array_destory((range_array *)(current->range_block));
    }
    free(current);
    cache->count--;
    return result;
}

void rb_removeByKey(LRUCache *cache, uint64_t key)
{
    // ftl_log("delete Node %ld\n", key);
    uint64_t index = rb_hash(key, cache->capacity);
    lru_node *prev = NULL;
    lru_node *current = cache->hashTable[index];

    // Find the node to be removed
    while (current != NULL)
    {
        if (current->min == key)
        {
            break;
        }
        prev = current;
        current = current->hash_next;
    }

    if (current == NULL)
    {
        // Node with the given key not found
        return;
    }

    // Update the linked list pointers
    if (prev == NULL)
    {
        cache->hashTable[index] = current->hash_next;
    }
    else
    {
        prev->hash_next = current->hash_next;
    }

    // Update the LRUCache pointers if needed
    if (cache->head == current)
    {
        cache->head = current->next;
    }
    if (cache->tail == current)
    {
        cache->tail = current->prev;
    }

    if (current->prev != NULL)
    {
        current->prev->next = current->next;
    }
    if (current->next != NULL)
    {
        current->next->prev = current->prev;
    }

    // Free the memory allocated for the node's range bucket and the node itself
    if (!current->is_RA)
    {
        range_group_destory((range_group *)(current->range_block));
    }
    else
    {
        range_array_destory((range_array *)(current->range_block));
    }

    free(current);
    cache->count--;
}

// Function to add a node to the front of the cache (MRU position)
void rb_addToFront(LRUCache *cache, lru_node *node)
{
    node->next = cache->head;
    node->prev = NULL;
    if (cache->head != NULL)
    {
        cache->head->prev = node;
    }
    cache->head = node;
    if (cache->tail == NULL)
    {
        cache->tail = node;
    }
}

// Function to move a node to the front of the cache (MRU position)
void rb_moveToFront(LRUCache *cache, lru_node *node)
{
    if (node == cache->head)
    {
        return;
    }
    if (node == cache->tail)
    {
        cache->tail = node->prev;
    }
    node->prev->next = node->next;
    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }
    rb_addToFront(cache, node);
}

// Function to move a node to the end of the cache (LRU position)
void rb_moveToEnd(LRUCache *cache, lru_node *node)
{
    if (node == cache->tail)
    {
        return; // Node is already at the end
    }

    // Update pointers for the neighboring nodes
    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }

    // Update cache pointers if needed
    if (node == cache->head)
    {
        cache->head = node->next;
    }

    // Move the node to the end of the cache
    if (cache->tail != NULL)
    {
        cache->tail->next = node;
    }
    node->prev = cache->tail;
    node->next = NULL;
    cache->tail = node;
}

// Function to create an LRU cache
LRUCache *rb_createLRUCache(int capacity)
{
    LRUCache *cache = (LRUCache *)malloc(sizeof(LRUCache));
    cache->capacity = capacity;
    cache->count = 0;
    cache->hashTable = (lru_node **)malloc(capacity * sizeof(lru_node *));
    cache->head = NULL;
    cache->tail = NULL;
    for (int i = 0; i < capacity; i++)
    {
        cache->hashTable[i] = NULL;
    }
    return cache;
}

// Function to add a node to the cache
// Each time we add a  node to cache, it should return the bucket range to GRT
uint64_t rb_addNodeToCache(LRUCache *cache, lru_node *node)
{
    uint64_t evict_key = UNMAPPED_PPA;
    // ftl_log("Add node %ld %ld to cache\n", node->min, node->max);
    if (cache->count >= cache->capacity)
    {
        // If the cache is full, evict the least recently used node
        evict_key = evict(cache);
    }

    int index = rb_hash(node->min, cache->capacity);
    node->hash_next = cache->hashTable[index];
    cache->hashTable[index] = node;

    rb_addToFront(cache, node);
    cache->count++;
    return evict_key;
}

uint64_t rb_addNodeToEndOfCache(LRUCache *cache, lru_node *node)
{
    uint64_t evict_key = UNMAPPED_PPA;
    // ftl_log("Add node %ld %ld to End of cache\n", node->min, node->max);
    if (cache->count >= cache->capacity)
    {
        // If the cache is full, evict the least recently used node
        evict_key = evict(cache);
    }
    // Update node pointers
    node->prev = cache->tail;
    node->next = NULL;

    // Update cache pointers
    if (cache->tail != NULL)
    {
        cache->tail->next = node;
    }
    cache->tail = node;

    // If cache was empty, update head pointer
    if (cache->head == NULL)
    {
        cache->head = node;
    }

    int index = rb_hash(node->min, cache->capacity);
    node->hash_next = cache->hashTable[index];
    cache->hashTable[index] = node;

    cache->count++;
    return evict_key;
}

// function add node to nand cache
uint64_t rb_addNodeToNandCache(LRUCache *nand_cache, lru_node *node)
{
    uint64_t index = rb_hash(node->min, nand_cache->capacity);
    node->hash_next = nand_cache->hashTable[index];
    nand_cache->hashTable[index] = node;

    rb_addToFront(nand_cache, node);
    nand_cache->count++;
    return node->min;
}

// Function to evict the least recently used node from the cache
uint64_t evict(LRUCache *cache)
{
    if (cache->count >= cache->capacity)
    {
        lru_node *lastNode = cache->tail;
        if (lastNode)
        {
            uint64_t index = rb_hash(lastNode->min, cache->capacity);
            lru_node *current = cache->hashTable[index];
            lru_node *prev = NULL;

            // Traverse the linked list to find and remove the lastNode
            while (current != NULL && current != lastNode)
            {
                prev = current;
                current = current->hash_next;
            }
            if (current == lastNode)
            {
                if (prev)
                {
                    prev->hash_next = current->hash_next;
                }
                else
                {
                    cache->hashTable[index] = current->hash_next;
                }
                if (lastNode->prev)
                {
                    lastNode->prev->next = lastNode->next;
                }
                if (lastNode->next)
                {
                    lastNode->next->prev = lastNode->prev;
                }
                cache->tail = lastNode->prev;
                uint64_t result = lastNode->min;
                assert(lastNode->is_RA == true);
                cache->count--;
                return result;
            }
        }
    }
    ftl_err("EVICT FAILED\n");
    return UNMAPPED_PPA;
}

// Function to free the memory occupied by the LRU cache
void delete_LRU_cache(LRUCache *cache)
{
    for (int i = 0; i < cache->capacity; i++)
    {
        lru_node *node = cache->hashTable[i];
        while (node != NULL)
        {
            lru_node *next = node->hash_next;
            free(node->range_block);
            free(node);
            node = next;
        }
    }
    free(cache->hashTable);
    free(cache);
}

// Function to calculate the rb_hash value for a given key
uint64_t rb_hash(uint64_t key, int capacity)
{
    return key % ((uint64_t)capacity);
}

// Function to rb_get the value corresponding to a key from the LRU cache
uint64_t rb_get(LRUCache *cache, uint64_t key, uint64_t lpn)
{
    uint64_t index = rb_hash(key, cache->capacity);
    lru_node *node = cache->hashTable[index];
    while (node != NULL)
    {
        if (node->min == key)
        {
            if (!node->is_RA)
            {
                rb_moveToFront(cache, node);
            }
            uint64_t result = UNMAPPED_PPA;
            if (node->is_RA)
            {
                result = range_array_search((range_array *)(node->range_block), lpn);
            }
            else
            {
                result = range_group_search((range_group *)(node->range_block), lpn);
            }
            return result;
        }
        node = node->hash_next;
    }
    return UNMAPPED_PPA; // Key not found
}

uint64_t rb_get_debug(LRUCache *cache, uint64_t key, uint64_t lpn)
{
    uint64_t index = rb_hash(key, cache->capacity);
    lru_node *node = cache->hashTable[index];
    while (node != NULL)
    {
        if (node->min == key)
        {
            if (!node->is_RA)
            {
                rb_moveToFront(cache, node);
            }
            uint64_t result = UNMAPPED_PPA;
            if (node->is_RA)
            {
                result = range_array_search((range_array *)(node->range_block), lpn);
            }
            else
            {
                result = range_group_search((range_group *)(node->range_block), lpn);
                range_group_print((range_group *)(node->range_block));
            }
            return result;
        }
        node = node->hash_next;
    }
    return UNMAPPED_PPA; // Key not found
}

// Function to delete the from the LRU cache
uint64_t rb_delete(LRUCache *cache, uint64_t key, uint64_t lpn)
{
    if (lpn == check_key)
    {
        ftl_err("key %lu delete in bucket %lu\n", lpn, key);
    }
    uint64_t index = rb_hash(key, cache->capacity);
    lru_node *node = cache->hashTable[index];

    while (node != NULL)
    {
        if (node->min == key)
        {
            uint64_t result = UNMAPPED_PPA;
            rb_moveToFront(cache, node);
            if (node->is_RA)
            {
                result = range_array_delete((range_array *)(node->range_block), lpn);
                if (result != UNMAPPED_PPA)
                {
                    node->load--;
                }
            }
            else
            {
                result = range_group_delete((range_group *)(node->range_block), lpn);
                if (result != UNMAPPED_PPA)
                {
                    node->load--;
                }
            }
            return result;
        }
        node = node->hash_next;
    }

    return UNMAPPED_PPA; // Key not found
}

// Function to insert a key-value pair into the LRU cache
int rb_put(LRUCache *cache, GRT *grt, seg_meta *seg_meta_arr, uint64_t key, uint64_t lpn, uint64_t ppn, uint64_t grt_max_key)
{
    uint64_t max_key = 0;
    int res = 0;
    uint64_t index = rb_hash(key, cache->capacity);
    lru_node *node = cache->hashTable[index];
    while (node != NULL)
    {
        if (node->min == key)
        {
            if (node->max < key)
            {
                node->max = key;
            }
            rb_moveToFront(cache, node);
            if (node->is_RA)
            {
                range_array *ra = (range_array *)(node->range_block);
                max_key = ra->max;
                res = range_array_insert(ra, lpn, ppn);
                return res;
            }
            else
            {
                while (1)
                {
                    res = range_group_insert((range_group *)(node->range_block), lpn, ppn);
                    max_key = ((range_group *)node->range_block)->max;
                    if (res != 0)
                    {
                        return res;
                    }
                    // printf("Insert kv %lu %lu failed\n",lpn,ppn);
                    bool trans2RA = false;
                    int nums_of_RA = 0;
                    void **ptr = NULL;
                    ptr = range_group_split((range_group *)(node->range_block), seg_meta_arr, grt_max_key, &trans2RA, false, &nums_of_RA);

                    if (trans2RA)
                    { // split into multi RA
                        rbtree_node *grt_node = search_grt(grt, node->min);
                        grt_node->addr = ptr[0];
                        rb_deleteNode(cache, node);

                        range_array **RAs = (range_array **)ptr;
                        for (int i = 0; i < nums_of_RA; i++)
                        {
                            // construct lru_node
                            lru_node *new_node = (lru_node *)malloc(sizeof(lru_node));
                            new_node->next = NULL;
                            new_node->prev = NULL;
                            new_node->is_RA = true;
                            new_node->hash_next = NULL;
                            new_node->min = RAs[i]->min;
                            new_node->max = RAs[i]->max;
                            new_node->range_block = ptr[i];

                            // update value
                            if (lpn < new_node->max && lpn >= new_node->min)
                            {
                                res = range_array_insert((RAs[i]), lpn, ppn);
                            }
                            new_node->load = RAs[i]->load;

                            // Add Node to Cache
                            uint64_t evict_key = UNMAPPED_PPA;
                            evict_key = rb_addNodeToEndOfCache(cache, new_node);

                            if (evict_key != UNMAPPED_PPA)
                            {
                                set_in_flash(grt, evict_key);
                            }

                            //  update grt
                            if (i == 0)
                            {

                                // When Max RG trans 2 RAs, Put new Max RG into LRU incase of data missing
                                if (new_node->min >= grt_max_key)
                                {
                                    lru_node *new_nodex = rb_createNode(max_key + 1, max_key + 1, false);
                                    insert_grt(grt, max_key + 1, false, new_nodex->range_block);
                                    if (lpn >= RAs[nums_of_RA - 1]->max)
                                    {
                                        range_group_insert((range_group *)new_nodex->range_block, lpn, ppn);
                                    }
                                    evict_key = UNMAPPED_PPA;
                                    evict_key = rb_addNodeToCache(cache, new_nodex);
                                    if (evict_key != UNMAPPED_PPA)
                                    {
                                        set_in_flash(grt, evict_key);
                                    }
                                }
                            }
                            else
                            {
                                insert_grt(grt, new_node->min, true, new_node->range_block);
                            }
                        }
                        free(ptr);
                        return res;
                    }
                    else
                    {
                        // split into RG
                        lru_node *new_node = (lru_node *)malloc(sizeof(lru_node));
                        new_node->next = NULL;
                        new_node->prev = NULL;
                        new_node->is_RA = false;
                        new_node->hash_next = NULL;
                        range_group *rg = ((range_group **)ptr)[0];
                        new_node->min = rg->min;
                        new_node->max = rg->max;
                        new_node->load = rg->load;
                        new_node->range_block = ptr[0];
                        uint64_t split_key = rg->min;
                        // update node info
                        node->max = new_node->min;
                        node->load = ((range_group *)(node->range_block))->load;
                        // ftl_log("Node[%ld %ld] RG[%ld %ld]\n",node->min,node->max,((range_group *)(node->range_block))->min,((range_group *)(node->range_block))->max);

                        uint64_t evict_key = UNMAPPED_PPA;
                        evict_key = rb_addNodeToCache(cache, new_node);

                        if (evict_key != UNMAPPED_PPA)
                        {
                            set_in_flash(grt, evict_key);
                        }
                        insert_grt(grt, new_node->min, false, new_node->range_block);

                        // try to update again in range bucket
                        if (lpn >= split_key)
                        {
                            node = new_node;
                        }
                        rb_moveToFront(cache, node);
                        free(ptr);
                    }
                }
            }
        }
        node = node->hash_next;
    }
    return res;
}

// Function to update a key-value pair into the LRU cache
int rb_update(LRUCache *cache, GRT *grt, seg_meta *seg_meta_arr, uint64_t key, uint64_t lpn, uint64_t ppn, uint64_t grt_max_key, bool is_pma0_flush)
{
    uint64_t max_key = 0;
    int res = 0;
    uint64_t index = rb_hash(key, cache->capacity);
    lru_node *node = cache->hashTable[index];
    while (node != NULL)
    {
        if (node->min == key)
        {
            if (node->max < key)
            {
                node->max = key;
            }
            rb_moveToFront(cache, node);
            if (node->is_RA)
            {
                range_array *ra = (range_array *)(node->range_block);
                res = range_array_insert(ra, lpn, ppn);
                max_key = ra->max;
                return res;
            }
            else
            {
                while (1)
                {
                    res = range_group_update((range_group *)(node->range_block), lpn, ppn);
                    max_key = ((range_group *)(node->range_block))->max;
                    if (res != 0)
                    {
                        return res;
                    }

                    bool trans2RA = false;
                    int nums_of_RA = 0;
                    void **ptr = NULL;
                    ptr = range_group_split((range_group *)(node->range_block), seg_meta_arr, grt_max_key, &trans2RA, is_pma0_flush, &nums_of_RA);
                    if (trans2RA)
                    {
                        // split into multi RA
                        rb_deleteNode(cache, node);
                        range_array **RAs = (range_array **)ptr;
                        for (int i = 0; i < nums_of_RA; i++)
                        {
                            // construct lru_node
                            lru_node *new_node = (lru_node *)malloc(sizeof(lru_node));
                            new_node->next = NULL;
                            new_node->prev = NULL;
                            new_node->is_RA = true;
                            new_node->hash_next = NULL;
                            new_node->min = RAs[i]->min;
                            new_node->max = RAs[i]->max;

                            new_node->range_block = ptr[i];

                            // update value
                            if (lpn < new_node->max && lpn >= new_node->min)
                            {
                                res = range_array_insert((RAs[i]), lpn, ppn);
                            }
                            new_node->load = RAs[i]->load;
                            // Add Node to Cache
                            uint64_t evict_key = UNMAPPED_PPA;
                            evict_key = rb_addNodeToEndOfCache(cache, new_node);

                            // check whether evict happen
                            if (evict_key != UNMAPPED_PPA)
                            {
                                set_in_flash(grt, evict_key);
                            }

                            //  update grt
                            if (i == 0)
                            {
                                // When Max RG trans 2 RAs, Put new Max RG into LRU incase of data missing
                                if (new_node->min >= grt_max_key)
                                {
                                    evict_key = UNMAPPED_PPA;
                                    lru_node *new_nodex = rb_createNode(max_key + 1, max_key + 1, false);
                                    insert_grt(grt, max_key + 1, false, new_nodex->range_block);
                                    evict_key = rb_addNodeToCache(cache, new_nodex);
                                    if (evict_key != UNMAPPED_PPA)
                                    {
                                        set_in_flash(grt, evict_key);
                                    }
                                }
                            }
                            else
                            {
                                insert_grt(grt, new_node->min, true, new_node->range_block);
                            }
                        }
                        free(ptr);
                        return res;
                    }
                    else
                    { // split into RG

                        lru_node *new_node = (lru_node *)malloc(sizeof(lru_node));
                        new_node->next = NULL;
                        new_node->prev = NULL;
                        new_node->is_RA = false;
                        new_node->hash_next = NULL;
                        range_group *rg = ((range_group **)ptr)[0];
                        new_node->min = rg->min;
                        new_node->max = rg->max;
                        new_node->load = rg->load;
                        new_node->range_block = ptr[0];
                        uint64_t split_key = rg->min;
                        // ftl_log("after split min %u max %u\n", new_node->range_group->min, new_node->range_group->max);

                        node->max = new_node->min;
                        node->load = ((range_group *)(node->range_block))->load;
                        // ftl_log("Node[%ld %ld] RG[%ld %ld]\n",node->min,node->max,((range_group *)(node->range_block))->min,((range_group *)(node->range_block))->max);

                        if (node->load == 0 && node->min != 0 && lpn >= split_key)
                        {
                            rb_deleteNode(cache, node);
                            delete_grt(grt, node->min);
                        }
                        else if (new_node->load == 0 && lpn < split_key)
                        {
                            free(new_node->range_block);
                            free(new_node);
                            rb_moveToFront(cache, node);
                            continue;
                        }

                        uint64_t evict_key = UNMAPPED_PPA;
                        evict_key = rb_addNodeToCache(cache, new_node);

                        insert_grt(grt, new_node->min, false, new_node->range_block);

                        // check whether evict happen
                        if (evict_key != UNMAPPED_PPA)
                        {
                            set_in_flash(grt, evict_key);
                        }
                        // try to update again in range bucket
                        if (lpn >= split_key)
                        {
                            node = new_node;
                        }
                        rb_moveToFront(cache, node);
                        free(ptr);
                    }
                }
            }
        }
        node = node->hash_next;
    }
    return res;
}

lru_node *find_range_block(LRUCache *cache, uint64_t key)
{
    uint64_t index = rb_hash(key, cache->capacity);
    lru_node *node = cache->hashTable[index];
    while (node != NULL)
    {
        if (node->min == key)
        {
            return node;
        }
        node = node->hash_next;
    }
    return NULL; // Range bucket not found
}

// Function to print the contents of the LRU cache
void rb_printLRUCache(LRUCache *cache)
{
    ftl_log("LRUCache Contents:\n");
    lru_node *node = cache->head;
    int i = 0;
    int empty_num = 0;
    int ra_entry_num = 0;
    int ra_total_load = 0;
    int rb_entry_num = 0;
    int rb_total_load = 0;
    int ra_num = 0;
    int rb_num = 0;
    uint64_t tt_RB_byte = 0;
    uint64_t tt_RA_byte = 0;

    while (node != NULL)
    {
        if (node->is_RA)
        {
            range_array *ra = (range_array *)node->range_block;
            if (ra->load == 0)
            {
                empty_num++;
            }
            // ftl_log(" RA[%ld,%ld,%d] ", ((range_array *)node->range_block)->min, ((range_array *)node->range_block)->max, ((range_array *)node->range_block)->load);
            ra_num++;
            ra_entry_num += ra->load;
            tt_RA_byte += (ra->max - ra->min - 1) * 4;
            ra_total_load += (ra->max - ra->min - 1);
            assert(ra->max >= (ra->min + 1));
        }
        else
        {
            if (((range_group *)node->range_block)->load == 0)
            {
                empty_num++;
            }
            else
            {

                tt_RB_byte += (64 * 1024);
                // range_group_stat(((range_group *)node->range_block),&(entry_in_hash_bkt),&(entry_in_stah_bkt),&(entry_in_split_bkt));
            }
            // ftl_log(" RG[%ld,%ld,%d] ", ((range_group *)node->range_block)->min, ((range_group *)node->range_block)->max, ((range_group *)node->range_block)->load);
            rb_num++;
            rb_entry_num += ((range_group *)node->range_block)->load;
            rb_total_load += PAGE_CAPACITY * ASSOC_NUM;
        }
        i++;
        node = node->next;
    }

    ftl_log("RA bytes %lu,RG bytes %lu,RG DRAM consumption %lf,RA DRAM consumption %lf\n", tt_RA_byte, tt_RB_byte, tt_RB_byte * 1.0 / 33554432, tt_RA_byte * 1.0 / 33554432);
    ftl_log("# of RG %d, # of RA %d, empty count %d cache count %d, capacity %d,# of entry %d, RA lf %f RG lf %f \n", rb_num, ra_num, empty_num, cache->count, cache->capacity, ra_entry_num + rb_entry_num, ra_entry_num * 1.0 / ra_total_load, rb_entry_num * 1.0 / rb_total_load);
}

// Function to check the spesific bucket of the LRU cache
bool rb_check_block(LRUCache *cache, uint64_t key)
{
    lru_node *node = cache->head;
    bool is_contained = false;
    while (node != NULL)
    {
        if (node->min == key)
        {
            is_contained = true;
            break;
        }
        node = node->next;
    }
    return is_contained;
}
