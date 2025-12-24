#include "../Headers/RGS.h"

#include <ctime>
#include <cmath>
#include <iostream>
#include <string.h>
#define MIN(a,b) ((a)<(b))?(a):(b)
const uint32_t R[32]={2725628290,1444492164,1001369998,2291192083,3632694421,3217476757,962572696,3523028760,1564808096,1686744101,2572712615,186525479,1267589045,2944996661,3807606838,2892595386,1879920448,3800713281,4044617030,1088214349,2912533710,4048616015,223249368,1094862426,1603814236,3377932768,121561825,2119485156,1642325476,1361943285,3238649846,992916862};

RGS::RGS(uint32_t d, uint32_t s, uint32_t m, uint32_t lam, uint32_t w, uint32_t t) :bucketArray(w,std::vector<std::pair<uint32_t,uint32_t>>(lam)) {
    this->d = d;
    this->s = s;
    this->m = m;
    this->lam = lam;
    this->w = w;
    cur_time = 0;
    this->threshold_t = t;
    //regArray=new uint8_t[int(ceil(d*m*5.0/8.0))]();
    regArray = new uint8_t *[d];
    for (int i = 0; i < d; ++i) {
        regArray[i] = new uint8_t[m];
        memset(regArray[i], 0, sizeof(uint8_t) * m);
    }
    IDs = new uint32_t *[lam];
    Cards = new float *[lam];
    for (int i = 0; i < lam; i ++) {
        IDs[i] = new uint32_t[w];
        memset(IDs[i], 0, sizeof(uint32_t) * w);
        Cards[i] = new float[w];
        memset(Cards[i], 0, sizeof(float) * w);
    }
    srand((uint32_t) time(NULL));
    hash_seed = rand() % 10000;
    hash_seed_bkt = rand() % 10000;
    current_packet_no = 0;
}

void RGS::insert(uint32_t key, uint32_t ele, uint8_t _period) {
    current_packet_no ++;
    char key_input_str[5];
    char ele_input_str[5];
    uint32_t hash_value1, hash_value2;
    memcpy(key_input_str, &key, 4);
    memcpy(ele_input_str, &ele, 4);
    MurmurHash3_x86_32(key_input_str, 4, hash_seed, &hash_value1);
    MurmurHash3_x86_32(ele_input_str, 4, hash_seed + 15417891, &hash_value2);
    uint32_t tempHash1 = hash_value1 + hash_value2;
    uint32_t virGroupIdx = tempHash1 % s;
    uint32_t regIdxInGroup = (tempHash1 / s) % m;
    tempHash1 = tempHash1 /s/m;
//     uint32_t rhoValue = MIN(maxRho,(int)__builtin_clz(tempHash1)+1);
    uint32_t rhoValue = 0;
    while (tempHash1) {
        rhoValue ++;
        tempHash1 = tempHash1 >> 1;
    }
    rhoValue = MIN(maxRho - rhoValue + 1,maxRho);
    uint32_t phyGroupIdx = (R[virGroupIdx] ^ hash_value1) % d;

    uint32_t oriRhoValue = regArray[phyGroupIdx][regIdxInGroup];

    if(rhoValue > oriRhoValue) {
        double sum = 0.0;
        for(int i = 0; i < m; i++){
            uint32_t regVal = regArray[phyGroupIdx][i];
            if(regVal < maxRho) {//when the reg value is maxRho, it cannot be updated again
                sum += pow(0.5, regVal);
            }
        }

        float updateValue= (1.0 * m) / sum;
        uint32_t bktIdx = hash_value1 % w;
        int empty_row = -1, empty_col = -1;
        int min_row = -1, min_col = -1;
        float min_value;
        for (int i = 0; i < lam; i ++) {
            if (IDs[i][bktIdx] == 0) {
                empty_row = i;
            } else {
                if (IDs[i][bktIdx] == key) {
                    Cards[i][bktIdx] += updateValue;
                    regArray[phyGroupIdx][regIdxInGroup] = rhoValue;
                    return;
                } else {
                    if (min_row == -1) {
                        min_row = i;
                        min_col = bktIdx;
                        min_value = Cards[i][bktIdx];
                    } else {
                        if (Cards[i][bktIdx] < min_value) {
                            min_row = i;
                            min_col = bktIdx;
                            min_value = Cards[i][bktIdx];
                        }
                    }
                }
            }
        }
        if (empty_row != -1) {
            IDs[empty_row][bktIdx] = key;
            Cards[empty_row][bktIdx] = updateValue;
            regArray[phyGroupIdx][regIdxInGroup] = rhoValue;
        } else {
            double rnd=(double)rand()/RAND_MAX;
            if (rnd <= 1.0 / (min_value + 1.0)) {
                IDs[min_row][min_col] = key;
                Cards[min_row][min_col] = min_value + 1.0;
                regArray[phyGroupIdx][regIdxInGroup] = rhoValue;
            }
        }
    }
}

void RGS::clean() {
    for (int i = 0; i < lam; i++) {
        for (int j = 0; j < w; j++) {
            if (Cards[i][j] >= this->threshold_t) {
                current_superspreaders[cur_time].insert(IDs[i][j]);
            }
        }
    }
    cur_time ++;
    // regArray=new uint8_t[int(ceil(d*m*5.0/8.0))]();
    //bucketArray.clear();
    for (int i = 0; i < d; ++i) {
        memset(regArray[i], 0, sizeof(uint8_t) * m);
    }
    for (int i = 0; i < lam; i ++) {
        memset(IDs[i], 0, sizeof(uint32_t) * w);
        memset(Cards[i], 0, sizeof(float) * w);
    }
}

std::unordered_set<uint32_t> RGS::detect(uint32_t thr) {
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

uint32_t RGS::query(uint32_t key) {
    if (flow_time_map.find(key) != flow_time_map.end()) {
        return flow_time_map[key].size();
    } else {
        return 0;
    }
}

void RGS::post_process() {
    for (int i = 0; i < lam; i++) {
        for (int j = 0; j < m; j++) {
            if (Cards[i][j] >= this->threshold_t) {
                current_superspreaders[cur_time].insert(IDs[i][j]);
            }
        }
    }
}
