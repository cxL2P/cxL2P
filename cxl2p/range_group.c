#include "range_group.h"
#include <inttypes.h>
#define HASH_NUM 2
uint64_t split_num = 0;
uint64_t used_split_slot = 0;
/*
function: initialize range bucket
*/
range_group *range_group_init(uint64_t min, uint64_t max)
{
    // printf("RG init min %ld max %ld\n", min, max);
    int hash_access_num = HASH_ACCESS_NUM;
    range_group *rg = (range_group *)malloc(sizeof(range_group));
    rg->buckets = (bucket *)calloc(hash_access_num, sizeof(bucket));
    rg->stash_buckets = (bucket *)calloc(STASH_NUM, sizeof(bucket));
    rg->split_buckets = (bucket *)calloc(STASH_SPLIT_NUM, sizeof(bucket));
    rg->max = max;
    rg->min = min;
    rg->load = 0;
    return rg;
}

range_array *range_array_init(uint64_t min, uint64_t max)
{
    ftl_assert((max - min) < RA_CAPACITY);
    // ftl_log("RA init min %ld max %ld\n", min, max);
    range_array *ra = (range_array *)malloc(sizeof(range_array));
    ra->min = min;
    ra->max = max;
    ra->load = 0;
    ra->arr = (uint64_t *)malloc((max - min) * sizeof(uint64_t));
    memset(ra->arr, 0xff, (max - min) * sizeof(uint64_t));
    ftl_assert(ra->arr[0] == UNMAPPED_PPA);
    return ra;
}

/*
function:insert an entry into range bucket
*/
uint8_t range_group_insert(range_group *rg, uint64_t key, uint64_t value)
{
    if (key == check_key)
        printf("Insert:%lu %lu in RG %lu\n", key, value, rg->min);
    uint32_t step_len = HASH_ACCESS_NUM / HASH_NUM;
    uint32_t hash_access = HASH_ACCESS_NUM;
    // insert in hash pos
    for (int i = 0; i < HASH_NUM; i++)
    {
        uint32_t index = ((key + i * step_len) % hash_access);

        bucket *b = &(rg->buckets[index]);
        if (b->token < ASSOC_NUM)
        {
            b->slot[b->token].key = key;
            b->slot[b->token].value = value;
            b->token++;
            if (key > rg->max)
            {
                rg->max = key;
            }
            rg->load++;
            return 1;
        }
    }

    // Try to insert next to hash pos
    for (int i = 0; i < HASH_NUM; i++)
    {
        uint32_t index = ((key + i * step_len) % hash_access);
        for (int j = 1; j <= ALTERNATIVE_NUM; j++)
        {
            uint32_t new_index = (index + j) % hash_access;
            bucket *b = &(rg->buckets[new_index]);
            if (b->token < ASSOC_NUM)
            {
                b->slot[b->token].key = key;
                b->slot[b->token].value = value;
                b->token++;
                if (key > rg->max)
                {
                    rg->max = key;
                }
                rg->load++;
                return 1;
            }
        }
    }

    // Try to insert in stash buckets
    for (int i = 0; i < STASH_NUM; i++)
    {
        bucket *b = &(rg->stash_buckets[i]);
        if (b->token < ASSOC_NUM)
        {
            b->slot[b->token].key = key;
            b->slot[b->token].value = value;
            b->token++;
            if (key > rg->max)
            {
                rg->max = key;
            }
            rg->load++;
            return 1;
        }
    }

    return 0; // Couldn't insert in any buckets
}

uint8_t range_group_insert_split_stash(range_group *rg, uint64_t key, uint64_t value)
{
    for (int i = 0; i < STASH_SPLIT_NUM; i++)
    {
        bucket *b = &(rg->split_buckets[i]);
        if (b->token < ASSOC_NUM)
        {
            // printf("insert in split stash:key=%ld,value=%ld\n", key, value);
            b->slot[b->token].key = key;
            b->slot[b->token].value = value;
            b->token++;
            rg->load++;
            used_split_slot++;
            return 1;
        }
    }
    return 0;
}

/*
function: Search a key in range group
*/
uint64_t range_group_search(range_group *rg, uint64_t key)
{
    if (key == check_key)
    {
        printf("search %lu in RG %ld %ld\n", key, rg->min, rg->max);
    }
    uint32_t step_len = HASH_ACCESS_NUM / HASH_NUM;
    uint32_t hash_access = HASH_ACCESS_NUM;
    // search in hash position
    for (int i = 0; i < HASH_NUM; i++)
    {
        uint32_t index = ((key + i * step_len) % hash_access);
        bucket *b = &(rg->buckets[index]);
        for (int i = 0; i < b->token; i++)
        {
            if (b->slot[i].key == key)
            {
                return b->slot[i].value;
            }
        }
    }

    // search in alter position
    for (int i = 0; i < HASH_NUM; i++)
    {
        uint32_t index = ((key + i * step_len) % hash_access);
        for (int j = 1; j <= ALTERNATIVE_NUM; j++)
        {
            uint32_t new_index = (index + j) % hash_access;
            bucket *b = &(rg->buckets[new_index]);
            for (int i = 0; i < b->token; i++)
            {
                if (b->slot[i].key == key)
                {
                    return b->slot[i].value;
                }
            }
        }
    }

    // Try to search in stash bucket
    for (int j = 0; j < STASH_NUM; j++)
    {
        bucket *b = &(rg->stash_buckets[j]);
        for (int i = 0; i < b->token; i++)
        {
            if (b->slot[i].key == key)
            {
                return b->slot[i].value;
            }
        }
    }

    // Try to search in split stash bucket
    for (int i = 0; i < STASH_SPLIT_NUM; i++)
    {
        bucket *b = &(rg->split_buckets[i]);
        for (int i = 0; i < b->token; i++)
        {
            if (b->slot[i].key == key)
            {
                return b->slot[i].value;
            }
        }
    }

    // traverse hash buckets
    for (int i = 0; i < HASH_ACCESS_NUM; i++)
    {
        bucket *b = &(rg->buckets[i]);
        for (int i = 0; i < b->token; i++)
        {
            if (b->slot[i].key == key)
            {
                return b->slot[i].value;
            }
        }
    }
    return UNMAPPED_PPA; // Key not found
}

/*
function : delete a key in range bucket
*/
uint64_t range_group_delete(range_group *rg, uint64_t key)
{
    if (key == check_key)
        printf("RG delete:%lu in RG %lu\n", key, rg->min);
    uint32_t step_len = HASH_ACCESS_NUM / HASH_NUM;
    uint32_t hash_access = HASH_ACCESS_NUM;
    uint64_t result = UNMAPPED_PPA;
    if (rg->max == key)
    {
        rg->max = key - 1;
    }

    // check in hash buckets
    for (int i = 0; i < HASH_NUM; i++)
    {
        uint32_t index = ((key + i * step_len) % hash_access);
        bucket *b = &(rg->buckets[index]);
        for (int i = 0; i < b->token; i++)
        {
            if (b->slot[i].key == key)
            {
                result = b->slot[i].value;
                b->token--;
                b->slot[i] = b->slot[b->token]; // Move last entry to the deleted slot
                rg->load--;
                return result;
            }
        }
    }

    // check in altenative buckets
    for (int i = 0; i < HASH_NUM; i++)
    {
        uint32_t index = ((key + i * step_len) % hash_access);
        for (int j = 1; j <= ALTERNATIVE_NUM; j++)
        {
            uint32_t new_index = (index + j) % hash_access;
            bucket *b = &(rg->buckets[new_index]);
            for (int i = 0; i < b->token; i++)
            {
                if (b->slot[i].key == key)
                {
                    result = b->slot[i].value;
                    b->token--;
                    b->slot[i] = b->slot[b->token]; // Move last entry to the deleted slot
                    rg->load--;
                    return result;
                }
            }
        }
    }

    // try to delete in stash bucket
    for (int j = 0; j < STASH_NUM; j++)
    {
        bucket *b = &(rg->stash_buckets[j]);
        for (int i = 0; i < b->token; i++)
        {
            if (b->slot[i].key == key)
            {
                result = b->slot[i].value;
                b->token--;
                b->slot[i] = b->slot[b->token]; // Move last entry to the deleted slot
                rg->load--;
                return result;
            }
        }
    }

    // try to delete in split bucket
    for (int j = 0; j < STASH_SPLIT_NUM; j++)
    {
        bucket *b = &(rg->stash_buckets[j]);
        for (int i = 0; i < b->token; i++)
        {
            if (b->slot[i].key == key)
            {
                result = b->slot[i].value;
                b->token--;
                b->slot[i] = b->slot[b->token]; // Move last entry to the deleted slot
                rg->load--;
                return result;
            }
        }
    }

    return result; // Key not found
}

/*
return (0 failed) (1,insert) (2,update)
*/
uint8_t range_group_update(range_group *rg, uint64_t key, uint64_t new_value)
{
    uint32_t step_len = HASH_ACCESS_NUM / HASH_NUM;
    uint32_t hash_access = HASH_ACCESS_NUM;
    if (key == check_key)
        printf("update:%lu %lu in RG %lu\n", key, new_value, rg->min);
    //   Update range
    if (rg->max < key)
    {
        rg->max = key;
    }

    // try to insert in hash buckets
    for (int i = 0; i < HASH_NUM; i++)
    {
        uint32_t index = (key + i * step_len) % hash_access;
        bucket *b = &(rg->buckets[index]);
        for (int i = 0; i < b->token; i++)
        {
            if (b->slot[i].key == key)
            {
                b->slot[i].value = new_value;
                return 2;
            }
        }
    }

    // Try next to hash bucket
    // check in altenative buckets
    for (int i = 0; i < HASH_NUM; i++)
    {
        uint32_t index = ((key + i * step_len) % hash_access);
        for (int j = 1; j <= ALTERNATIVE_NUM; j++)
        {
            uint32_t new_index = (index + j) % hash_access;
            bucket *b = &(rg->buckets[new_index]);
            for (int i = 0; i < b->token; i++)
            {
                if (b->slot[i].key == key)
                {
                    b->slot[i].value = new_value;
                    return 2;
                }
            }
        }
    }

    // Try stash bucket
    for (int j = 0; j < STASH_NUM; j++)
    {
        bucket *b = &(rg->stash_buckets[j]);

        for (int i = 0; i < b->token; i++)
        {
            if (b->slot[i].key == key)
            {
                b->slot[i].value = new_value;
                return 2;
            }
        }
    }

    // try update in split bucket
    for (int j = 0; j < STASH_SPLIT_NUM; j++)
    {
        bucket *b = &(rg->split_buckets[j]);

        for (int i = 0; i < b->token; i++)
        {
            if (b->slot[i].key == key)
            {
                b->slot[i].value = new_value;
                return 2;
            }
        }
    }
    return range_group_insert(rg, key, new_value);
}

/*
function: Split range bucket into two bucket  when the current bucket is full
*/
void **range_group_split(range_group *old_rg, seg_meta *seg_meta_arr, uint64_t grt_max_key, bool *trans2RA, bool is_pma0_flush, int *nums_of_RA)
{
    *trans2RA = false;
    split_num++;

    if ((old_rg->max - old_rg->min) > RA_TRANS_THRESHOLD)
    {
        uint64_t split_key = (old_rg->min + (old_rg->max - old_rg->min) / 2);

        // Create two new buckets
        range_group *new_rg1 = range_group_init(old_rg->min, split_key);
        range_group *new_rg2 = range_group_init(split_key, old_rg->max);
        // move normal buckets
        for (int i = 0; i < HASH_ACCESS_NUM; i++)
        {
            bucket *old_bucket = &(old_rg->buckets[i]);

            for (int j = 0; j < old_bucket->token; j++)
            {
                uint64_t key = old_bucket->slot[j].key;
                uint64_t value = old_bucket->slot[j].value;

                if (!is_pma0_flush && set_seg_meta_arr(seg_meta_arr, key, value) == 1)
                {
                    continue;
                }

                // Determine which new bucket the entry should go to
                uint32_t index = key >= split_key ? 1 : 0;

                // Insert the entry into the appropriate new bucket
                if (range_group_insert(index == 0 ? new_rg1 : new_rg2, key, value) == 0)
                {
                    if (range_group_insert_split_stash(index == 0 ? new_rg1 : new_rg2, key, value) == 0)
                    {
                        printf("FAILED RG[%ld %ld] split into RG [%ld %ld] [%ld %ld]\n", old_rg->min, old_rg->max, new_rg1->min, new_rg1->max, new_rg2->min, new_rg2->max);
                        exit(0);
                    }
                }
            }
        }
        // move normal stash buckets
        for (int i = 0; i < STASH_NUM; i++)
        {
            bucket *old_bucket = &(old_rg->stash_buckets[i]);
            for (int j = 0; j < old_bucket->token; j++)
            {
                uint64_t key = old_bucket->slot[j].key;
                uint64_t value = old_bucket->slot[j].value;

                if (!is_pma0_flush && set_seg_meta_arr(seg_meta_arr, key, value) == 1)
                {
                    continue;
                }
                // Determine which new bucket the entry should go to
                uint32_t index = key >= split_key ? 1 : 0;

                // Insert the entry into the appropriate new bucket
                if (range_group_insert(index == 0 ? new_rg1 : new_rg2, key, value) == 0)
                {
                    if (range_group_insert_split_stash(index == 0 ? new_rg1 : new_rg2, key, value) == 0)
                    {
                        printf("FAILED RG[%ld %ld] split into RG [%ld %ld] [%ld %ld]\n", old_rg->min, old_rg->max, new_rg1->min, new_rg1->max, new_rg2->min, new_rg2->max);
                        exit(0);
                    }
                }
            }
        }
        // move split stash buckets
        for (int i = 0; i < STASH_SPLIT_NUM; i++)
        {
            bucket *old_bucket = &(old_rg->split_buckets[i]);
            for (int j = 0; j < old_bucket->token; j++)
            {
                uint64_t key = old_bucket->slot[j].key;
                uint64_t value = old_bucket->slot[j].value;

                if (!is_pma0_flush && set_seg_meta_arr(seg_meta_arr, key, value) == 1)
                {
                    continue;
                }

                // Determine which new bucket the entry should go to
                uint32_t index = key >= split_key ? 1 : 0;

                // Insert the entry into the appropriate new bucket
                if (range_group_insert(index == 0 ? new_rg1 : new_rg2, key, value) == 0)
                {
                    if (range_group_insert_split_stash(index == 0 ? new_rg1 : new_rg2, key, value) == 0)
                    {
                        printf("FAILED RG[%ld %ld] split into RG [%ld %ld] [%ld %ld]\n", old_rg->min, old_rg->max, new_rg1->min, new_rg1->max, new_rg2->min, new_rg2->max);
                        exit(0);
                    }
                }
            }
        }
        // printf("RG[%ld %ld %d] split into RG [%ld %ld %d] [%ld %ld %d]\n", old_rg->min, old_rg->max, old_rg->load, new_rg1->min, new_rg1->max, new_rg1->load, new_rg2->min, new_rg2->max, new_rg2->load);

        // Replace the old bucket with the new buckets
        free(old_rg->buckets);
        free(old_rg->split_buckets);
        free(old_rg->stash_buckets);
        old_rg->buckets = new_rg1->buckets;
        old_rg->stash_buckets = new_rg1->stash_buckets;
        old_rg->split_buckets = new_rg1->split_buckets;
        old_rg->max = new_rg1->max;
        old_rg->min = new_rg1->min;
        old_rg->load = new_rg1->load;

        range_group **new_rg = (range_group **)malloc(sizeof(range_group *));
        new_rg[0] = new_rg2;
        return (void **)new_rg;
    }
    else
    {
        // Split RG into RAs
        *trans2RA = true;
        // split into x Range Array
        int range_array_num = (old_rg->max - old_rg->min + RA_CAPACITY - 1) / RA_CAPACITY;
        *nums_of_RA = range_array_num;
        // Create range arrays
        range_array **range_arrays = (range_array **)malloc(range_array_num * sizeof(range_array *));
        uint64_t temp_min = old_rg->min;
        // printf("RG[%ld %ld %p],max key %ld split into RAs ", old_rg->min, old_rg->max, old_rg, grt_max_key);
        for (int i = 0; i < range_array_num; i++)
        {
            if (i == range_array_num - 1)
            {
                if (old_rg->min >= grt_max_key)
                {
                    range_arrays[i] = range_array_init(temp_min, old_rg->max + 1);
                    // printf("[%ld %ld %p]\n", temp_min, old_rg->max + 1,range_arrays[i]);
                }
                else
                {
                    range_arrays[i] = range_array_init(temp_min, old_rg->max);
                    // printf("[%ld %ld %p]\n", temp_min, old_rg->max,range_arrays[i]);
                }
            }
            else
            {
                range_arrays[i] = range_array_init(temp_min, temp_min + RA_CAPACITY);
                // printf("[%ld %ld %p] ", temp_min, temp_min + RA_CAPACITY,range_arrays[i]);
                temp_min += RA_CAPACITY;
            }
        }
        // move normal buckets
        for (int i = 0; i < HASH_ACCESS_NUM; i++)
        {
            bucket *old_bucket = &(old_rg->buckets[i]);

            for (int j = 0; j < old_bucket->token; j++)
            {
                uint64_t key = old_bucket->slot[j].key;
                uint64_t value = old_bucket->slot[j].value;

                // if (set_seg_meta_arr(seg_meta_arr, key, value) == 1)
                // {
                //     continue;
                // }

                // Determine which new bucket the entry should go to
                for (int x = 0; x < range_array_num; x++)
                {
                    if (key < range_arrays[x]->max && key >= range_arrays[x]->min)
                    {
                        range_array_insert(range_arrays[x], key, value);
                    }
                }
            }
        }
        // move normal stash buckets
        for (int i = 0; i < STASH_NUM; i++)
        {
            bucket *old_bucket = &(old_rg->stash_buckets[i]);
            for (int j = 0; j < old_bucket->token; j++)
            {
                uint64_t key = old_bucket->slot[j].key;
                uint64_t value = old_bucket->slot[j].value;

                // if (set_seg_meta_arr(seg_meta_arr, key, value) == 1)
                // {
                //     continue;
                // }

                // Determine which new bucket the entry should go to
                for (int x = 0; x < range_array_num; x++)
                {
                    if (key <= range_arrays[x]->max && key >= range_arrays[x]->min)
                    {
                        range_array_insert(range_arrays[x], key, value);
                    }
                }
            }
        }
        // move split stash buckets
        for (int i = 0; i < STASH_SPLIT_NUM; i++)
        {
            bucket *old_bucket = &(old_rg->split_buckets[i]);
            for (int j = 0; j < old_bucket->token; j++)
            {
                uint64_t key = old_bucket->slot[j].key;
                uint64_t value = old_bucket->slot[j].value;

                // if (set_seg_meta_arr(seg_meta_arr, key, value) == 1)
                // {
                //     continue;
                // }
                // Determine which new bucket the entry should go to
                for (int x = 0; x < range_array_num; x++)
                {
                    if (key <= range_arrays[x]->max && key >= range_arrays[x]->min)
                    {
                        range_array_insert(range_arrays[x], key, value);
                    }
                }
            }
        }
        short total_load = 0;
        for (int x = 0; x < range_array_num; x++)
        {
            total_load += range_arrays[x]->load;
        }
        ftl_assert(old_rg->load == total_load);
        return (void **)range_arrays;
    }
}

bool range_block_compact(void **range_block1, void **range_block2, bool is_seq, int mode)
{
    switch (mode)
    {
    case 0:
        return range_groups_compact((range_group **)range_block1, (range_group **)range_block2);
        break;
    case 1:
        return RA_RG_compact((range_group **)range_block1, (range_array **)range_block2, is_seq);
        break;

    case 2:
        bool result = range_array_compact(range_block1, range_block2);
        return result;
        break;
    default:
        printf("Err invalid mode input\n");
        exit(0);
        break;
    }
}

bool range_groups_compact(range_group **bucket1, range_group **bucket2)
{
    int total_entries = (*bucket1)->load + (*bucket2)->load;

    entry *entries = (entry *)malloc(total_entries * sizeof(entry));

    int entry_count = 0;

    // store normal buckets in buckets1
    for (int i = 0; i < HASH_ACCESS_NUM; i++)
    {
        bucket *b = &((*bucket1)->buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // store normal stash buckets in buckets1
    for (int i = 0; i < STASH_NUM; i++)
    {
        bucket *b = &((*bucket1)->stash_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // store split stash buckets  in buckets1
    for (int i = 0; i < STASH_SPLIT_NUM; i++)
    {
        bucket *b = &((*bucket1)->split_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // store normal buckets in buckets2
    for (int i = 0; i < HASH_ACCESS_NUM; i++)
    {
        bucket *b = &((*bucket2)->buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // store normal stash buckets in buckets2
    for (int i = 0; i < STASH_NUM; i++)
    {
        bucket *b = &((*bucket2)->stash_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // store split stash buckets  in buckets2
    for (int i = 0; i < STASH_SPLIT_NUM; i++)
    {
        bucket *b = &((*bucket2)->split_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // Initialize a new range bucket for the compacted entries
    range_group *compacted_range_group = range_group_init((*bucket1)->min, (*bucket2)->max);
    // Insert the sorted entries into the new range bucket
    for (int i = 0; i < total_entries; i++)
    {
        if (range_group_insert(compacted_range_group, entries[i].key, entries[i].value) == 0)
        {
            if (range_group_insert_split_stash(compacted_range_group, entries[i].key, entries[i].value) == 0)
            {
                ftl_log("insert in compact split stash failed\n");
                range_group_destory(compacted_range_group);
                free(entries);
                return false;
            }
        }
    }
    // range_group_destory(*bucket2);
    free((*bucket1)->buckets);
    free((*bucket1)->stash_buckets);
    free((*bucket1)->split_buckets);
    free(*bucket1);
    *bucket1 = compacted_range_group;
    free(entries);
    return true;
}

// return (NULL,failed) (RG,success)
//  In any case we process compact, we only compact to a RG
bool range_array_compact(void **range_array1, void **range_array2)
{
    range_array **RA1 = (range_array **)range_array1;
    range_array **RA2 = (range_array **)range_array2;
    // printf("RA1 [%ld %ld %d] RA2[%ld %ld %d]\n", (*RA1)->min, (*RA1)->max, (*RA1)->load, (*RA2)->min, (*RA2)->max, (*RA2)->load);
    // Init a RG
    range_group *compacted_range_group = range_group_init((*RA1)->min, (*RA2)->max);

    uint64_t min = (*RA1)->min;
    uint64_t value = UNMAPPED_PPA;
    short valid_cnt = 0;

    // Insert RA1's vaild entries into RG
    for (int i = 0; i < (*RA1)->max - min; i++)
    {
        value = (*RA1)->arr[i];
        if (value != UNMAPPED_PPA)
        {
            valid_cnt++;
            if (range_group_insert(compacted_range_group, min + i, value) == 0)
            {
                range_group_destory(compacted_range_group);
                return 0;
            }
        }
    }

    // check correctness
    ftl_assert(valid_cnt == (*RA1)->load);

    // Insert RA2's valid entries into RG
    valid_cnt = 0;
    min = (*RA2)->min;
    for (int i = 0; i < (*RA2)->max - min; i++)
    {
        value = (*RA2)->arr[i];
        if (value != UNMAPPED_PPA)
        {
            valid_cnt++;
            if (range_group_insert(compacted_range_group, min + i, value) == 0)
            {
                range_group_destory(compacted_range_group);
                return 0;
            }
        }
    }

    // check correctness
    assert(valid_cnt == (*RA2)->load);
    // range_array_destory(*RA2);
    // *RA2 = NULL;
    // *range_array2 = NULL;
    range_array_destory(*RA1);
    *RA1 = NULL;
    *range_array1 = NULL;
    *range_array1 = (void *)compacted_range_group;

    return 1;
}

/*
is_seq = true,RA key > RG key
*/
bool RA_RG_compact(range_group **RG, range_array **RA, bool is_seq)
{
    // Init a RG
    range_group *compact_RG = NULL;

    if (is_seq)
    {
        compact_RG = range_group_init((*RA)->min, (*RG)->max);
    }
    else
    {
        compact_RG = range_group_init((*RG)->min, (*RA)->max);
    }

    uint64_t min = (*RA)->min;
    uint64_t value = UNMAPPED_PPA;
    short valid_cnt = 0;
    // Insert RA's entry into compact_RG
    for (int i = 0; i < RA_CAPACITY; i++)
    {
        value = (*RA)->arr[i];
        if (value != UNMAPPED_PPA)
        {
            valid_cnt++;
            if (range_group_insert(compact_RG, min + i, value) == 0)
            {
                return 0;
            }
        }
    }
    ftl_assert(valid_cnt == compact_RG->load);
    // insert RG's entry into compact RG
    // insert RG's normal buckets
    for (int i = 0; i < HASH_ACCESS_NUM; i++)
    {
        bucket *b = &((*RG)->buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            range_group_insert(compact_RG, b->slot[j].key, b->slot[j].value);
        }
    }

    // insert RG's normal stash buckets
    for (int i = 0; i < STASH_NUM; i++)
    {
        bucket *b = &((*RG)->stash_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            range_group_insert(compact_RG, b->slot[j].key, b->slot[j].value);
        }
    }

    // insert RG's split stash buckets
    for (int i = 0; i < STASH_SPLIT_NUM; i++)
    {
        bucket *b = &((*RG)->split_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            range_group_insert(compact_RG, b->slot[j].key, b->slot[j].value);
        }
    }
    // range_array_destory(*RA);
    // *RA = NULL;
    range_group_destory(*RG);
    *RG = compact_RG;
    return 1;
}

void range_group_destory(range_group *rg)
{
    // printf("Destory RG %ld \n",range_group->min);
    free(rg->buckets);
    free(rg->split_buckets);
    free(rg->stash_buckets);
    free(rg);
}
void range_group_stat(range_group *rg, uint64_t *hashCnt, uint64_t *stashCnt, uint64_t *splitCnt)
{

    for (int i = 0; i < HASH_ACCESS_NUM; i++)
    {
        bucket *b = &(rg->buckets[i]);
        (*hashCnt) += b->token;
    }
    for (int i = 0; i < STASH_NUM; i++)
    {
        bucket *b = &(rg->stash_buckets[i]);
        (*stashCnt) += b->token;
    }
    for (int i = 0; i < STASH_SPLIT_NUM; i++)
    {
        bucket *b = &(rg->split_buckets[i]);
        (*splitCnt) += b->token;
    }
}

void range_group_print(range_group *rg)
{
    printf("Range Bucket Contents:\n");
    printf("Min Key: %lu\n", rg->min);
    printf("Max Key: %lu\n", rg->max);

    for (int i = 0; i < HASH_ACCESS_NUM; i++)
    {
        bucket *b = &(rg->buckets[i]);

        if (b->token > 0)
        {
            printf("Bucket %d:\n", i);
            for (int j = 0; j < b->token; j++)
            {
                printf("  Key: %lu, Value: %lu\n", b->slot[j].key, b->slot[j].value);
            }
        }
    }
    printf("Stash buckets:\n");
    for (int i = 0; i < STASH_NUM; i++)
    {
        bucket *b = &(rg->stash_buckets[i]);

        if (b->token > 0)
        {
            printf("Bucket %d:\n", i);
            for (int j = 0; j < b->token; j++)
            {
                printf("  Key: %lu, Value: %lu\n", b->slot[j].key, b->slot[j].value);
            }
        }
    }
    printf("Split stash buckets:\n");
    for (int i = 0; i < STASH_SPLIT_NUM; i++)
    {
        bucket *b = &(rg->split_buckets[i]);

        if (b->token > 0)
        {
            printf("Bucket %d:\n", i);
            for (int j = 0; j < b->token; j++)
            {
                printf("  Key: %lu, Value: %lu\n", b->slot[j].key, b->slot[j].value);
            }
        }
    }
}

// Modified insertion_sort to return the median
uint64_t insertion_sort_median(entry *arr, uint32_t n)
{
    for (uint32_t i = 1; i < n; i++)
    {
        entry current_entry = arr[i];
        int j = i - 1;

        while (j >= 0 && arr[j].key > current_entry.key)
        {
            arr[j + 1] = arr[j];
            j--;
        }

        arr[j + 1] = current_entry;
    }

    // Calculate median
    if (n % 2 == 0)
    {
        // If even number of elements, return the average of the two middle values
        uint32_t middle = n / 2;
        return (arr[middle - 1].key + arr[middle].key) / 2;
    }
    else
    {
        // If odd number of elements, return the middle value
        return arr[n / 2].key;
    }
}

bool range_groups_compact_three_to_two(range_group **rg1, range_group **rg2, range_group **rg3)
{

    // Combine entries from the three buckets into a single array
    int total_entries = (*rg1)->load + (*rg2)->load + (*rg3)->load;
    entry *entries = (entry *)malloc(total_entries * sizeof(entry));

    int entry_count = 0;

    // store normal buckets in buckets1
    for (int i = 0; i < HASH_ACCESS_NUM; i++)
    {
        bucket *b = &((*rg1)->buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // store normal stash buckets in buckets1
    for (int i = 0; i < STASH_NUM; i++)
    {
        bucket *b = &((*rg1)->stash_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // store split stash buckets  in buckets1
    for (int i = 0; i < STASH_SPLIT_NUM; i++)
    {
        bucket *b = &((*rg1)->split_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // ... (Repeat the same process for other buckets)
    // store normal buckets in buckets2
    for (int i = 0; i < HASH_ACCESS_NUM; i++)
    {
        bucket *b = &((*rg2)->buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }
    // store normal stash buckets in buckets2
    for (int i = 0; i < STASH_NUM; i++)
    {
        bucket *b = &((*rg2)->stash_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // store split stash buckets  in buckets1
    for (int i = 0; i < STASH_SPLIT_NUM; i++)
    {
        bucket *b = &((*rg2)->split_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // ... (Repeat the same process for other buckets)
    // store normal buckets in buckets3
    for (int i = 0; i < HASH_ACCESS_NUM; i++)
    {
        bucket *b = &((*rg3)->buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }
    // store normal stash buckets in buckets3
    for (int i = 0; i < STASH_NUM; i++)
    {
        bucket *b = &((*rg3)->stash_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // store split stash buckets  in buckets3
    for (int i = 0; i < STASH_SPLIT_NUM; i++)
    {
        bucket *b = &((*rg3)->split_buckets[i]);
        for (int j = 0; j < b->token; j++)
        {
            entries[entry_count].key = b->slot[j].key;
            entries[entry_count].value = b->slot[j].value;
            entry_count++;
        }
    }

    // Sort the entries based on keys
    uint64_t split_key = insertion_sort_median(entries, entry_count);
    if (entry_count != total_entries)
    {
        ftl_err("Err\n");
        exit(0);
    }
    // Initialize two new range buckets for the compacted entries
    range_group *new_bucket1 = range_group_init((*rg1)->min, split_key);
    range_group *new_bucket2 = range_group_init(split_key, (*rg3)->max);

    // Insert the sorted entries into the new range buckets
    for (int i = 0; i < entry_count; i++)
    {
        if (entries[i].key < split_key)
        {
            if (range_group_insert(new_bucket1, entries[i].key, entries[i].value) == 0)
            {
                if (range_group_insert_split_stash(new_bucket1, entries[i].key, entries[i].value) == 0)
                {
                    printf("Insert split stash failed in compact\n");
                    range_group_destory(new_bucket1);
                    range_group_destory(new_bucket2);
                    free(entries);
                    return false;
                }
            }
        }
        else
        {
            if (range_group_insert(new_bucket2, entries[i].key, entries[i].value) == 0)
            {
                if (range_group_insert_split_stash(new_bucket2, entries[i].key, entries[i].value) == 0)
                {
                    printf("Insert split stash failed in compact\n");
                    range_group_destory(new_bucket1);
                    range_group_destory(new_bucket2);
                    free(entries);
                    return false;
                }
            }
        }
    }

    // Free memory of old buckets and entries array
    free((*rg1)->buckets);
    free((*rg1)->stash_buckets);
    free((*rg1)->split_buckets);
    free(*rg1);
    free((*rg2)->buckets);
    free((*rg2)->stash_buckets);
    free((*rg2)->split_buckets);
    free(*rg2);
    free(entries);

    // Assign the new buckets to the input pointers
    *rg1 = new_bucket1;
    *rg2 = new_bucket2;
    ftl_assert((new_bucket1->load + new_bucket2->load) == total_entries);

    return true;
}
/*return (0 failed) (1,insert) (2,update) (3,delete)*/
uint8_t range_array_insert(range_array *range_array, uint64_t key, uint64_t value)
{
    ftl_assert(key < range_array->max);
    if (key == check_key)
        printf("RA insert:%lu %lu in %lu\n", key, value, range_array->min);
    uint64_t old_value = (range_array->arr)[key - range_array->min];
    if (old_value == UNMAPPED_PPA && value == UNMAPPED_PPA)
    {
        return 2;
    }
    (range_array->arr)[key - range_array->min] = value;

    if (old_value == UNMAPPED_PPA && value != UNMAPPED_PPA)
    {
        range_array->load++;
        return 1;
    }
    if (old_value != UNMAPPED_PPA && value == UNMAPPED_PPA)
    {
        range_array->load--;
        return 3;
    }
    if (old_value != UNMAPPED_PPA && value != UNMAPPED_PPA)
    {
        return 2;
    }
    return 0;
}

uint64_t range_array_search(range_array *range_array, uint64_t key)
{
    if (key == check_key)
    {
        printf("search check_key in RA %ld %ld\n", range_array->min, range_array->max);
    }
    uint64_t result = range_array->arr[key - range_array->min];
    return result;
}

uint64_t range_array_delete(range_array *range_array, uint64_t key)
{
    if (key == check_key)
        printf("RA delete:%lu  in RA %lu\n", key, range_array->min);
    uint64_t old_value = (range_array->arr)[key - range_array->min];
    (range_array->arr)[key - range_array->min] = UNMAPPED_PPA;
    if (old_value != UNMAPPED_PPA)
    {
        range_array->load--;
    }
    return old_value;
}

void range_array_destory(range_array *range_array)
{
    free(range_array->arr);
    free(range_array);
}