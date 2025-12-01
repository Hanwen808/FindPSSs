#ifndef HLLSAMPLER_H
#define HLLSAMPLER_H
#include <string.h>
#include <unordered_map>
#include <valarray>
#include <unordered_set>
#include "Sketch.h"
#define sampleseed 11231
class HLL {
public:
    float p;
    uint8_t *R;
    uint32_t n;
    uint32_t seed;

    HLL(uint32_t n, uint32_t seed) {
        this->n = n;
        this->seed = seed;
        this->p = 1.0;
        R = new uint8_t[n];
        memset(R, 0, sizeof(uint8_t) * n);
    }

    float update(uint32_t key, uint32_t ele) {
        char hash_input_str[9];
        uint32_t hash_value, hash_index;
        uint32_t geohash_value, leftmost = 0;
        float q;
        memcpy(hash_input_str, &key, 4);
        memcpy(hash_input_str + 4, &ele, 4);
        MurmurHash3_x86_32(hash_input_str, 8, seed, &hash_value);
        hash_index = hash_value % this->n;
        MurmurHash3_x86_32(hash_input_str, 8, sampleseed, &geohash_value);
        while (geohash_value) {
            leftmost ++;
            geohash_value = geohash_value >> 1;
        }
        leftmost = 32 - leftmost + 1;
        if (leftmost > R[hash_index]) {
            q = p;
            p += - (1.0 / n) * (std::pow(0.5, R[hash_index] * 1.0)) + (1.0 / n) * (std::pow(0.5, leftmost));
            R[hash_index] = leftmost;
            return q;
        }
        return -1;
    }

    void clean() {
        p = 1.0;
        memset(R, 0, sizeof(uint8_t) * n);
    }
};

class HLLsampler : public Sketch {
public:
    uint32_t d, w, n;
    uint32_t T;
    uint32_t **IDs;
    int **Cnts;
    HLL ***HLLs;
    uint32_t *row_seeds;
    uint32_t cur_time;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> current_superspreaders;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> flow_time_map;
    HLLsampler(uint32_t d, uint32_t w, uint32_t n, uint32_t T);
    ~HLLsampler() {
        delete row_seeds;
        for (int i = 0; i < d; ++i) {
            delete[] IDs[i];
            delete[] Cnts[i];
        }
        for (int i = 0; i < d; ++i) {
            for (int j = 0; j < w; ++j) {
                delete HLLs[i][j];
            }
        }
    }
    void insert(uint32_t key, uint32_t ele, uint8_t _period);
    std::unordered_set<uint32_t> detect(uint32_t thr);
    void clean();
    uint32_t query(uint32_t key);
    void post_process();
};

#endif //HLLSAMPLER_H
