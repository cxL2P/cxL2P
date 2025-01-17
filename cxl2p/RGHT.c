#include "RGHT.h"
uint64_t last_access_RA = UINT64_MAX;

RGHT *init_RGHT(RGHT *rght, int capacity, uint64_t tt_page)
{
    // allocate memory
    rght = (RGHT *)malloc(sizeof(RGHT));
    rght->grt = NULL;
    rght->grt = init_grt(rght->grt, tt_page);
    rght->lru = rb_createLRUCache(RB_IN_NAND_CAPACITY);
    printf("%d,%d,%d,%d,%d\n", capacity, RB_IN_NAND_CAPACITY, PAGE_CAPACITY, STASH_NUM, STASH_SPLIT_NUM);

    // put first RB in cache
    lru_node *new_node = rb_createNode(0, 0, false);
    rb_addNodeToCache(rght->lru, new_node);

    //  add first key in grt
    insert_grt(rght->grt, 0, false, new_node->range_block);
    return rght;
}

// insert a l2p entry in range bucket/SA
void insert_RGHT(RGHT *rght, seg_meta *seg_meta, uint64_t key, uint64_t value, uint64_t *cxl_lat)
{
    if (key == check_key)
    {
        ftl_log("insert ftl cache %lu %lu\n", key, value);
    }
    // search in grt to determine in which bucket this  l2p insert
    rbtree_node *node = search_grt(rght->grt, key);
    uint64_t grt_max_key = get_max_key(rght->grt);
    // printf("Insert key %lu,value %lu insert in bucket %lu %p in flash %d\n", key, value, node->key, node->addr,node->in_flash);
    if (node->in_flash == true)
    {
        if (node->key != last_access_RA)
        {
            (*cxl_lat) += CXL_WRITE_LATENCY;
            last_access_RA = node->key;
        }
        range_array_insert((range_array *)(node->addr), key, value);

        return;
    }

    //  insert in range bucket
    rb_put(rght->lru, rght->grt, seg_meta, node->key, key, value, grt_max_key);
}

// update a l2p entry in range bucket
// return (0 failed）（1 , insert）(2 , update)
int update_RGHT(RGHT *rght, seg_meta *seg_meta_arr, uint64_t key, uint64_t value, bool is_pma0_flush, uint64_t *cxl_lat)
{
    if (key == check_key)
    {
        ftl_log("update ftl cache %lu %lu\n", key, value);
    }
    // search in grt to determine in which bucket this  l2p insert
    rbtree_node *node = search_grt(rght->grt, key);
    uint64_t grt_max_key = get_max_key(rght->grt);
    int res = 0;
    //  printf("Update key %lu,value %lu insert in bucket %lu\n", key, value, node->key);
    if (node->in_flash == true)
    {
        if (node->key != last_access_RA)
        {
            (*cxl_lat) += CXL_WRITE_LATENCY;
            last_access_RA = node->key;
        }
        range_array_insert((range_array *)(node->addr), key, value);
        return 2;
    }

    //  insert in range bucket
    res = rb_update(rght->lru, rght->grt, seg_meta_arr, node->key, key, value, grt_max_key, is_pma0_flush);

    return res;
}

// search a l2p entry in range bucket
uint64_t search_RGHT(RGHT *rght, uint64_t key, uint64_t *cxl_lat)
{
    // seach in grt
    rbtree_node *node = search_grt(rght->grt, key);
    ftl_assert(node != NULL);
    if (node->in_flash == true)
    {
        if (node->key != last_access_RA)
        {
            (*cxl_lat) += CXL_WRITE_LATENCY;
            last_access_RA = node->key;
        }
        return range_array_search((range_array *)(node->addr), key);
    }
    // search in range bucket
    uint64_t result = rb_get(rght->lru, node->key, key);

    return result;
}

uint64_t search_RGHT_debug(RGHT *rght, uint64_t key, uint64_t *swap)
{
    // seach in grt
    rbtree_node *node = search_grt(rght->grt, key);
    ftl_assert(node != NULL);
    if (node->in_flash == true)
    {
        printf("Search %lu in node %lu\n", key, node->key);
        assert(0);
    }
    // search in range bucket
    uint64_t result = rb_get_debug(rght->lru, node->key, key);

    return result;
}

// delete a l2p entry in range buckey
uint64_t delete_RGHT(RGHT *rght, uint64_t key, uint64_t *cxl_lat)
{
    uint64_t result;
    // seach in grt
    rbtree_node *node = search_grt(rght->grt, key);
    // printf("Delete %lu in range bucket %lu\n",key,node->key);
    if (node->in_flash == true)
    {
        if (node->key != last_access_RA)
        {
            (*cxl_lat) += CXL_WRITE_LATENCY;
            last_access_RA = node->key;
        }
        result = range_array_delete((range_array *)(node->addr), key);
        (*cxl_lat) += CXL_WRITE_LATENCY;
        return result;
    }
    result = rb_delete(rght->lru, node->key, key);

    return result;
}

void destory_RGHT(RGHT *rght)
{
    delete_LRU_cache(rght->lru);
    free(rght);
}

void compact_RGHT(RGHT *rght)
{
    for (uint64_t k = 0; k <= rght->grt->max_tree_index; k++)
    {
        // get all keys
        KeyArray *result = (KeyArray *)malloc(sizeof(KeyArray));
        result->keys = malloc(RB_IN_NAND_CAPACITY * sizeof(uint64_t));
        result->in_flash = malloc(RB_IN_NAND_CAPACITY * sizeof(uint64_t));
        result->len = 0;
        uint64_t *arr = (uint64_t *)malloc(RB_IN_NAND_CAPACITY * sizeof(uint64_t));
        uint64_t arr_len = 0;

        rbtree_get_keys(rght->grt->tree, rght->grt->tree->root, result);

        for (int i = 0; i < result->len - 1; i++)
        {
            lru_node *node1 = NULL;
            lru_node *node2 = NULL;
            if (result->in_flash[i])
            {
                assert(0);
            }
            else
            {
                node1 = find_range_block(rght->lru, result->keys[i]);
            }
            if (result->in_flash[i + 1])
            {
                assert(0);
            }
            else
            {
                node2 = find_range_block(rght->lru, result->keys[i + 1]);
            }
            if (node1 == NULL)
            {
                ftl_err("eRR bucket 1:%lu ,in_flash:%d is NOT found\n", result->keys[i], result->in_flash[i]);
                exit(0);
            }
            if (node2 == NULL)
            {
                ftl_err("eRR bucket 2:%lu ,in flash:%d is NOT found\n", result->keys[i + 1], result->in_flash[i + 1]);
                exit(0);
            }

            int mode = 0;
            bool is_seq = true;
            if (node1->is_RA && node2->is_RA)
            {
                mode = 2;
            }
            else if (!(node1->is_RA) && !(node2->is_RA))
            {
                mode = 0;
            }
            else
            {
                continue;
                if (node1->is_RA && node1->min > node2->min)
                {
                    is_seq = false;
                }
                else if (node1->is_RA && node1->min < node2->min)
                {
                    is_seq = true;
                }
                else if (node2->is_RA && node1->min < node2->min)
                {
                    is_seq = false;
                }
                else if (node2->is_RA && node1->min > node2->min)
                {
                    is_seq = true;
                }
                mode = 1;
            }

            if (node1->load != (uint16_t)0 && (node1->load + node2->load) > (PAGE_CAPACITY * 4 - STASH_SPLIT_NUM * 4))
                continue;

            uint64_t delete_node = node2->min;
            uint64_t delete_grt_key = node2->min;

            // printf("Block1:[%ld,%ld,%d] %d and Block2:[%ld,%ld,%d] %d \n", node1->min, node1->max, node1->load, node1->is_RA, node2->min, node2->max, node2->load, node2->is_RA);

            lru_node *node3 = NULL;
            if (mode == 1 && node1->is_RA)
            {
                delete_node = node1->min;
                node3 = node1;
                node1 = node2;
                node2 = node3;
            }

            if (range_block_compact(&(node1->range_block), &(node2->range_block), is_seq, mode))
            {
                // compact succeed,node2 should be deleted

                range_group *rg = (range_group *)(node1->range_block);
                // printf("node1 %p node2%p RB %p\n", node1, node2, RB);
                printf("is compacted into bucket:[%ld,%ld,%d]\n", rg->min, rg->max, rg->load);

                if (result->in_flash[i + 1])
                {
                    rbtree_node *ra_node = search_grt(rght->grt, delete_grt_key);
                    free(ra_node->addr);
                }
                else
                {
                    rb_removeByKey(rght->lru, delete_node);
                }
                // update rb_Node info
                node1->is_RA = false;
                node1->min = rg->min;
                node1->max = rg->max;
                node1->load = rg->load;
                delete_grt(rght->grt, delete_grt_key);
                rbtree_node *check_node = search_grt(rght->grt, delete_grt_key);
                if (check_node->key == delete_grt_key)
                {
                    printf("delete grt err\n");
                }
                arr[arr_len++] = node1->min;
                i++;
            }
            else
            {
                arr[arr_len++] = node1->min;
                arr[arr_len++] = node2->min;
            }
        }
        for (uint64_t i = 0; i < arr_len; i++)
        {
            lru_node *nodex = NULL;
            nodex = find_range_block(rght->lru, arr[i]);
            rbtree_node *nodey = search_grt(rght->grt, arr[i]);
            if (nodex == NULL && nodey->is_RA == false)
            {
                ftl_err("compact err\n");
            }
        }
        free(arr);
        free(result->keys);
        free(result->in_flash);
        free(result);
    }
    return;
}

// /// @brief Try to compact 3 rangebucket
// /// @param RGHT
// void compact_RGHT_v2(RGHT *rght)
// {
//     // get all keys
//     KeyArray *result = (KeyArray *)malloc(sizeof(KeyArray));
//     result->keys = malloc(RB_IN_NAND_CAPACITY * sizeof(uint64_t));
//     result->in_flash = malloc(RB_IN_NAND_CAPACITY * sizeof(uint64_t));
//     result->len = 0;

//     // get all keys
//     KeyArray *check = (KeyArray *)malloc(sizeof(KeyArray));
//     check->keys = malloc(RB_IN_NAND_CAPACITY * sizeof(uint64_t));
//     check->in_flash = malloc(RB_IN_NAND_CAPACITY * sizeof(uint64_t));
//     check->len = 0;

//     rbtree_get_keys(rght->grt->g_range_tbl, rght->grt->g_range_tbl->root, result);
//     // printf("Number of range bucket record in GRT:%d\n",result->len);
//     int len = result->len;
//     for (int i = 0; i < len - 2; i++)
//     {
//         rb_Node *node1 = NULL;
//         rb_Node *node2 = NULL;
//         rb_Node *node3 = NULL;
//         if (result->in_flash[i])
//         {
//             node1 = find_range_block(rght->nand_rb, result->keys[i]);
//         }
//         else
//         {
//             node1 = find_range_block(rght->rb, result->keys[i]);
//         }
//         if (result->in_flash[i + 1])
//         {
//             node2 = find_range_block(rght->nand_rb, result->keys[i + 1]);
//         }
//         else
//         {
//             node2 = find_range_block(rght->rb, result->keys[i + 1]);
//         }
//         if (result->in_flash[i + 2])
//         {
//             node3 = find_range_block(rght->nand_rb, result->keys[i + 2]);
//         }
//         else
//         {
//             node3 = find_range_block(rght->rb, result->keys[i + 2]);
//         }

//         if (node1 == NULL)
//         {
//             printf("ERR bucket 1:%ld ,in_flash:%d is NOT found\n", result->keys[i], result->in_flash[i]);
//             exit(0);
//         }
//         if (node2 == NULL)
//         {
//             printf("ERR bucket 2:%ld ,in flash:%d is NOT found\n", result->keys[i + 1], result->in_flash[i + 1]);
//             exit(0);
//         }
//         if (node3 == NULL)
//         {
//             printf("ERR bucket 3:%ld ,in flash:%d is NOT found\n", result->keys[i + 2], result->in_flash[i + 2]);
//             exit(0);
//         }
//         // uint64_t key1 = node1->range_bucket->min;
//         uint64_t key2 = node2->min;
//         uint64_t key3 = node3->min;

//         int total_load = node1->load + node2->load + node3->load;
//         if (total_load > ((PAGE_CAPACITY * ASSOC_NUM - STASH_SPLIT_NUM * 4) * 2))
//         {
//             check->keys[check->len] = node1->min;
//             check->in_flash[check->len] = result->in_flash[i];
//             check->len++;
//             continue;
//         }
//         check->keys[check->len] = node1->min;
//         check->in_flash[check->len] = result->in_flash[i];
//         check->len++;
//         if (range_buckets_compact_three_to_two(&(node1->range_block), &(node2->range_block), &(node3->range_block)))
//         {
//             check->keys[check->len] = node2->min;
//             check->in_flash[check->len] = result->in_flash[i + 1];
//             check->len++;
//             // compact succeed,node2 should be deleted
//             // printf("(%ld,%ld,%ld)total load %d->[%ld,%ld,%d] [%ld,%ld,%d]\n",key1,key2,key3,total_load ,node1->range_bucket->min,node1->range_bucket->max, node1->range_bucket->load,node2->range_bucket->min, node2->range_bucket->max,node2->range_bucket->load);
//             if (result->in_flash[i + 2])
//             {
//                 rb_removeByKey(rght->nand_rb, key3);
//                 ftl_assert(rb_check_bucket(rght->nand_rb, key3) == false);
//             }
//             else
//             {
//                 rb_removeByKey(rght->rb, key3);
//                 ftl_assert(rb_check_bucket(rght->rb, key3) == false);
//             }
//             rbtree_deleteByKey(rght->grt->g_range_tbl, key2);
//             rbtree_deleteByKey(rght->grt->g_range_tbl, key3);

//             insert_grt(rght->grt, node2->min, 0);
//             if (result->in_flash[i + 1])
//             {
//                 set_in_flash(rght->grt, node2->min);
//             }

//             // change hash list of node2
//             if (result->in_flash[i + 1])
//             {
//                 rb_move_to_new_hash_list(rght->nand_rb, node2, key2);
//             }
//             else
//             {
//                 rb_move_to_new_hash_list(rght->rb, node2, key2);
//             }

//             // check  node2
//             rb_Node *nodex = NULL;
//             rbtree_node *tmp_node = search_grt(rght->grt, node2->min);
//             if (tmp_node->key != node2->min)
//             {
//                 ftl_err("node not equal\n");
//                 exit(0);
//             }
//             nodex = find_range_block(rght->nand_rb, key2);
//             nodex = find_range_block(rght->rb, key2);
//             if (nodex != NULL && node2->min != key2)
//             {
//                 ftl_err("Old key2 did not removed\n");
//                 exit(0);
//             }
//             if (tmp_node->in_flash)
//             {
//                 nodex = find_range_block(rght->nand_rb, node2->min);
//             }
//             else
//             {
//                 nodex = find_range_block(rght->rb, node2->min);
//             }
//             if (nodex == NULL)
//             {
//                 printf("Insert %lu failed ,inflash %d\n", node2->min, result->in_flash[i + 2]);
//                 exit(0);
//             }

//             rb_Node *nodey = NULL;
//             rbtree_node *tmp_node3 = search_grt(rght->grt, key3);
//             if (tmp_node3->key != node2->min && tmp_node3->key != node1->min)
//             {
//                 ftl_err("node3 still in grt\n");
//                 exit(0);
//             }
//             nodey = find_range_block(rght->nand_rb, key3);
//             nodey = find_range_block(rght->rb, key3);
//             if (nodey != NULL && key3 != node2->min)
//             {
//                 ftl_err("Node3 didnot remove\n");
//                 exit(0);
//             }
//             if (node1->load + node2->load != total_load)
//             {
//                 ftl_err("Load err after compact\n");
//                 exit(0);
//             }
//             i += 2;
//         }
//         else
//         {
//             check->keys[check->len] = node2->min;
//             check->in_flash[check->len] = result->in_flash[i + 1];
//             check->len++;
//             check->keys[check->len] = node3->min;
//             check->in_flash[check->len] = result->in_flash[i + 2];
//             check->len++;
//         }
//     }

//     for (size_t j = 0; j < check->len; j++)
//     {
//         if (check->in_flash[j])
//         {
//             if (find_range_block(rght->nand_rb, check->keys[j]) == NULL)
//             {
//                 ftl_err("compact v2 check err\n");
//                 exit(0);
//             }
//         }
//         else
//         {
//             if (find_range_block(rght->rb, check->keys[j]) == NULL)
//             {
//                 ftl_err("compact v2 check err\n");
//                 exit(0);
//             }
//         }
//     }
//     free(check->keys);
//     free(check->in_flash);
//     free(check);

//     free(result->keys);
//     free(result->in_flash);
//     free(result);
//     return;
// }

bool deconstruct_pma0_seg(RGHT *rght, seg_meta *seg_meta_arr, uint64_t seg_idx)
{
    // Insert valid value to RBHT
    uint64_t swap = 0;
    for (int i = 0; i < NODE_CNT_PER_SEG; i++)
    {
        if (seg_meta_arr[seg_idx].pma0_seg_addr[i] != UNMAPPED_PPA)
        {
            uint64_t key = (seg_idx * NODE_CNT_PER_SEG + i) * NODE_SIZE;
            uint64_t value = seg_meta_arr[seg_idx].pma0_seg_addr[i];
            // ftl_log("Flush pma0 %lu ,k %lu v %lu\n",seg_idx,key,value);
            update_RGHT(rght, seg_meta_arr, key, value, true, &swap);
        }
    }
    // free pma0 seg
    free(seg_meta_arr[seg_idx].pma0_seg_addr);
    seg_meta_arr[seg_idx].pma0_seg_addr = NULL;
    return true;
}