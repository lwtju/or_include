//
// Created by liuwang-s on 2017/7/24.
//

#ifndef SF_CUCKOO_FILTER_H
#define SF_CUCKOO_FILTER_H

#include <elf.h>

#define BUCKET_SIZE 4

#define MAX_FIND_TIME 32

typedef struct cuckoo_bucket{
    uint16_t entry[BUCKET_SIZE];
} cuckoo_bucket;

typedef struct cuckoo_filter{
    unsigned count;
    unsigned b_num;
    unsigned h_offset;
    cuckoo_bucket bucket[];
} cuckoo_filter;

#endif //SF_CUCKOO_FILTER_H
