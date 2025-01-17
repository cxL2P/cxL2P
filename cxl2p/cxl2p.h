#include "RGHT.h"

typedef struct _cxl_l2p
{
    RGHT *rght;
    BMP_TYPE *bmp_array;
    seg_meta *seg_meta_arr;
    uint64_t pma0_seg_cnt;
} cxl2p_t;

cxl2p_t *cxl2p_init(int array_size);

/*
 *Not batch update
 */
void cxl2p_insert(cxl2p_t *rb_l2p_map, uint64_t key, uint64_t value, uint64_t old_value, uint64_t *cxl_lat);

/*
 * return value of the key, error code is recorded in the last paramter
 */
uint64_t cxl2p_get(cxl2p_t *rb_l2p_map, uint64_t key, uint64_t *cxl_lat);

/*
 *destory rb_l2p
 */
void cxl2p_destroy(cxl2p_t *rb_l2p_map);

// uint64_t rb_l2p_get_debug(cxl2p_t *rb_l2p_map, uint64_t key, uint64_t *cxl_lat);
