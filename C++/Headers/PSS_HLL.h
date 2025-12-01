#ifndef PSS_HLL_H
#define PSS_HLL_H

#include "Sketch.h"

class PSSHLL : public Sketch {
private:
    uint32_t m;
    double q;
    uint32_t d, w;
    uint8_t *R;
    uint32_t **IDs;
    float **Cards;
    uint8_t **Times, **Weights;
    uint32_t hash_seed, sample_seed;
    uint32_t threshold_t;
    uint32_t *row_seeds;
    uint32_t current_packet_no;
public:
    PSSHLL(uint32_t m, uint32_t d, uint32_t w, uint32_t);
    ~PSSHLL() {
        delete[] R;
        for (int i = 0; i < d; ++i) {
            delete[] IDs[i];
            delete[] Cards[i];
            delete[] Times[i];
            delete[] Weights[i];
            delete row_seeds;
        }
    }
    void insert(uint32_t key, uint32_t ele, uint8_t _period);
    std::unordered_set<uint32_t> detect(uint32_t thr);
    void clean();
    uint32_t query(uint32_t key);
    void post_process();
};

#endif //PSS_HLL_H
