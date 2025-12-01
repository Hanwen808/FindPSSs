#ifndef SPREADSKETCH_H
#define SPREADSKETCH_H
#include <algorithm>
#include <cstdint>
#include <ctime>
#include <string.h>
#include <cmath>
#include <unordered_map>

#include "MurmurHash3.h"
#include "Sketch.h"


class MRB {
public:
    uint32_t m, k, T, _len;
    uint32_t *c;
    uint8_t *B;
    uint32_t seed;
    MRB(uint32_t m, uint32_t k, uint32_t T) {
        this->m = m;
        this->k = k;
        this->T = T;
        this->_len = m / k;
        c = new uint32_t[k];
        memset(c, 0, sizeof(uint32_t) * k);
        srand(time(NULL));
        seed = rand() % 10000;
        B = new uint8_t[m];
        memset(B, 0, sizeof(uint8_t) * m);
    }

    void update(uint32_t key, uint32_t ele) {
        uint32_t hash_index, hash_value;
        char hash_input_str[8];
        uint32_t leftmost = 0, bitmap_index = 0;
        memcpy(hash_input_str, &ele, 4);
        memcpy(hash_input_str + 4, &key, 4);
        MurmurHash3_x86_32(hash_input_str, 8, seed, &hash_value);
        hash_index = hash_value % _len; // offset
        while (hash_value) {
            leftmost ++;
            hash_value = hash_value >> 1;
        }
        leftmost = 32 - leftmost + 1;
        bitmap_index = std::min(leftmost, k) - 1;
        if (B[bitmap_index * _len + hash_index] == 0) {
            B[bitmap_index * _len + hash_index] = 1;
            c[bitmap_index] += 1;
        }
    }

    int estimate() {
        float est_spread = 0.0;
        int start_index = -1;
        for (int i = k - 1; i >= 0; i--) {
            if (c[i] >= T) {
                start_index = i;
                break;
            }
        }
        for (int i = start_index; i < k; ++i) {
            if (c[i] == _len) {
                est_spread += _len * log(_len);
            } else {
                est_spread += - _len * log(1 - ((1.0 * c[i]) / _len));
            }
        }
        est_spread = std::pow(2.0, start_index) * est_spread;
        return static_cast<int>(est_spread);
    }

    void clean() {
        memset(c, 0, sizeof(uint32_t) * k);
        memset(B, 0, sizeof(uint8_t) * m);
    }

    void merge(MRB mrb1) {
        for (int i = 0; i < m; ++i) {
            if (B[i] == 1) {
                if (mrb1.B[i] == 0) {
                    c[i / _len] -= 1;
                    B[i] = 0;
                }
            }
        }
    }

    void copyMrb(MRB mrb1) {
        memcpy(B, mrb1.B, m * sizeof(uint8_t));
        memcpy(c, mrb1.c, k * sizeof(uint32_t));
    }
};

class SpreadSketch : public Sketch {
public:
    uint32_t d, w, m, k, T, threshold_t;
    uint32_t **IDs;
    uint8_t **Ranks;
    MRB ***Mrbs;
    uint32_t *hash_seeds;
    uint32_t geo_hashseed;
    uint32_t cur_time;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> current_superspreaders;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> flow_time_map;

    SpreadSketch(uint32_t d, uint32_t w, uint32_t m, uint32_t k, uint32_t T, uint32_t threshold_t);
    ~SpreadSketch() {
        delete hash_seeds;
        for (int i = 0; i < d; ++i) {
            delete IDs[i];
            delete Ranks[i];
            delete[] Mrbs[i];
        }
    }
    void insert(uint32_t key, uint32_t ele, uint8_t _period);
    void clean();
    std::unordered_set<uint32_t> detect(uint32_t thr);
    uint32_t query(uint32_t key);
    void post_process();
};

#endif //SPREADSKETCH_H
