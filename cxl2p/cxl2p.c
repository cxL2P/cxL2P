#include <stdbool.h>
#include "cxl2p.h"

cxl2p_t *l2p_map = NULL;

uint32_t map_node_size;

cxl2p_t *cxl2p_init(int tt_pgs)
{
    int node_cnt = (tt_pgs + NODE_SIZE - 1) / NODE_SIZE;
    int seg_cnt = node_cnt / NODE_CNT_PER_SEG;
    l2p_map = (cxl2p_t *)malloc(sizeof(cxl2p_t));
    if (!l2p_map)
        return NULL;

    // Ideal dram size equals to tt_pgs * 4
    uint32_t dram_size = tt_pgs;
    int range_bucket_num = (dram_size - node_cnt * 2 - seg_cnt * 12) / (1024 * 64);
    map_node_size = node_cnt * 2 + seg_cnt * 12;

    // Init RGHT
    l2p_map->rght = NULL;
    l2p_map->rght = init_RGHT(l2p_map->rght, range_bucket_num, tt_pgs);

    // Init cArray
    l2p_map->bmp_array = (BMP_TYPE *)malloc(node_cnt * sizeof(BMP_TYPE));
    memset(l2p_map->bmp_array, 0, node_cnt * sizeof(BMP_TYPE));

    l2p_map->seg_meta_arr = (seg_meta *)malloc(seg_cnt * sizeof(seg_meta));
    segs_meta_init(l2p_map->seg_meta_arr, seg_cnt);
    l2p_map->pma0_seg_cnt = 0;
    return l2p_map;
}

void cxl2p_destroy(cxl2p_t *l2p_map)
{
    destory_RGHT(l2p_map->rght);
    free(l2p_map->bmp_array);
    free(l2p_map->seg_meta_arr);
    free(l2p_map);
}

void cxl2p_insert(cxl2p_t *l2p_map, uint64_t key, uint64_t value, uint64_t old_value, uint64_t *cxl_lat)
{
    uint64_t node_idx = key / NODE_SIZE;
    uint32_t in_node_offset = key % NODE_SIZE;
    uint64_t seg_idx = key / (NODE_CNT_PER_SEG * NODE_SIZE);
    uint32_t in_seg_offset = node_idx % NODE_CNT_PER_SEG;
    bool update_later = false;

    // pre-caculate pointers to frequently used data
    uint64_t *pma0_seg_addr = l2p_map->seg_meta_arr[seg_idx].pma0_seg_addr;
    BMP_TYPE *bmap = &(l2p_map->bmp_array[node_idx]);
    seg_meta *seg_meta_addr = &(l2p_map->seg_meta_arr[seg_idx]);

    if (in_node_offset != (NODE_SIZE - 1) && bit_map_check_bit(bmap, in_node_offset + 1))
    {
        update_later = true;
    }

    // Determine the construction or deconstruction of PMA0 Seg
    if (seg_meta_addr->total_seg_insert % (LMA_SEG_SIZE / 50) == 0)
    {
        if (pma0_seg_addr == NULL && (seg_meta_addr->total_seg_insert - seg_meta_addr->total_bmp_insert) < CONSTRUCT_LAMDA)
        {
            uint64_t *pma0_seg = (uint64_t *)malloc(NODE_CNT_PER_SEG * sizeof(uint64_t));
            memset(pma0_seg, 0xff, NODE_CNT_PER_SEG * sizeof(uint64_t));

            l2p_map->seg_meta_arr[seg_idx].pma0_seg_addr = pma0_seg;
            l2p_map->pma0_seg_cnt++;
            // printf("Construct Pma0 Seg %ld ,pma0_addr %p,pam0 %lu\n", seg_idx, l2p_map->seg_meta_arr[seg_idx].pma0_seg_addr, pma0_seg[0]);
            // printf("ID %lu seg_insert_cnt %lu bmp_insert_cnt %lu LMADA %u\n", seg_idx, seg_meta_addr->total_seg_insert, seg_meta_addr->total_bmp_insert, CONSTRUCT_LAMDA);
        }
    }

    // printf("ID %lu seg_insert_cnt %lu bmp_insert_cnt %lu LMADA %u\n", seg_idx, seg_meta_addr->total_seg_insert, seg_meta_addr->total_bmp_insert, DECONSTRUCT_LAMDA);
    if (pma0_seg_addr != NULL && seg_meta_addr->total_bmp_insert % (LMA_SEG_SIZE / 50) == 0 && (seg_meta_addr->total_seg_insert - seg_meta_addr->total_bmp_insert) > DECONSTRUCT_LAMDA)
    {
        deconstruct_pma0_seg(l2p_map->rght, l2p_map->seg_meta_arr, seg_idx);
        l2p_map->seg_meta_arr[seg_idx].pma0_seg_addr = NULL;
        l2p_map->pma0_seg_cnt--;
        // printf("Deconstruct Pma0 Seg %ld \n", seg_idx);
        pma0_seg_addr = l2p_map->seg_meta_arr[seg_idx].pma0_seg_addr;
    }

    // Update LMA segment insert cnt
    if (old_value == UNMAPPED_PPA && value != UNMAPPED_PPA)
    {
        ftl_assert(seg_meta_addr->total_seg_insert < LMA_SEG_SIZE);
        seg_meta_addr->total_seg_insert++;
    }
    else if (old_value != UNMAPPED_PPA && value == UNMAPPED_PPA)
    {
        seg_meta_addr->total_seg_insert--;
    }

    // first check the compact array to see whether the LMA can be stored in the PMA0 segment
    if (in_node_offset == 0)
    {
        if (old_value == UNMAPPED_PPA)
        {
            // if PMA0 segment is constucted
            if (pma0_seg_addr != NULL)
            {
                pma0_seg_addr[in_seg_offset] = value;
            }
            else
            {
                insert_RGHT(l2p_map->rght, l2p_map->seg_meta_arr, key, value, cxl_lat);
            }
        }
        else
        {
            if (value == UNMAPPED_PPA)
            {
                if (pma0_seg_addr != NULL && pma0_seg_addr[in_seg_offset] != UNMAPPED_PPA)
                {
                    pma0_seg_addr[in_seg_offset] = value;
                }
                else
                {
                    delete_RGHT(l2p_map->rght, key, cxl_lat);
                }
            }
            else
            {
                if (pma0_seg_addr != NULL)
                {
                    if (pma0_seg_addr[in_seg_offset] == UNMAPPED_PPA)
                    {
                        ftl_assert(delete_RGHT(l2p_map->rght, key, cxl_lat) != UNMAPPED_PPA);
                    }
                    pma0_seg_addr[in_seg_offset] = value;
                }
                else
                {
                    update_RGHT(l2p_map->rght, l2p_map->seg_meta_arr, key, value, false, cxl_lat);
                }
            }
        }
    }
    else if (!bit_map_check_bit(bmap, in_node_offset)) // old L2P entry is in RGHT
    {
        if (value == UNMAPPED_PPA)
        {
            // delete in RGHT
            delete_RGHT(l2p_map->rght, key, cxl_lat);
        }
        else if (value == (cxl2p_get(l2p_map, key - 1, cxl_lat) + 1))
        {
            bit_map_set_bit(bmap, in_node_offset);
            delete_RGHT(l2p_map->rght, key, cxl_lat);
            seg_meta_addr->total_bmp_insert++;
        }
        else
        {
            // Could be optimized to one operation
            delete_RGHT(l2p_map->rght, key, cxl_lat);
            insert_RGHT(l2p_map->rght, l2p_map->seg_meta_arr, key, value, cxl_lat);
        }
    }
    else // old L2P entry is in bmp
    {
        bit_map_clear_bit(bmap, in_node_offset);
        seg_meta_addr->total_bmp_insert--;
        if (value == UNMAPPED_PPA)
        {
            return;
        }
        else
        {
            // insert in rbht
            insert_RGHT(l2p_map->rght, l2p_map->seg_meta_arr, key, value, cxl_lat);
        }
    }

    // eliminate the influence on the next bit
    if (update_later)
    {
        bit_map_clear_bit(bmap, in_node_offset + 1);
        seg_meta_addr->total_bmp_insert--;
        if (old_value + 1 == UNMAPPED_PPA)
        {
            return;
        }
        else
        { // insert in rbht
            insert_RGHT(l2p_map->rght, l2p_map->seg_meta_arr, key + 1, old_value + 1, cxl_lat);
        }
    }

    return;
}

/*
 * return value of the key, error code is recorded in the last paramter
 */
uint64_t cxl2p_get(cxl2p_t *l2p_map, uint64_t key, uint64_t *cxl_lat)
{
    BMP_TYPE cnt = 0;
    uint64_t node_idx = key / NODE_SIZE;
    uint32_t in_node_offset = key % NODE_SIZE;
    uint64_t seg_idx = key / (NODE_CNT_PER_SEG * NODE_SIZE);
    uint32_t node_in_seg_offset = node_idx % NODE_CNT_PER_SEG;

    // pre-caculate pointers to frequently used data
    uint64_t *pma0_seg_addr = l2p_map->seg_meta_arr[seg_idx].pma0_seg_addr;
    BMP_TYPE *bmap = &(l2p_map->bmp_array[node_idx]);

    // first check the PMA0 segment
    if (key % NODE_SIZE == 0)
    {
        // if pma0 segment is constructed
        if (pma0_seg_addr != NULL)
        {
            // if entry is in RGHT, delete and update in pma0
            if (pma0_seg_addr[node_in_seg_offset] == UNMAPPED_PPA)
            {
                uint64_t result = delete_RGHT(l2p_map->rght, key, cxl_lat);
                pma0_seg_addr[node_in_seg_offset] = result;
                return result;
            }
            return pma0_seg_addr[node_in_seg_offset];
        }
        else
        {
            // PMA0 seg unconstructed
            return search_RGHT(l2p_map->rght, key, cxl_lat);
        }
    }

    cnt = get_farthest_left_neighbor(bmap, in_node_offset);

    if (!cnt)
    {

        uint64_t result = search_RGHT(l2p_map->rght, key, cxl_lat);
        // if (key == 2202345)
        //     printf("key %ld ,node idx %ld,cnt %d ,bmp %d result %ld\n", key, node_idx, cnt, l2p_map->map_node_array[node_idx].bmap, result);
        return result;
    }

    return cxl2p_get(l2p_map, key - cnt, cxl_lat) + cnt;
}
