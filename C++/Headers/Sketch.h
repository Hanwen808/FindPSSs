#ifndef SKETCH_H
#define SKETCH_H
#include <cstdint>
#include <unordered_set>
#include <cstdlib>

#include "MurmurHash3.h"

class Sketch {
public:
    virtual void insert(uint32_t key, uint32_t ele, uint8_t _period) = 0;
    virtual std::unordered_set<uint32_t> detect(uint32_t thr) = 0;
    virtual void clean() = 0;
    virtual uint32_t query(uint32_t key) = 0;
    virtual void post_process() = 0;
};

#endif //SKETCH_H
