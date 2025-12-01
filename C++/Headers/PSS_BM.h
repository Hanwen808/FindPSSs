#ifndef PSS_H
#define PSS_H
#include "bitmap.h"
#include "Sketch.h"

class PSSBM : public Sketch {
private:
    bitmap B;
    uint32_t c; // count the zero bits in B
    uint32_t d, w, m;
    uint32_t **IDs;
    float **Cards;
    uint8_t **Times, **Weights;
    uint32_t hash_seed;
    uint32_t threshold_t;
    uint32_t *row_seeds;
    
    uint32_t current_packet_no;
    
public:
    PSSBM(uint32_t m, uint32_t d, uint32_t w, uint32_t);
    ~PSSBM() {
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

#endif //PSS_H
