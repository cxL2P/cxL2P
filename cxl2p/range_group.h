#ifndef RANGE_GROUP_H
#define RANGE_GROUP_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "grt.h"
#define ASSOC_NUM 4
#define RA_TRANS_THRESHOLD (10 * 16384)
#define RA_CAPACITY 16384
#define PAGE_CAPACITY 1980
#define ALTERNATIVE_NUM 10
#define STASH_NUM 2 * ALTERNATIVE_NUM
#define STASH_SPLIT_NUM 10
#define HASH_ACCESS_NUM (PAGE_CAPACITY - STASH_NUM - STASH_SPLIT_NUM)

// entry
typedef struct entry
{
    uint64_t key;
    uint64_t value;
} entry;

typedef struct bucket
{
    uint8_t token; // used as a bitmap to indicate entry validness in a slot
    entry slot[ASSOC_NUM];
} __attribute__((packed)) bucket;

typedef struct linear_probing_hash
{ // A linear probing hash
    uint64_t min;
    uint64_t max;
    uint16_t load;
    bucket *buckets;
    bucket *stash_buckets; // buckets for dealing with normal hash insert conflict
    bucket *split_buckets; // buckets for stash hash conflict after spliting
} __attribute__((packed)) range_group;

typedef struct _range_array
{
    uint64_t min;
    uint64_t max;
    uint16_t load;
    uint64_t *arr;
} __attribute__((packed)) range_array;

typedef struct _rg_stat
{
    uint64_t entry_in_hash_bkt;
    uint64_t entry_in_stah_bkt;
    uint64_t entry_in_split_bkt;
} rg_stat;

uint8_t range_group_insert_split_stash(range_group *rg, uint64_t key, uint64_t value);

range_group *range_group_init(uint64_t min, uint64_t max);

void range_group_stat(range_group *rg, uint64_t *hashCnt, uint64_t *stashCnt, uint64_t *splitCnt);

/*
function : insert  an entry into range group;
return : 1 means insertion succeed while 0 means insertion fail
*/
uint8_t range_group_insert(range_group *rg, uint64_t key, uint64_t value);

uint64_t range_group_search(range_group *rg, uint64_t key);

uint64_t range_group_delete(range_group *rg, uint64_t key);

uint8_t range_group_update(range_group *rg, uint64_t key, uint64_t new_value);

void **range_group_split(range_group *old_rg, seg_meta *seg_meta_arr, uint64_t grt_max_key, bool *trans2RA, bool is_pma0_flush, int *nums_of_RA);

void range_group_destory(range_group *rg);

void range_group_print(range_group *rg);

uint64_t insertion_sort_median(entry *arr, uint32_t n);

bool range_groups_compact_three_to_two(range_group **rg1, range_group **rg2, range_group **rg3);

bool range_groups_compact(range_group **rg1, range_group **rg2);

bool range_array_compact(void **ra1, void **ra2);

bool RA_RG_compact(range_group **rg, range_array **ra, bool is_seq);

/*
is_seq : RA key > RG key
mode 1: RG RG compact
mode 2: RA RG compact
mode 3: RA RA compact
*/
bool range_block_compact(void **range_block1, void **range_block2, bool is_seq, int mode);

// RA Implementation
range_array *range_array_init(uint64_t min, uint64_t max);

uint8_t range_array_insert(range_array *ra, uint64_t key, uint64_t value);

uint64_t range_array_search(range_array *ra, uint64_t key);

/*
res = 1:delete a value ,load --
res = 0:key not found
*/
uint64_t range_array_delete(range_array *ra, uint64_t key);

void range_array_destory(range_array *ra);
#endif