//
// Created by liuwang-s on 2017/7/24.
//

#include <math.h>
#include "cuckoo_filter.h"
#include "malloc.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static uint64_t MurmurHash64A(const void *key, int len, unsigned int seed) {
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;
    uint64_t h = seed ^(len * m);
    const uint8_t *data = (const uint8_t *) key;
    const uint8_t *end = data + (len - (len & 7));

    while (data != end) {
        uint64_t k;

#if (BYTE_ORDER == LITTLE_ENDIAN)
        k = *((uint64_t *) data);
#else
        k = (uint64_t)data[0];
    k |= (uint64_t)data[1] << 8;
    k |= (uint64_t)data[2] << 16;
    k |= (uint64_t)data[3] << 24;
    k |= (uint64_t)data[4] << 32;
    k |= (uint64_t)data[5] << 40;
    k |= (uint64_t)data[6] << 48;
    k |= (uint64_t)data[7] << 56;
#endif

        k *= m;
        k ^= k >> r;
        k *= m;
        h ^= k;
        h *= m;
        data += 8;
    }

    switch (len & 7) {
        case 7:
            h ^= (uint64_t) data[6] << 48;
        case 6:
            h ^= (uint64_t) data[5] << 40;
        case 5:
            h ^= (uint64_t) data[4] << 32;
        case 4:
            h ^= (uint64_t) data[3] << 24;
        case 3:
            h ^= (uint64_t) data[2] << 16;
        case 2:
            h ^= (uint64_t) data[1] << 8;
        case 1:
            h ^= (uint64_t) data[0];
            h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    return h;
}

uint16_t cf_finger_print(uint64_t h) {
    h = h >> 48;
    return h ? h : 1;
}

int count_z(unsigned x) {
    int n;

    if (x == 0) return 32;
    n = 1;
    if ((x >> 16) == 0) {
        n = n + 16;
        x = x << 16;
    }
    if ((x >> 24) == 0) {
        n = n + 8;
        x = x << 8;
    }
    if ((x >> 28) == 0) {
        n = n + 4;
        x = x << 4;
    }
    if ((x >> 30) == 0) {
        n = n + 2;
        x = x << 2;
    }
    n = n - (x >> 31);
    return n;
}


cuckoo_filter *cf_new(int num) {
    unsigned bucket_num = (unsigned) ceil(num / BUCKET_SIZE);
    cuckoo_filter *cf = malloc(sizeof(cuckoo_bucket) * bucket_num + sizeof(cuckoo_filter));
    cf->count = 0;
    cf->b_num = bucket_num;
    cf->h_offset = (unsigned) count_z(bucket_num) + 32 + 1;
    memset(cf->bucket, 0, sizeof(cuckoo_bucket) * bucket_num);

    return cf;
}

int cf_bucket_check_in(cuckoo_filter *cf, int index, uint16_t fp) {
    for (int i = 0; i < BUCKET_SIZE; ++i) {
        if (cf->bucket[index].entry[i] == fp) {
            return 1;
        }
    }

    return 0;
}

int cf_bucket_insert(cuckoo_filter *cf, int index, uint16_t fp) {
    for (int i = 0; i < BUCKET_SIZE; ++i) {
        if (cf->bucket[index].entry[i] == 0) {
            cf->bucket[index].entry[i] = fp;
            return 1;
        }
    }

    return 0;
}

int cf_add(cuckoo_filter *cf, const char *str, int len) {
    uint64_t h = MurmurHash64A(str, len, 0xadc83b19ULL);
    uint16_t fp = cf_finger_print(h);
    unsigned i1 = (unsigned) (h % cf->b_num);
    unsigned i2 = (i1 ^ (unsigned) (MurmurHash64A(&fp, sizeof(uint16_t), 0xadc83b19ULL) >> (cf->h_offset))) % cf->b_num;
    if (cf_bucket_check_in(cf, i1, fp)) {
        return 0;
    }
    if (cf_bucket_check_in(cf, i2, fp)) {
        return 0;
    }

    if (cf_bucket_insert(cf, i1, fp)) {
        return 1;
    }
    if (cf_bucket_insert(cf, i2, fp)) {
        return 1;
    }

    //随机选择一个index
    unsigned i0;
    if (rand() % 2) {
        i0 = i1;
    } else {
        i0 = i2;
    }

    for (int i = 0; i < MAX_FIND_TIME; ++i) {
        int entry = rand() % BUCKET_SIZE;
        uint16_t tmp = cf->bucket[i0].entry[entry];
        cf->bucket[i0].entry[entry] = fp;
        fp = tmp;

        i0 = (i0 ^ (unsigned) (MurmurHash64A(&fp, sizeof(uint16_t), 0xadc83b19ULL) >> (cf->h_offset))) % cf->b_num;
        if (cf_bucket_check_in(cf, i0, fp)) {
            return 0;
        }
        if (cf_bucket_insert(cf, i0, fp)) {
            return 1;
        }
    }

    return -1;
}

int cf_check_in(cuckoo_filter *cf, const char *str, int len) {
    if (cf == NULL) {
        return -1;
    }
    uint64_t h = MurmurHash64A(str, len, 0xadc83b19ULL);
    uint16_t fp = cf_finger_print(h);
    unsigned i1 = (unsigned) (h % cf->b_num);
    if (cf_bucket_check_in(cf, i1, fp)) {
        return 1;
    }
    unsigned i2 = (i1 ^ (unsigned) (MurmurHash64A(&fp, sizeof(uint16_t), 0xadc83b19ULL) >> (cf->h_offset))) % cf->b_num;
    if (cf_bucket_check_in(cf, i2, fp)) {
        return 1;
    }

    return 0;
}

cuckoo_filter *get_storage_cf(const char *file_name) {
    int fd;
    cuckoo_filter *cf;
    struct stat sb;

    fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }
    fstat(fd, &sb);
    cf = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);

    if (cf == MAP_FAILED) {
        return NULL;
    }

    return cf;
}

int free_cf(cuckoo_filter *cf, const char *file_name) {
    int fd;
    struct stat sb;

    fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        return -1;
    }
    fstat(fd, &sb);
    close(fd);

    return munmap((void *) cf, sb.st_size);
}

int resize_line(int num) {
    if (num < 100000) {
        return 1000000;
    }
    if (num < 1000000) {
        return 2000000;
    }

    return ((num / 1000000) + 1) * 2000000;
}

void main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("please specify input file path\n");
        return;
    }

    if (argc < 3) {
        printf("please specify output file path\n");
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("open input file fail!");
        return;
    }

    int line_num = 0;
    int ch = 0;
    while (!feof(fp)) {
        ch = fgetc(fp);
        if (ch == '\n') {
            line_num++;
        }
    }

    int num = resize_line(line_num);
    cuckoo_filter *cf = cf_new(num);

    int c_0 = 0;
    int c_1 = 0;
    for (int i = 0; i < 3; ++i) {
        fseek(fp, 0, SEEK_SET);
        while (1) {
            char buf[1000];
            char *retC = fgets(buf, 1000, fp);
            if (retC == NULL) {
                break;
            }
            memset(buf + 32, 0, 1000 - 32);
            int ret = cf_add(cf, buf, 32);
            if (ret == 0) {
                c_0++;
            } else {
                c_1++;
            }

        }
    }
    printf("success add line number: %d\n", c_1);
    fclose(fp);

    FILE *fw = fopen(argv[2], "w");

    size_t cf_size = sizeof(cuckoo_bucket) * (unsigned) ceil(num / BUCKET_SIZE) + sizeof(cuckoo_filter);
    fwrite(cf, cf_size, 1, fw);

    free(cf);
    fclose(fw);
}
