#include "../Headers/KTSketch.h"

#include <ctime>
#include <string.h>

KTSketch::KTSketch(uint32_t M1, uint32_t d1, uint32_t M2, uint32_t N2, uint32_t cellnum2, uint32_t M3, uint32_t N3, uint32_t cellnum3, uint32_t threshold_t, float lam) {
    this->M1 = M1;
    this->d1 = d1;
    this->M2 = M2;
    this->N2 = N2;
    this->c2 = cellnum2;
    this->M3 = M3;
    this->N3 = N3;
    this->c3 = cellnum3;
    this->Z2 = M2;
    this->Z3 = M3;
    this->threshold_t = threshold_t;
    this->lam = lam;
    srand(time(NULL));
    // init part 1
    BloomFilter = new uint8_t[M1];
    memset(BloomFilter, 0, sizeof(uint8_t) * M1);
    filter_seeds = new uint32_t[d1];
    std::unordered_set<uint32_t> mset;
    while (mset.size() != d1) {
        mset.insert(rand() % 10000);
    }
    int cursor = 0;
    for (auto iter = mset.begin(); iter != mset.end(); iter ++) {
        filter_seeds[cursor ++] = *iter;
    }
    // init part 2
    Bitmap2 = new uint8_t[M2];
    memset(Bitmap2, 0, sizeof(uint8_t) * M2);
    H2 = new Bucket2 *[N2];
    for (int i = 0; i < N2; ++i) {
        H2[i] = new Bucket2(c2);
    }
    bitmap_hash_seed = rand() % 10000;
    summary_hash_seed = rand() % 10000;
    // init part 3
    Bitmap3 = new uint8_t[M3];
    memset(Bitmap3, 0, sizeof(uint8_t) * M3);
    H3 = new Bucket3 *[N3];
    for (int i = 0; i < N3; ++i) {
        H3[i] = new Bucket3(c3);
    }
}

void KTSketch::insert2(uint32_t key, uint32_t ele) {
    uint32_t hash_index, hash_value;
    char hash_input_str[9];
    memcpy(hash_input_str, &key, 4);
    memcpy(hash_input_str + 4, &ele, 4);
    MurmurHash3_x86_32(hash_input_str, 8, bitmap_hash_seed, &hash_value);
    hash_index = hash_value % M2;
    float p = -1;
    if (Bitmap2[hash_index] == 0) {
        p = (1.0 * Z2) / (1.0 * M2);
        Bitmap2[hash_index] = 1;
        Z2 -= 1;
    }
    if (p != -1) {
        char flow_input_str[5];
        memcpy(flow_input_str, &key, 4);
        MurmurHash3_x86_32(flow_input_str, 4, summary_hash_seed, &hash_value);
        hash_index = hash_value % N2;
        H2[hash_index]->update(key, 1.0 / p);
    }
}

void KTSketch::insert3(uint32_t key, uint32_t ele) {
    uint32_t hash_index, hash_value;
    char hash_input_str[9];
    memcpy(hash_input_str, &key, 4);
    memcpy(hash_input_str + 4, &ele, 4);
    MurmurHash3_x86_32(hash_input_str, 8, bitmap_hash_seed, &hash_value);
    hash_index = hash_value % M3;
    float p = -1;
    if (Bitmap3[hash_index] == 0) {
        p = (1.0 * Z3) / (1.0 * M3);
        Bitmap3[hash_index] = 1;
        Z3 -= 1;
    }
    if (p != -1) {
        char flow_input_str[5];
        memcpy(flow_input_str, &key, 4);
        MurmurHash3_x86_32(flow_input_str, 4, summary_hash_seed, &hash_value);
        hash_index = hash_value % N3;
        if (H3[hash_index]->update(key, 1.0 / p) == -1) {
            this->insert2(key, ele);
        }
    }
}

void KTSketch::insert(uint32_t key, uint32_t ele, uint8_t _period) {
    uint32_t hash_index, hash_value;
    char hash_input_str[5];
    memcpy(hash_input_str, &key, 4);
    int is_potential = 1;
    for (int i = 0; i < d1; ++i) {
        MurmurHash3_x86_32(hash_input_str, 4, filter_seeds[i], &hash_value);
        hash_index = hash_value % M1;
        if (BloomFilter[hash_index] == 0) {
            is_potential = 0;
            break;
        }
    }
    if (is_potential) {
        this->insert3(key, ele);
    } else {
        this->insert2(key, ele);
    }
}

void KTSketch::clean() {
    for (int i = 0; i < N3; ++i) {
        for (int j = 0; j < c3; ++j) {
            if (H3[i]->cells[j].Card >= threshold_t) {
                H3[i]->cells[j].n1 += 1;
                H3[i]->cells[j].n2 = 0;
                H3[i]->cells[j].weight = H3[i]->cells[j].n1 - H3[i]->cells[j].n2;
            } else {
                H3[i]->cells[j].n2 += 1;
                H3[i]->cells[j].weight = H3[i]->cells[j].n1 - H3[i]->cells[j].n2;
            }
            H3[i]->cells[j].Card = 0;
        }
    }
    memset(Bitmap3, 0, sizeof(uint8_t) * M3);
    Z3 = M3;
    for (int i = 0; i < N2; ++i) {
        for (int j = 0; j < c2; ++j) {
            if (std::abs(H2[i]->cells[j].Card - H2[i]->cells[j].Error) >= lam * threshold_t) {
                uint32_t hash_index, hash_value;
                char hash_input_str[5];
                memcpy(hash_input_str, &(H2[i]->cells[j].ID), 4);
                // insert to bloom filter
                for (int k = 0; k < d1; ++k) {
                    MurmurHash3_x86_32(hash_input_str, 4, filter_seeds[k], &hash_value);
                    hash_index = hash_value % M1;
                    BloomFilter[hash_index] = 1;
                }
                MurmurHash3_x86_32(hash_input_str, 4, summary_hash_seed, &hash_value);
                hash_index = hash_value % N3;
                H3[hash_index]->updateFromStage2(H2[i]->cells[j].ID);
                 //H3[hash_index]->msort();
                 // int have_empty = -1;
                 // int min_r = -1; // new
                 // int min_weight = -1; // new
                 // for (int r = 0; r < c3; ++r) {
                 //     if (H3[hash_index]->cells[r].ID == 0) {
                 //         H3[hash_index]->cells[r].ID = H2[i]->cells[j].ID;
                 //         H3[hash_index]->cells[r].Card = 0;
                 //         H3[hash_index]->cells[r].n1 = 1;
                 //         H3[hash_index]->cells[r].n2 = 0;
                 //         have_empty = r;
                 //     }
                 //     else { // new
                 //         if (min_r == -1) {
                 //             min_r = r;
                 //             min_weight = H3[hash_index]->cells[r].weight;
                 //         } else {
                 //             if (H3[hash_index]->cells[r].weight < min_weight) {
                 //                 min_weight = H3[hash_index]->cells[r].weight;
                 //                 min_r = r;
                 //             }
                 //         }
                 //     }
                 // }
                 // if (have_empty == -1) {
                 //     // H3[hash_index]->cells[0].ID = H2[i]->cells[j].ID;
                 //     // H3[hash_index]->cells[0].Card = 0;
                 //     // H3[hash_index]->cells[0].n1 = 1;
                 //     // H3[hash_index]->cells[0].n2 = 0;
                 //     H3[hash_index]->cells[min_r].ID = H2[i]->cells[j].ID; // new
                 //     H3[hash_index]->cells[min_r].Card = 0;
                 //     H3[hash_index]->cells[min_r].n1 = 1;
                 //     H3[hash_index]->cells[min_r].n2 = 0;
                 // }
            }
        }
    }
    memset(Bitmap2, 0, sizeof(uint8_t) * M2);
    Z2 = M2;
    for (int i = 0; i < N2; ++i) {
        for (int j = 0; j < c2; ++j) {
            H2[i]->cells[j].ID = 0;
            H2[i]->cells[j].Card = 0.0;
            H2[i]->cells[j].Error = 0.0;
        }
    }
}

void KTSketch::post_process() {
    for (int i = 0; i < N3; ++i) {
        for (int j = 0; j < c3; ++j) {
            if (H3[i]->cells[j].Card >= threshold_t) {
                H3[i]->cells[j].n1 += 1;
                H3[i]->cells[j].n2 = 0;
                H3[i]->cells[j].weight = H3[i]->cells[j].n1 - H3[i]->cells[j].n2;
            } else {
                H3[i]->cells[j].n2 += 1;
                H3[i]->cells[j].weight = H3[i]->cells[j].n1 - H3[i]->cells[j].n2;
            }
            H3[i]->cells[j].Card = 0;
        }
    }
    for (int i = 0; i < N2; ++i) {
        for (int j = 0; j < c2; ++j) {
            if (std::abs(H2[i]->cells[j].Card - H2[i]->cells[j].Error) >= lam * threshold_t) {
                uint32_t hash_index, hash_value;
                char hash_input_str[5];
                memcpy(hash_input_str, &(H2[i]->cells[j].ID), 4);
                // insert to bloom filter
                for (int k = 0; k < d1; ++k) {
                    MurmurHash3_x86_32(hash_input_str, 4, filter_seeds[k], &hash_value);
                    hash_index = hash_value % M1;
                    BloomFilter[hash_index] = 1;
                }
                MurmurHash3_x86_32(hash_input_str, 4, summary_hash_seed, &hash_value);
                hash_index = hash_value % N3;
                H3[hash_index]->updateFromStage2(H2[i]->cells[j].ID);
                 //H3[hash_index]->msort();
                 // int have_empty = -1;
                 // int min_r = -1; // new
                 // int min_weight = -1; // new
                 // for (int r = 0; r < c3; ++r) {
                 //     if (H3[hash_index]->cells[r].ID == 0) {
                 //         H3[hash_index]->cells[r].ID = H2[i]->cells[j].ID;
                 //         H3[hash_index]->cells[r].Card = 0;
                 //         H3[hash_index]->cells[r].n1 = 1;
                 //         H3[hash_index]->cells[r].n2 = 0;
                 //         have_empty = r;
                 //     }
                 //     else { // new
                 //         if (min_r == -1) {
                 //             min_r = r;
                 //             min_weight = H3[hash_index]->cells[r].weight;
                 //         } else {
                 //             if (H3[hash_index]->cells[r].weight < min_weight) {
                 //                 min_weight = H3[hash_index]->cells[r].weight;
                 //                 min_r = r;
                 //             }
                 //         }
                 //     }
                 // }
                 // if (have_empty == -1) {
                 //     // H3[hash_index]->cells[0].ID = H2[i]->cells[j].ID;
                 //     // H3[hash_index]->cells[0].Card = 0;
                 //     // H3[hash_index]->cells[0].n1 = 1;
                 //     // H3[hash_index]->cells[0].n2 = 0;
                 //     H3[hash_index]->cells[min_r].ID = H2[i]->cells[j].ID; // new
                 //     H3[hash_index]->cells[min_r].Card = 0;
                 //     H3[hash_index]->cells[min_r].n1 = 1;
                 //     H3[hash_index]->cells[min_r].n2 = 0;
                 // }
            }
        }
    }
}

std::unordered_set<uint32_t> KTSketch::detect(uint32_t thr) {
    std::unordered_set<uint32_t> pred_ps_set;
    for (int i = 0; i < N3; ++i) {
        for (int j = 0; j < c3; ++j) {
            if (H3[i]->cells[j].n1 >= thr) {
                pred_ps_set.insert(H3[i]->cells[j].ID);
            }
        }
    }
    return pred_ps_set;
}

uint32_t KTSketch::query(uint32_t key) {
    uint32_t hash_index, hash_value;
    char hash_input_str[5];
    memcpy(hash_input_str, &key, 4);
    MurmurHash3_x86_32(hash_input_str, 4, summary_hash_seed, &hash_value);
    hash_index = hash_value % N3;
    for (int r = 0; r < c3; ++r) {
        if (H3[hash_index]->cells[r].ID == key) {
            return H3[hash_index]->cells[r].n1;
        }
    }
    return 0;
}
