#include "../Headers/FreeRS.h"

#include <cmath>
#include <ctime>

#include "../Headers/MurmurHash3.h"
#include <string.h>

FreeRS::FreeRS(uint32_t _sslen, uint32_t _m, uint32_t  _T) {
    this->ssLen = _sslen;
    this->m = _m;
    this->T = _T;
    R = new uint8_t[m];
    memset(R, 0, sizeof(uint8_t) * m);
    SS = new StreamSummary(_sslen);
    p = 1.0;
    std::unordered_set<uint32_t> mset;
    srand(time(NULL));
    while (mset.size() != 2) {
        mset.insert(rand());
    }
    hash_seed = *mset.begin();
    hash_seed2 = *(mset.begin()++);
    cur_time = 0;
}

void FreeRS::insert(uint32_t key, uint32_t ele, uint8_t _period) {
    uint32_t hash_value, hash_index;
    char hash_input_str[9];
    memcpy(hash_input_str, &key, 4);
    memcpy(hash_input_str + 4, &ele, 4);
    MurmurHash3_x86_32(hash_input_str, 8, hash_seed, &hash_value);
    hash_index = hash_value % m;
    MurmurHash3_x86_32(hash_input_str, 8, hash_seed2, &hash_value);
    uint32_t leftmost = 0;
    while (hash_value) {
        leftmost ++;
        hash_value >>= 1;
    }
    leftmost = 32 - leftmost + 1;
    float inc = 0.0;
    if (leftmost > R[hash_index]) {
        inc = 1.0 / p;
        p = p - pow(0.5, R[hash_index]) / (1.0 * m) + std::pow(0.5, leftmost) / (1.0 * m); //1.0 / ((1<<static_cast<uint32_t>(R[hash_index])) * m) + 1.0 / ((1<<leftmost) * m);
        R[hash_index] = leftmost;
    }
    if (inc != 0) {
        SSKeyNode* keyNode=SS->findKey(key);
        if(keyNode!= nullptr){
            float newVal = inc + keyNode->parent->counter;
            SS->SSUpdate(keyNode,newVal);
        } else {
            if(SS->getSize() < ssLen){
                SS->SSPush(key, inc, 0);
            }else{
                double temp=(double)rand()/RAND_MAX;
                float minVal=SS->getMinVal();
                float newVal=minVal + inc;

                if(temp<(double)inc / newVal){
                    SS->SSPush(key,newVal,minVal);
                }
                else{
                    SSKeyNode* minKeyNode=SS->getMinKeyNode();
                    SS->SSUpdate(minKeyNode,newVal);
                }
            }
        }
    }
}

void FreeRS::clean() {
    std::unordered_map<uint32_t, float> estFlowSpreads;
    SS->getKeyVals(estFlowSpreads);
    for (auto iter = estFlowSpreads.begin(); iter != estFlowSpreads.end(); iter ++) {
        if (iter->second >= T) {
            current_superspreaders[cur_time].insert(iter->first);
        }
    }
    cur_time ++;
    memset(R, 0, sizeof(uint8_t) * m);
    p = 1.0;
    delete SS;
    SS = new StreamSummary(ssLen);
}

void FreeRS::post_process() {
    std::unordered_map<uint32_t, float> estFlowSpreads;
    SS->getKeyVals(estFlowSpreads);
    for (auto iter = estFlowSpreads.begin(); iter != estFlowSpreads.end(); iter ++) {
        if (iter->second >= T) {
            current_superspreaders[cur_time].insert(iter->first);
        }
    }
}

std::unordered_set<uint32_t> FreeRS::detect(uint32_t thr) {
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

uint32_t FreeRS::query(uint32_t key) {
    if (flow_time_map.find(key) != flow_time_map.end()) {
        return flow_time_map[key].size();
    } else {
        return 0;
    }
}
