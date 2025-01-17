#include "cArray.h"

int seg_meta_init(seg_meta *seg)
{
    if (!seg)
    {
        return 0;
    }
    seg->pma0_seg_addr = NULL;
    seg->total_bmp_insert = 0;
    seg->total_seg_insert = 0;
    return 1;
}

int segs_meta_init(seg_meta *seg_meta_arr, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (seg_meta_init(&seg_meta_arr[i]))
        {
            return -1;
        }
    }
    return 0;
}

int set_seg_meta_arr(seg_meta *seg_meta_arr, uint64_t key, uint64_t value)
{
    if (key % NODE_SIZE == 0)
    {
        uint64_t seg_idx = key / (NODE_SIZE * NODE_CNT_PER_SEG);
        uint32_t in_seg_offset = (key / NODE_SIZE) % NODE_CNT_PER_SEG;
        if (seg_meta_arr[seg_idx].pma0_seg_addr == NULL)
        {
            return 0;
        }
        if (key == check_key)
        {
            ftl_log("%lu %lu upload to seg %ld\n", key, value, seg_idx);
        }
        seg_meta_arr[seg_idx].pma0_seg_addr[in_seg_offset] = value;
        return 1;
    }
    return 0;
}

BMP_TYPE bit_map_check_bit(BMP_TYPE *map, BMP_TYPE bit_idx)
{
    return *map & (1 << bit_idx);
}

void bit_map_set_bit(BMP_TYPE *map, BMP_TYPE bit_idx)
{
    *map |= (1 << bit_idx);
}

void bit_map_clear_bit(BMP_TYPE *map, BMP_TYPE bit_idx)
{
    *map &= ~(1 << bit_idx);
}

/*
 * return 0 indicates pma[idx] cannot be derived from the compact array
 * return n indicates pma[idx] = pma[idx-n] + n
 * */
BMP_TYPE get_farthest_left_neighbor(BMP_TYPE *bmap, BMP_TYPE bit_idx)
{
    BMP_TYPE cnt = 0;

    while ((bit_idx >= cnt) && ((*bmap >> (bit_idx - cnt)) & 1))
        cnt++;
    return cnt;
}

void print_uint64_binary(uint64_t num)
{
    for (int i = 63; i >= 0; i--)
    {
        uint64_t mask = 1ULL << i;
        ftl_log("%d", (num & mask) ? 1 : 0);

        if (i % 8 == 0)
        {
            ftl_log(" "); // Print a space every 8 bits for better readability
        }
    }
    ftl_log("\n");
}