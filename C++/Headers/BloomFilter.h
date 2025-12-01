#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H
#include <cstdint>
#include "./bitmap.h"
#include "MurmurHash3.h"

class BloomFilter {
public:
    int len;
    int k;
    int *seedArray;
    uint8_t *bitmap;

    BloomFilter(int len, int k, int* seedArray) {
        this->len = len;
        this->k = k;
        this->seedArray = seedArray;
        bitmap = allocbitmap(len);
        fillzero(bitmap, len);
    }

    void insertOneFlow(uint64_t flowLabel) {
        uint32_t hash_value;
        for (int i = 0; i < this->k; ++i) {
            MurmurHash3_x86_32(&flowLabel, sizeof(uint64_t), this->seedArray[i], &hash_value);
            unsigned int pos = hash_value % this->len;
            setbit(pos, bitmap);
        }
    }

    bool isFlowInBF(uint64_t flowLabel) {
        uint32_t hash_value;
        for (int i = 0; i < this->k; ++i) {
            MurmurHash3_x86_32(&flowLabel, sizeof(uint64_t), this->seedArray[i], &hash_value);
            int pos = hash_value % this->len;
            if (getbit(pos, bitmap) == 0) {
                return false;
            }
        }
        return true;
    }
};
#endif //BLOOMFILTER_H
