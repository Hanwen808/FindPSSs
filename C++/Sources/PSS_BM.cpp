#include "../Headers/PSS_BM.h"

#include <ctime>
#include <string.h>

PSSBM::PSSBM(uint32_t m, uint32_t d, uint32_t w, uint32_t t) {
    this->m = m;
    this->d = d;
    this->w = w;
    this->c = m;
    this->threshold_t = t;
    B = allocbitmap(m);
    this->IDs = new uint32_t *[d];
    this->Cards = new float *[d];
    this->Times = new uint8_t *[d];
    this->Weights = new uint8_t *[d];
    for (int i = 0; i < d; ++i) {
        IDs[i] = new uint32_t[w];
        memset(IDs[i], 0, sizeof(uint32_t) * w);
        Cards[i] = new float[w];
        memset(Cards[i], 0, sizeof(float) * w);
        Times[i] = new uint8_t[w];
        memset(Times[i], 0, sizeof(uint8_t) * w);
        Weights[i] = new uint8_t[w];
        memset(Weights[i], 0, sizeof(uint8_t) * w);
    }
    srand((uint32_t) time(NULL));
    hash_seed = rand() % 10000;
    row_seeds = new uint32_t[d];
    std::unordered_set<uint32_t> mset;
    while (mset.size() != this->d) {
        mset.insert(rand() % 10000);
    }
    int cursor = 0;
    for (auto iter = mset.begin(); iter != mset.end(); ++iter) {
        row_seeds[cursor ++] = *iter;
    }
    
    current_packet_no = 0;
}

void PSSBM::insert(uint32_t key, uint32_t ele, uint8_t _period) {
    current_packet_no ++;
    uint32_t hash_index, hash_value;
    char hash_input_xor[9];
    char flow_input_str[5];
    memcpy(hash_input_xor, &key, 4);
    memcpy(hash_input_xor + 4, &ele, 4);
    memcpy(flow_input_str, &key, 4);
    MurmurHash3_x86_32(hash_input_xor, 8, hash_seed, &hash_value);
    hash_index = hash_value % m;
    float inc = 0.0;
    if (getbit(hash_index, B) == 0) {
        inc = (1.0 * m) / (1.0 * c);
        c -= 1;
        setbit(hash_index, B);
    }
    if (inc != 0.0) {
        int empty_row = -1, empty_col = -1;
        int min_row = -1, min_col = -1;
        float min_value;
        for (int i = 0; i < d; ++i) {
            MurmurHash3_x86_32(flow_input_str, 4, row_seeds[i], &hash_value);
            hash_index = hash_value % w;
            if (IDs[i][hash_index] == 0) {
                empty_row = i;
                empty_col = hash_index;
            } else {
                if (IDs[i][hash_index] == key) {
                    if (Times[i][hash_index] == _period) {
                        Cards[i][hash_index] += inc;
                    } else {
                        if (Cards[i][hash_index] >= this->threshold_t) {
                            this->Weights[i][hash_index] += 1;
                        }
                        Cards[i][hash_index] = inc;
                        Times[i][hash_index] = _period;
                    }
                    return;
                } else {
                    if (min_row == -1) {
                        min_row = i;
                        min_col = hash_index;
                        min_value = Cards[i][hash_index];
                    } else {
                        if (Cards[i][hash_index] < min_value) {
                            min_row = i;
                            min_col = hash_index;
                            min_value = Cards[i][hash_index];
                        }
                    }
                }
            }
        }
        if (empty_row != -1) {
            IDs[empty_row][empty_col] = key;
            Cards[empty_row][empty_col] = inc;
            Times[empty_row][empty_col] = _period;
            Weights[empty_row][empty_col] = 0;
        } else {
            double Pr = 0.0;
//             if (Weights[min_row][min_col] == 0) {
//                 Pr = (1.0 * inc) / (1.0 * inc + Cards[min_row][min_col]);
//             } else {
//                 int xxx = static_cast<int>(1.0 * _period - 1.0 * Times[min_row][min_col] - 1.0 * Weights[min_row][min_col]);
//                 if (xxx >= 0) {
//                     Pr = std::min((1<<xxx) * ((1.0 * inc) / (1.0 * inc + Cards[min_row][min_col])), 1.0);
//                 } else {
//                   Pr =   ((1.0 * inc) / (1.0 * inc + Cards[min_row][min_col])) / (1.0*(1<<(-xxx)));
//                 }
//             }
            int delta_t = static_cast<int>(_period - Times[min_row][min_col]);
            if (delta_t == 0 && Weights[min_row][min_col] != 0)
                return;
            float reinforce_inc = 1.0 * inc * (1<<delta_t);
            int delta_w = static_cast<int>(Weights[min_row][min_col]);
            float reinforce_x = 1.0 * Cards[min_row][min_col] * (1<<delta_w);
            Pr = reinforce_inc / (reinforce_inc + reinforce_x);
            //double rnd = 1.0 * (rand() % 10000) / 10000.0;
            double rnd=(double)rand()/RAND_MAX;
//             uint32_t hash_rand;
//             char rand_input_str[5];
//             memcpy(rand_input_str, &current_packet_no, 4);
//             MurmurHash3_x86_32(rand_input_str, 4, 33121, &hash_rand);
//             hash_rand = hash_rand % 10000;
            if (rnd <= Pr) {
                IDs[min_row][min_col] = key;
                Cards[min_row][min_col] = inc;
                Times[min_row][min_col] = _period;
                Weights[min_row][min_col] = 0;
            }
        }
    }
}

void PSSBM::clean() {
    //fillzero(B, m);
    B = allocbitmap(m);
    c = m;
}

void PSSBM::post_process() {
    return;
}

std::unordered_set<uint32_t> PSSBM::detect(uint32_t thr) {
    std::unordered_set<uint32_t> pred_ps_set;
    for (int i = 0; i < d; ++i) {
        for (int j = 0; j < w; ++j) {
            uint32_t is_cur = 0;
            if (Cards[i][j] >= threshold_t) {
                is_cur = 1;
            }
            if (is_cur + Weights[i][j] >= thr) {
                pred_ps_set.insert(IDs[i][j]);
            }
        }
    }
    return pred_ps_set;
}

uint32_t PSSBM::query(uint32_t key) {
    uint32_t hash_index, hash_value;
    char hash_input_str[5];
    memcpy(hash_input_str, &key, 4);
    for (int i = 0; i < d; ++i) {
        MurmurHash3_x86_32(hash_input_str, 4, row_seeds[i], &hash_value);
        hash_index = hash_value % w;
        if (IDs[i][hash_index] == key) {
            uint32_t is_cur = 0;
            if (Cards[i][hash_index] >= threshold_t) {
                is_cur = 1;
            }
            return is_cur + Weights[i][hash_index];
        }
    }
    return 0;
}
