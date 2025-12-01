#include "../Headers/SpreadSketch.h"

SpreadSketch::SpreadSketch(uint32_t d, uint32_t w, uint32_t m, uint32_t k, uint32_t T, uint32_t threshold_t) {
    this->d = d;
    this->w = w;
    this->m = m;
    this->k = k;
    this->T = T;
    this->threshold_t = threshold_t;
    IDs = new uint32_t *[d];
    Ranks = new uint8_t *[d];
    Mrbs = new MRB **[d];
    for (int i = 0; i < d; ++i) {
        IDs[i] = new uint32_t[w];
        memset(IDs[i], 0, sizeof(uint32_t) * w);
        Ranks[i] = new uint8_t[w];
        memset(Ranks[i], 0, sizeof(uint8_t) * w);
        Mrbs[i] = new MRB *[w];
        for (int j = 0; j < w; ++j) {
            Mrbs[i][j] = new MRB(m, k, T);
        }
    }
    srand(time(NULL));
    hash_seeds = new uint32_t[d];
    std::unordered_set<uint32_t> mset;
    while (mset.size() != this->d) {
        mset.insert(rand() % 10000);
    }
    int cursor = 0;
    for (auto iter = mset.begin(); iter != mset.end(); iter ++, cursor ++) {
        hash_seeds[cursor] = *iter;
    }
    geo_hashseed = rand() % 10000;
    cur_time = 0;
}

void SpreadSketch::insert(uint32_t key, uint32_t ele, uint8_t _period) {
    uint32_t hash_index, hash_value, geohash_value;
    char hash_input_str[5];
    char key_ele_input_str[9];
    memcpy(key_ele_input_str, &key, 4);
    memcpy(key_ele_input_str + 4, &ele, 4);
    memcpy(hash_input_str, &key, 4);
    for (int i = 0; i < d; ++i) {
        MurmurHash3_x86_32(hash_input_str, 4, hash_seeds[i], &hash_value);
        hash_index = hash_value % w;
        Mrbs[i][hash_index]->update(key, ele);
        uint32_t leftmost = 0;
        MurmurHash3_x86_32(key_ele_input_str, 8, geo_hashseed, &geohash_value);
        while (geohash_value) {
            leftmost ++;
            geohash_value = geohash_value >> 1;
        }
        leftmost = 32 - leftmost + 1;
        if (leftmost >= Ranks[i][hash_index]) {
            IDs[i][hash_index] = key;
            Ranks[i][hash_index] = leftmost;
        }
    }
}

void SpreadSketch::clean() {
    std::unordered_set<uint32_t> S;
    for (int i = 0; i < d; i++) {
        for (int j = 0; j < w; ++j) {
            if (Mrbs[i][j]->estimate() >= threshold_t) {
                S.insert(IDs[i][j]);
            }
        }
    }
    for (auto iter = S.begin(); iter != S.end(); iter ++) {
        MRB* tmp_mrb = new MRB(m, k, T);
        uint32_t hash_index, hash_value;
        char hash_input_str[5];
        memcpy(hash_input_str, &(*iter), 4);
        for (int i = 0; i < d; ++i) {
            MurmurHash3_x86_32(hash_input_str, 4, hash_seeds[i], &hash_value);
            hash_index = hash_value % w;
            if (i == 0) {
                tmp_mrb->copyMrb(*Mrbs[i][hash_index]);
            } else {
                tmp_mrb->merge(*Mrbs[i][hash_index]);
            }
        }
        if (tmp_mrb->estimate() >= threshold_t) {
            current_superspreaders[cur_time].insert(*iter);
        }
        delete tmp_mrb;
    }
    cur_time ++;
    for (int i = 0; i < d; ++i) {
        memset(IDs[i], 0, sizeof(uint32_t) * w);
        memset(Ranks[i], 0, sizeof(uint8_t) * w);
        for (int j = 0; j < w; ++j) {
            Mrbs[i][j]->clean();
        }
    }
}

void SpreadSketch::post_process() {
    std::unordered_set<uint32_t> S;
    for (int i = 0; i < d; i++) {
        for (int j = 0; j < w; ++j) {
            if (Mrbs[i][j]->estimate() >= threshold_t) {
                S.insert(IDs[i][j]);
            }
        }
    }
    for (auto iter = S.begin(); iter != S.end(); iter ++) {
        MRB* tmp_mrb = new MRB(m, k, T);
        uint32_t hash_index, hash_value;
        char hash_input_str[5];
        memcpy(hash_input_str, &(*iter), 4);
        for (int i = 0; i < d; ++i) {
            MurmurHash3_x86_32(hash_input_str, 4, hash_seeds[i], &hash_value);
            hash_index = hash_value % w;
            if (i == 0) {
                tmp_mrb->copyMrb(*Mrbs[i][hash_index]);
            } else {
                tmp_mrb->merge(*Mrbs[i][hash_index]);
            }
        }
        if (tmp_mrb->estimate() >= threshold_t) {
            current_superspreaders[cur_time].insert(*iter);
        }
        delete tmp_mrb;
    }
}

std::unordered_set<uint32_t> SpreadSketch::detect(uint32_t thr) {
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

uint32_t SpreadSketch::query(uint32_t key) {
    if (flow_time_map.find(key) != flow_time_map.end()) {
        return flow_time_map[key].size();
    } else {
        return 0;
    }
}
