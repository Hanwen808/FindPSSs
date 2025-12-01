#ifndef FREERS_H
#define FREERS_H
#include "Sketch.h"
#include "StreamSummary.h"

class FreeRS : public Sketch {
public:
    uint32_t ssLen, m;
    float p;
    uint32_t T;
    uint8_t *R;
    StreamSummary *SS;
    uint32_t hash_seed, hash_seed2;
    uint32_t cur_time;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> current_superspreaders;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> flow_time_map;
    FreeRS(uint32_t _sslen, uint32_t _m, uint32_t _T);
    ~FreeRS() {
        delete R;
        delete SS;
    }
    void insert(uint32_t key, uint32_t ele, uint8_t _period);
    void clean();
    std::unordered_set<uint32_t> detect(uint32_t thr);
    uint32_t query(uint32_t key);
    void post_process();
};

#endif //FREERS_H
