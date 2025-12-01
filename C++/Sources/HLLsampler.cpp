#include "../Headers/HLLsampler.h"

#include <ctime>

HLLsampler::HLLsampler(uint32_t d, uint32_t w, uint32_t n, uint32_t T) {
    this->d = d;
    this->w = w;
    this->n = n;
    this->T = T;
    cur_time = 0;
    IDs = new uint32_t *[d];
    row_seeds = new uint32_t[d];
    Cnts = new int *[d];
    HLLs = new HLL **[d];
    std::unordered_set<uint32_t> mset;
    srand(time(NULL));
    while (mset.size() != d) {
        mset.insert(rand() % 10000);
    }
    int cursor = 0;
    for (auto iter = mset.begin(); iter != mset.end(); iter ++, cursor ++) {
        row_seeds[cursor] = *iter;
    }
    for (int i = 0; i < d; ++i) {
        IDs[i] = new uint32_t[w];
        memset(IDs[i], 0, sizeof(uint32_t) * w);
        Cnts[i] = new int[w];
        memset(Cnts[i], 0, sizeof(int) * w);
        HLLs[i] = new HLL *[w];
        for (int j = 0; j < w; ++j) {
            HLLs[i][j] = new HLL(this->n, row_seeds[i]);
        }
    }
}

void HLLsampler::insert(uint32_t key, uint32_t ele, uint8_t _period) {
    uint32_t hash_index, hash_value;
    char hash_input_str[5];
    memcpy(hash_input_str, &key, 4);
    float p, inc;
    uint32_t inc_int;
    for (int i = 0; i < d; ++i) {
        MurmurHash3_x86_32(hash_input_str, 4, row_seeds[i], &hash_value);
        hash_index = hash_value % w;
        p = HLLs[i][hash_index]->update(key, ele);
        inc = 1.0 / p;
        inc_int = static_cast<uint32_t>(ceilf(inc));
        if (p != -1) {
            if (IDs[i][hash_index] == 0) {
                if (rand() % 1000 <= 1000 * (inc / (1.0 * inc_int))) {
                    IDs[i][hash_index] = key;
                    Cnts[i][hash_index] = inc_int;
                }
            } else {
                if (IDs[i][hash_index] == key) {
                    if (rand() % 1000 <= 1000 * (inc / (1.0 * inc_int))) {
                        IDs[i][hash_index] = key;
                        Cnts[i][hash_index] += inc_int;
                    }
                } else {
                    float pd = std::pow(1.08, -1.0 * Cnts[i][hash_index]);
                    if (rand() % 1000 <= 1000 * pd) {
                        if (rand() % 1000 <= 1000 * (inc / (1.0 * inc_int))) {
                            Cnts[i][hash_index] -= static_cast<int>(inc_int);
                            if (Cnts[i][hash_index] <= 0) {
                                IDs[i][hash_index] = key;
                                Cnts[i][hash_index] = -1 * Cnts[i][hash_index];
                            }
                        }
                    }
                }
            }
        }
    }
}

void HLLsampler::clean() {
    std::unordered_set<uint32_t> S;
    for (int i = 0; i < d; ++i) {
        for (int j = 0; j < w; ++j) {
            S.insert(IDs[i][j]);
        }
    }
    for (auto iter = S.begin(); iter != S.end(); iter ++) {
        uint32_t max_value = 0;
        uint32_t hash_value, hash_index;
        char hash_input_str[5];
        memcpy(hash_input_str, &(*iter), 4);
        for (int i = 0; i < d; ++i) {
            MurmurHash3_x86_32(hash_input_str, 4, row_seeds[i], &hash_value);
            hash_index = hash_value % w;
            if (IDs[i][hash_index] == *iter) {
                if (Cnts[i][hash_index] > max_value) {
                    max_value = Cnts[i][hash_index];
                }
            }
        }
        if (max_value >= this->T) {
            current_superspreaders[cur_time].insert(*iter);
        }
    }
    cur_time ++;
    for (int i = 0; i < d; ++i) {
        memset(IDs[i], 0, sizeof(uint32_t) * w);
        memset(Cnts[i], 0, sizeof(uint32_t) * w);
        for (int j = 0; j < w; ++j) {
            HLLs[i][j]->clean();
        }
    }
}

void HLLsampler::post_process() {
    std::unordered_set<uint32_t> S;
    for (int i = 0; i < d; ++i) {
        for (int j = 0; j < w; ++j) {
            S.insert(IDs[i][j]);
        }
    }
    for (auto iter = S.begin(); iter != S.end(); iter ++) {
        uint32_t max_value = 0;
        uint32_t hash_value, hash_index;
        char hash_input_str[5];
        memcpy(hash_input_str, &(*iter), 4);
        for (int i = 0; i < d; ++i) {
            MurmurHash3_x86_32(hash_input_str, 4, row_seeds[i], &hash_value);
            hash_index = hash_value % w;
            if (IDs[i][hash_index] == *iter) {
                if (Cnts[i][hash_index] > max_value) {
                    max_value = Cnts[i][hash_index];
                }
            }
        }
        if (max_value >= this->T) {
            current_superspreaders[cur_time].insert(*iter);
        }
    }
}

std::unordered_set<uint32_t> HLLsampler::detect(uint32_t thr) {
    std::unordered_set<uint32_t> pred_ps_set;
    for (auto iter = current_superspreaders.begin(); iter != current_superspreaders.end(); iter ++) {
        for (auto iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2 ++) {
            flow_time_map[*iter2].insert(iter->first);
        }
    }
    for (auto iter = flow_time_map.begin(); iter != flow_time_map.end(); iter ++) {
        if (iter->second.size() >= thr) {
            pred_ps_set.insert(iter->first);
        }
    }
    return pred_ps_set;
}

uint32_t HLLsampler::query(uint32_t key) {
    if (flow_time_map.find(key) != flow_time_map.end()) {
        return flow_time_map[key].size();
    } else {
        return 0;
    }
}
