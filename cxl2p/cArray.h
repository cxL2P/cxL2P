#ifndef _COMPACT_ARRAY_H_
#define _COMPACT_ARRAY_H_
#include "../common.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define check_key UNMAPPED_PPA
#define BMP_TYPE uint8_t

#define NODE_SIZE 8
#define NODE_CNT_PER_SEG (16384)
#define LMA_SEG_SIZE (NODE_CNT_PER_SEG * NODE_SIZE)
#define CONSTRUCT_LAMDA (LMA_SEG_SIZE * 3 / 8)
#define DECONSTRUCT_LAMDA (LMA_SEG_SIZE / 2)
#define INVALID_PPA (~(0ULL))
#define INVALID_LPN (~(0ULL))
#define UNMAPPED_PPA (~(0ULL))
#define INVALID_ADDR (~(0ULL))

typedef struct _seg_meta
{
    uint64_t *pma0_seg_addr;
    uint64_t total_bmp_insert;
    uint64_t total_seg_insert;
} seg_meta;

int seg_meta_init(seg_meta *seg);

/*initialize pma0 segs addr*/
int segs_meta_init(seg_meta *seg_meta_arr, int len);

int set_seg_meta_arr(seg_meta *seg_meta_arr, uint64_t key, uint64_t value);

/* check a bit of the bitmap or tagmap */
BMP_TYPE bit_map_check_bit(BMP_TYPE *map, BMP_TYPE bit_idx);

/* set a bit of the bitmap or tagmap  to indicate l2p[idx] == l2p[idx-1]+1 */
void bit_map_set_bit(BMP_TYPE *map, BMP_TYPE bit_idx);

/* clear a bit of the bitmap or tagmap to indicate l2p[idx] != l2p[idx-1]+1 */
void bit_map_clear_bit(BMP_TYPE *map, BMP_TYPE bit_idx);

/* calculate how many b'1s are ahead of bit_idx in bmap */
BMP_TYPE get_farthest_left_neighbor(BMP_TYPE *bmap, BMP_TYPE bit_idx);

/* set all bits of the bmap or tagmap with a value */
void bit_map_set_val(BMP_TYPE *map, uint32_t val);

void print_uint64_binary(uint64_t num);
#endif